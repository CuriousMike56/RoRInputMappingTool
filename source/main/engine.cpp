/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "engine.h"
#include "SoundScriptManager.h"
#include "TorqueCurve.h"

#ifdef USE_ANGELSCRIPT
#include "ScriptEngine.h"
#endif

BeamEngine::BeamEngine(
float iddle
, float max
, float torque
, std::vector<float> gears
, float diff
, int trucknum)

: clutch_time(0.2f)
, shift_time(0.5f)
, post_shift_time(0.2f)

, numGears(gears.size() - 2)
, gearsRatio( gears )
, inertia(10.0)
, clutchForce(10000.0f)

, curGear(0)

, curEngineRPM(0.0f)
, curGearboxRPM(0.0f)
, curClutch(0.0f)
, curAcc(0.0f)
, curClutchTorque(0.0f)

, shifting(0)
, postshifting(0)

, autocurAcc(0)
, starter(0)
, autoselect(DRIVE)

, curTurboRPM(0.0f)
, apressure(0)
, automode(AUTOMATIC)
, trucknum(trucknum)

#ifdef USE_OPENAL
,	ssm(SoundScriptManager::getSingleton())
#endif //OPENAL
, torqueCurve (new TorqueCurve())


, iddleRPM(iddle)
, maxRPM(max)
, stallRPM(300.0f)
, brakingTorque(-torque/5.0f)
, engineTorque(torque - brakingTorque)

, hasturbo(true)
, hasair(true)
, type('t')
, running(0)
, contact(0)
, hydropump(0.0)
, prime(0)
{
	gearsRatio[0] = -gearsRatio[0];
	for(std::vector< float >::iterator it = gearsRatio.begin(); it != gearsRatio.end(); it++)
	{
		(*it) *= diff;
	}
}

void BeamEngine::setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime)
{
	inertia=einertia;
	type=etype;
	clutchForce=eclutch;

	if (ctime > 0)  clutch_time=ctime;
	if (stime > 0)  shift_time=stime;
	if (pstime > 0) post_shift_time=pstime;

	if (etype=='c')
	{
		// its a car!
		hasturbo=false;
		hasair=false;
		// set default clutch force
		if(clutchForce < 0) clutchForce = 5000.0f;
	} else
	{
		// its a truck
		if(clutchForce < 0) clutchForce = 10000.0f;
	}

}

void BeamEngine::update(float dt, int doUpdate)
{
	if (hasair)
	{
		//air pressure
		apressure+=dt*curEngineRPM;
#ifdef USE_OPENAL
		if (apressure>50000.0 && ssm) {ssm->trigOnce(trucknum, SS_TRIG_AIR_PURGE); apressure=0;};
#endif //OPENAL
	}
	if (hasturbo)
	{
		//update turbo speed (lag)
		float turbotorque=0.0f;
		float turboInertia=0.000003f;
		//braking (compression)
		turbotorque-=curTurboRPM/200000.0f;
		//powering (exhaust) with limiter
		if (curTurboRPM<200000 && running && curAcc>0.06) 
			turbotorque+=1.5*curAcc*(curEngineRPM/maxRPM);
		else
			turbotorque+=0.1*(curEngineRPM/maxRPM);
		
		//integration
		curTurboRPM+=dt*turbotorque/turboInertia;
	}

	//update engine speed
	float totaltorque=0.0f;
	//engine braking
	if (contact) totaltorque+=brakingTorque*curEngineRPM/maxRPM;
	else totaltorque+=10.0*brakingTorque*curEngineRPM/maxRPM;
	//braking by hydropump
	if (curEngineRPM>100.0) totaltorque-=8.0f*hydropump/(curEngineRPM*0.105f*dt);
	//engine power
	//with limiter
	float tqValue = 1.0f;
	float rpmRatio = curEngineRPM / maxRPM;
	if (rpmRatio > 1.0f) rpmRatio = 1.0f;
	if (torqueCurve) tqValue = torqueCurve->getEngineTorque(rpmRatio);
	if (running && contact && curEngineRPM<(maxRPM*1.25) && curEngineRPM>stallRPM)
		totaltorque += engineTorque * curAcc * tqValue;
	if (running && curEngineRPM<stallRPM) stop();
	//starter
	if (contact && starter && curEngineRPM<stallRPM*1.5) totaltorque+=-brakingTorque;//1000.0f;
	//restart
	if (!running && curEngineRPM>stallRPM && starter)
	{
		running=1; 
#ifdef USE_OPENAL
		if(ssm) ssm->trigStart(trucknum, SS_TRIG_ENGINE);
#endif //OPENAL
	}
	//clutch
	float retorque=0.0f;
	if (curGear)
		retorque=curClutchTorque/gearsRatio[curGear + 1];
	
	totaltorque -= retorque;
	
	//integration
	curEngineRPM += dt*totaltorque/inertia;

	//update clutch torque
	float gearboxspinner=0.0f;
	if (curGear){
		gearboxspinner = curEngineRPM/gearsRatio[curGear + 1];
		curClutchTorque = (gearboxspinner-curGearboxRPM)*curClutch*clutchForce;
	}	
	else curClutchTorque=0.0f;

	if (curEngineRPM<0.0f) curEngineRPM=0.0f;

	if (automode<MANUAL)
	{
		//autoshift
		if (shifting)
		{
			shiftclock += dt;
			
			//clutch
			if (shiftclock < clutch_time)
				curClutch = 1.0f - (shiftclock/clutch_time);
			else if (shiftclock > (shift_time - clutch_time))
				curClutch = 1.0f - (shift_time - shiftclock)/clutch_time;
			else curClutch = 0.0f;
			
			//shift
			if (shiftval && shiftclock>clutch_time/2)
			{
#ifdef USE_OPENAL
				if(ssm) ssm->trigStart(trucknum, SS_TRIG_SHIFT);
#endif //OPENAL
				curGear += shiftval;
				if (automode == AUTOMATIC)
					while (curGear > 2 && curGearboxRPM * gearsRatio[curGear + 1] < iddleRPM)
						curGear -= 2;
				
				if (curGear < -1) curGear = -1;
				if (curGear > numGears) curGear = numGears;
				shiftval = 0;
			};
			
			//end of shifting
			if (shiftclock>shift_time)
			{
#ifdef USE_OPENAL
				if(ssm) ssm->trigStop(trucknum, SS_TRIG_SHIFT);
#endif //OPENAL
				setAcc(autocurAcc);
				shifting=0;
				curClutch=1.0;
				postshifting=1;
				postshiftclock=0.0;
			}
		}
		
		if (postshifting)
		{
			postshiftclock+=dt;
			if (postshiftclock>post_shift_time) postshifting=0;
		}

		//auto-declutch
		if (shifting)
		{
			//we are shifting, just avoid stalling in worst case
			if (curEngineRPM<stallRPM*1.2) curClutch=0;
		} else if (postshifting)
		{
			//we are postshifting, no gear change
			if (curEngineRPM<stallRPM*1.2 && curAcc<0.5) curClutch=0;
			else curClutch=1.0;
		} else if (curEngineRPM<stallRPM*1.2 && curAcc<0.5)
		{
			curClutch=0; /*curGear=0;*/
		} else if (curGear==1 || curGear==-1)
		{
			//1st gear : special
			if (curEngineRPM>iddleRPM)
			{
				curClutch = ((curEngineRPM - iddleRPM)/(maxRPM - iddleRPM));
				if (curClutch>1.0) curClutch=1.0;
			}
			else curClutch=0.0;
		}
		else curClutch=1.0;
	}

	//audio stuff	
	updateAudio(doUpdate);
}

void BeamEngine::updateAudio(int doUpdate)
{
#ifdef USE_OPENAL
	
	// check if we have audio now instead of multiple places
	if( !ssm ) return;
	
	if (hasturbo) ssm->modulate(trucknum, SS_MOD_TURBO, curTurboRPM);
	if (doUpdate)
	{
		ssm->modulate(trucknum, SS_MOD_ENGINE, curEngineRPM);
		ssm->modulate(trucknum, SS_MOD_TORQUE, curClutchTorque);
		ssm->modulate(trucknum, SS_MOD_GEARBOX, curGearboxRPM);
		//gear hack
		if (curGear<0 || automode!=AUTOMATIC) return;
		
		if (!shifting && !postshifting && autoselect == DRIVE)
		{
			if(curEngineRPM > maxRPM-100.0 && curGear < numGears ) shift(1);
			else if(curEngineRPM < iddleRPM && curGear > 1 ) shift(-1);
		}
		
	}
	// reverse gear beep
	if (curGear==-1 && running) 
	{
		ssm->trigStart(trucknum, SS_TRIG_REVERSE_GEAR);
	}
	else
	{
		ssm->trigStop(trucknum, SS_TRIG_REVERSE_GEAR);
	}
#endif //OPENAL
	
}


float BeamEngine::getRPM()
{
	return curEngineRPM;
}

void BeamEngine::setRPM(float value)
{
	curEngineRPM = value;
}

void BeamEngine::toggleAutoMode()
{
	automode++;
	if (automode>MANUAL_RANGES) automode=AUTOMATIC;

	// this switches off all automatic symbols when in manual mode
	if(automode!=AUTOMATIC) autoselect = MANUALMODE;
	else					autoselect = NEUTRAL; 

	if (automode==MANUAL_RANGES)
		{
			this->setGearRange(0);
			this->setGear(0);
		}
}

int BeamEngine::getAutoMode()
{
	return automode;
}

void BeamEngine::setAutoMode(int mode)
{
	automode = mode;
}

void BeamEngine::setAcc(float val)
{
	if (prime)
	{
		if (curEngineRPM<850) val=1.0;
		else if (curEngineRPM<900) {float t=(900.0f-curEngineRPM)/50.0f;if (t>val) val=t;};
	}
#ifdef USE_OPENAL
	if(ssm) ssm->modulate(trucknum, SS_MOD_INJECTOR, val);
#endif //OPENAL
	curAcc=val*0.94f+0.06f;
}

float BeamEngine::getTurboPSI() {return curTurboRPM/10000.0f;}

float BeamEngine::getAcc()
{
	return curAcc;
}

//this is mainly for smoke...
void BeamEngine::netForceSettings(float rpm, float force)
{
	curEngineRPM=rpm;
	curAcc=force;
	running=(fabs(rpm)>10.0);
}

float BeamEngine::getSmoke()
{
	int maxTurboRPM = 200000.0f;
	if (running) return curAcc*(1.0f-curTurboRPM/maxTurboRPM);//*engineTorque/5000.0;
	else return -1;
}

float BeamEngine::getTorque()
{
	if (curClutchTorque>1000000.0) return 1000000.0;
	if (curClutchTorque<-1000000.0) return -1000000.0;
	return curClutchTorque;
}

void BeamEngine::setSpin(float rpm)
{
	curGearboxRPM=rpm;
}

//for hydros acceleration
float BeamEngine::getCrankFactor()
{
	return 1.0f+4.0f*(curEngineRPM-800.0f)/(maxRPM-800.0f);
}

void BeamEngine::setClutch(float clutch)
{
	curClutch=clutch;
}

float BeamEngine::getClutch()
{
	return curClutch;
}

float BeamEngine::getClutchForce()
{
	return clutchForce;
}

void BeamEngine::toggleContact()
{
	contact=!contact;
#ifdef USE_OPENAL
	if (contact && ssm) ssm->trigStart(trucknum, SS_TRIG_IGNITION);
	else if(ssm) ssm->trigStop(trucknum, SS_TRIG_IGNITION);
#endif //OPENAL
}

//quick start
void BeamEngine::start()
{
	if (automode == AUTOMATIC) 
	{
		curGear=1;
		autoselect=DRIVE;
	}
	else
	{
		if (automode == SEMIAUTO) 
		{
			curGear=1;
		}
		else
		{
			curGear=0;
		}
		autoselect=MANUALMODE;
	}
	curClutch=0;
	curEngineRPM=750.0f;
	curGearboxRPM=750.0f;
	curClutchTorque=0.0f;
	curTurboRPM=0.0f;
	apressure=0.0f;
	running=1;
	contact=1;
#ifdef USE_OPENAL
	if(ssm) ssm->trigStart(trucknum, SS_TRIG_IGNITION);
	setAcc(0);
	if(ssm) ssm->trigStart(trucknum, SS_TRIG_ENGINE);
#endif //OPENAL
}

void BeamEngine::offstart()
{
	curGear=0;
	curClutch=0;
	autoselect=NEUTRAL;
	curEngineRPM=0.0f;
	running=0;
	contact=0;
#ifdef USE_OPENAL
	if(ssm) ssm->trigStop(trucknum, SS_TRIG_IGNITION);
//		setAcc(0);
	if(ssm) ssm->trigStop(trucknum, SS_TRIG_ENGINE);
#endif //OPENAL
}

void BeamEngine::setstarter(int v)
{
	starter=v;
	if (v && curEngineRPM<750.0) setAcc(1.0f);
}

//low level gear changing
int BeamEngine::getGear() {return curGear;}
void BeamEngine::setGear(int v) {curGear=v;}

int BeamEngine::getGearRange() {return curGearRange;}
void BeamEngine::setGearRange(int v) {curGearRange=v;}


//stalling engine
void BeamEngine::stop()
{
	if (!running) return;
//		curGear=0;
//		curClutch=0;
	running=0;
#ifdef USE_ANGELSCRIPT
	//Script Event - engine death
	ScriptEngine::getSingleton().triggerEvent(ScriptEngine::SE_TRUCK_ENGINE_DIED, trucknum);
#endif
#ifdef USE_OPENAL
	if(ssm) ssm->trigStop(trucknum, SS_TRIG_ENGINE);
#endif //OPENAL
}


//high level controls
void BeamEngine::autoSetAcc(float val)
{
	autocurAcc=val;
	if (!shifting) setAcc(val);
}


void BeamEngine::shift(int val)
{
	if (automode<MANUAL)
	{
#ifdef USE_OPENAL
		if(ssm) ssm->trigStart(trucknum, SS_TRIG_SHIFT);
#endif //OPENAL
		shiftval=val;
		shifting=1;
		shiftclock=0.0;
		setAcc(0.0);
	}
	else
	{
		if (curClutch>0.25)
		{
#ifdef USE_OPENAL
			if(ssm) ssm->trigOnce(trucknum, SS_TRIG_GEARSLIDE);
#endif //OPENAL
		}
		else
		{
#ifdef USE_OPENAL
			if(ssm) ssm->trigOnce(trucknum, SS_TRIG_SHIFT);
#endif //OPENAL
			curGear+=val;
			if (curGear<-1) curGear=-1;
			if (curGear>numGears) curGear=numGears;
		}
	}
}

void BeamEngine::shiftTo(int val)
{
	if (val==curGear || val>getNumGears()) return;
	if (automode<MANUAL)
	{
		shiftval=val-curGear;
		shifting=1;
		shiftclock=0.0;
		setAcc(0.0);
	}
	else
	{
		if (curClutch>0.25)
		{
#ifdef USE_OPENAL
			if(ssm) ssm->trigOnce(trucknum, SS_TRIG_GEARSLIDE);
#endif //OPENAL
		}
		else
		{
#ifdef USE_OPENAL
			if(ssm) ssm->trigOnce(trucknum, SS_TRIG_SHIFT);
#endif //OPENAL
			curGear=val;
			if (curGear<-1) curGear=-1;
			if (curGear>numGears) curGear=numGears;
		}
	}
}

void BeamEngine::updateShifts()
{
/*		if (autoselect==REAR) shiftTo(-1);
	if (autoselect==NEUTRAL) shiftTo(0);
	if (autoselect==DRIVE) shiftTo(1);
	if (autoselect==TWO) shiftTo(2);
	if (autoselect==ONE) shiftTo(1);
*/
#ifdef USE_OPENAL
	if(ssm) ssm->trigOnce(trucknum, SS_TRIG_SHIFT);
#endif //OPENAL
	if (autoselect==REAR) curGear=-1;
	if (autoselect==NEUTRAL) curGear=0;
	if (autoselect==DRIVE && curGear <= 0) curGear=1;
	if (autoselect==TWO) curGear=2;
	if (autoselect==ONE) curGear=1;
}

void BeamEngine::autoShiftUp()
{
	if (autoselect!=REAR) 
	{
		autoselect=(autoswitch)(autoselect-1);
		updateShifts();
	}
}

void BeamEngine::autoShiftDown()
{
	if (autoselect!=ONE) 
	{
		autoselect=(autoswitch)(autoselect+1);
		updateShifts();
	}
}

int BeamEngine::getAutoShift()
{
	return (int)autoselect;
}

void BeamEngine::setManualClutch(float val)
{
	if (automode>=MANUAL)
	{
		if (val<0) val=0;
		curClutch=1.0-val;
	}
}

BeamEngine::~BeamEngine()
{
	// delete NULL is safe
	//delete gearsRatio; gearsRatio = NULL;
	delete torqueCurve; torqueCurve = NULL;
}
