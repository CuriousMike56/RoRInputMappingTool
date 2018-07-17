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
#ifndef __Engine_H__
#define __Engine_H__


class SoundScriptManager;
class TorqueCurve;

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "rormemory.h"

#define AUTOMATIC 0
#define SEMIAUTO 1
#define MANUAL 2
#define MANUAL_STICK 3
#define MANUAL_RANGES 4

enum autoswitch {REAR=0, NEUTRAL=1, DRIVE=2, TWO=3, ONE=4, MANUALMODE=5};

class BeamEngine : public MemoryAllocatedObject
{
protected:
	float clutch_time;
	float shift_time;
	float post_shift_time;

	int numGears;
	std::vector<float> gearsRatio;
	float inertia;
	float clutchForce;

	int curGear;
	int curGearRange;
	float curEngineRPM;
	float curGearboxRPM;
	float curClutch;
	float curAcc;
	float curClutchTorque;
	//shifting
	int shifting;
	int shiftval;
	float shiftclock;
	int postshifting;
	float postshiftclock;
	//auto
	float autocurAcc;
	int starter;
	autoswitch autoselect;
	//turbo
	float curTurboRPM;
	//air pressure
	float apressure;
	int automode;
	int trucknum;
	SoundScriptManager *ssm;
	TorqueCurve *torqueCurve;

public:
	float iddleRPM;
	float maxRPM;
	float stallRPM;
	float brakingTorque;
	float engineTorque;

	bool hasturbo;
	bool hasair;
	char type;
	int running;
	int contact;
	char status[256];
	float hydropump;
	int prime;

	BeamEngine(float iddle, float max, float torque, std::vector<float> gears, float diff, int trucknum);
	void setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime);
	void update(float dt, int doUpdate);
	void updateAudio(int doUpdate);
	float getRPM();
	void setRPM(float value);
	
	void toggleAutoMode();
	int getAutoMode();
	void setAutoMode(int mode);
	
	void setAcc(float val);
	float getTurboPSI();
	float getAcc();
	void netForceSettings(float rpm, float force);
	float getSmoke();
	float getTorque();
	void setSpin(float rpm);
	//for hydros acceleration
	float getCrankFactor();
	void setClutch(float clutch);
	float getClutch();
	float getClutchForce();
	void toggleContact();
	//quick start
	void start();
	void offstart();
	void setstarter(int v);
	//low level gear changing
	int getGear();
	int getGearRange();
	void setGear(int v);
	void setGearRange(int v);
	//stalling engine
	void stop();
	//high level controls
	void autoSetAcc(float val);
	void shift(int val);
	void shiftTo(int val);
	void updateShifts();
	void autoShiftUp();
	void autoShiftDown();
	int getAutoShift();
	void setManualClutch(float val);
	int getNumGears() { return gearsRatio.size() - 2; };
	int getNumGearsRanges() {return getNumGears()/6+1; }
	float getMaxRPM() { return maxRPM; };
	TorqueCurve *getTorqueCurve() { return torqueCurve; };
	~BeamEngine();
};


#endif
