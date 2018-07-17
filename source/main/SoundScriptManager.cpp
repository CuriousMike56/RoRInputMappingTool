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
#ifdef USE_OPENAL

#include "Ogre.h"
#include "SoundScriptManager.h"
#include "Settings.h"

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

using namespace Ogre;

static SoundScriptManager* singleton=NULL;

SoundScriptManager::SoundScriptManager()
{
	singleton=this;
	instance_counter=0;
	maxDistance=500.0;
	rolloffFactor=1.0;
	referenceDistance=7.5;
	for (int i=0; i<SS_MAX_TRIG; i++) free_trigs[i]=0;
	for (int i=0; i<SS_MAX_MOD; i++)
	{
		free_pitches[i]=0;
		free_gains[i]=0;
	}
	for (int i=0; i<SS_MAX_TRIG*MAX_TRUCKS; i++) statemap[i]=false;
	sm=new SoundManager(); //we can give a device name if we want here
	LogManager::getSingleton().logMessage("SoundScriptManager: Sound Manager started with "+StringConverter::toString(sm->maxSources())+" sources");
	mScriptPatterns.push_back("*.soundscript");
	ResourceGroupManager::getSingleton()._registerScriptLoader(this);
}

void SoundScriptManager::trigOnce(int truck, int trig)
{
	for (int i=0; i<free_trigs[trig]; i++)
	{
		SoundScriptInstance* inst=trigs[trig+i*SS_MAX_TRIG];
		if (inst->truck==truck) inst->runOnce();
	}
}

void SoundScriptManager::trigStart(int truck, int trig)
{
	if (getTrigState(truck, trig)) return;
	statemap[truck*SS_MAX_TRIG+trig]=true;
	for (int i=0; i<free_trigs[trig]; i++)
	{
		SoundScriptInstance* inst=trigs[trig+i*SS_MAX_TRIG];
		if (inst->truck==truck) inst->start();
	}
}

void SoundScriptManager::trigStop(int truck, int trig)
{
	if (!getTrigState(truck, trig)) return;
	statemap[truck*SS_MAX_TRIG+trig]=false;
	for (int i=0; i<free_trigs[trig]; i++)
	{
		SoundScriptInstance* inst=trigs[trig+i*SS_MAX_TRIG];
		if (inst->truck==truck) inst->stop();
	}
}

void SoundScriptManager::trigToggle(int truck, int trig)
{
	if (getTrigState(truck, trig)) trigStop(truck, trig);
	else trigStart(truck, trig);
}

bool SoundScriptManager::getTrigState(int truck, int trig)
{
	return statemap[truck*SS_MAX_TRIG+trig];
}

void SoundScriptManager::modulate(int truck, int mod, float value)
{
	for (int i=0; i<free_gains[mod]; i++)
	{
		SoundScriptInstance* inst=gains[mod+i*SS_MAX_MOD];
		if (inst->truck==truck)
		{
			//this one requires modulation
			float gain=value*value*inst->templ->gain_square+value*inst->templ->gain_multiplier+inst->templ->gain_offset;
			if (gain<0.0) gain=0.0;
			if (gain>1.0) gain=1.0;
			inst->setGain(gain);
		}
	}
	for (int i=0; i<free_pitches[mod]; i++)
	{
		SoundScriptInstance* inst=pitches[mod+i*SS_MAX_MOD];
		if (inst->truck==truck)
		{
			//this one requires modulation
			float pitch=value*value*inst->templ->pitch_square+value*inst->templ->pitch_multiplier+inst->templ->pitch_offset;
			if (pitch<0) pitch=0;
			inst->setPitch(pitch);
		}
	}
}

void SoundScriptManager::setCamera(Vector3 position, Vector3 direction, Vector3 up, Vector3 velocity)
{
	sm->setCamera(position, direction, up, velocity);
}

SoundScriptManager *SoundScriptManager::getSingleton()
{
	if (SETTINGS.getSetting("3D Sound renderer") == "No sound") return 0;
	if (!singleton) singleton=new SoundScriptManager();
	return singleton;
}

const StringVector& SoundScriptManager::getScriptPatterns(void) const
{
    return mScriptPatterns;
}

Real SoundScriptManager::getLoadingOrder(void) const
{
    /// Load late
    return 1000.0f;
}

SoundScriptTemplate* SoundScriptManager::createTemplate(String name, String groupname, String filename)
{
	//first, search if there is a template name collision
	if(templates.find(name) != templates.end())
	{
		OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, "SoundScript with the name " + name +
            " already exists.", "SoundScriptManager::createTemplate");
		return NULL;
	}

	SoundScriptTemplate *ssi = new SoundScriptTemplate(name, groupname, filename);
	templates[name] = ssi;
	return ssi;
}

void SoundScriptManager::unloadResourceGroup(String groupname)
{
	//first, search if there is a template name collision
	for(std::map<Ogre::String, SoundScriptTemplate*>::iterator it = templates.begin(); it!=templates.end();)
		if (it->second->groupname == groupname)
			templates.erase(it++);
		else
			++it;
}

void SoundScriptManager::clearTemplates()
{
	templates.clear();
}

SoundScriptInstance* SoundScriptManager::createInstance(Ogre::String templatename, int truck, Ogre::SceneNode *toAttach)
{
	//first, search template
	SoundScriptTemplate* templ=NULL;

	if(templates.find(templatename) == templates.end())
		// no template with that name found
		return NULL;
	templ = templates[templatename];
	if (templ->trigger_source==SS_TRIG_NONE) return NULL; //invalid template!
	//ok create instance
	SoundScriptInstance* inst=new SoundScriptInstance(truck, templ, sm, templ->filename+"-"+StringConverter::toString(truck)+"-"+StringConverter::toString(instance_counter));
	instance_counter++;
	//register to lookup tables
	trigs[templ->trigger_source+free_trigs[templ->trigger_source]*SS_MAX_TRIG]=inst;
	free_trigs[templ->trigger_source]=free_trigs[templ->trigger_source]+1;
	if (templ->gain_source!=SS_MOD_NONE)
	{
		gains[templ->gain_source+free_gains[templ->gain_source]*SS_MAX_MOD]=inst;
		free_gains[templ->gain_source]=free_gains[templ->gain_source]+1;
	}
	if (templ->pitch_source!=SS_MOD_NONE)
	{
		pitches[templ->pitch_source+free_pitches[templ->pitch_source]*SS_MAX_MOD]=inst;
		free_pitches[templ->pitch_source]=free_pitches[templ->pitch_source]+1;
	}
	//for always on
	if (templ->trigger_source==SS_TRIG_ALWAYSON) inst->start();
	return inst;
}

void SoundScriptManager::parseScript(DataStreamPtr& stream, const String& groupName)
{
	String line;
	SoundScriptTemplate* sst;
	std::vector<String> vecparams;

	LogManager::getSingleton().logMessage("SoundScriptManager: Parsing script "+stream->getName());
	sst = 0;

	while(!stream->eof())
	{
		line = stream->getLine();
		// Ignore comments & blanks
		if (!(line.length() == 0 || line.substr(0,2) == "//"))
		{
			if (sst == 0)
			{
				// No current SoundScript
				// So first valid data should be a SoundScript name
				LogManager::getSingleton().logMessage("SoundScriptManager: creating template "+line);
				sst = createTemplate(line, groupName, stream->getName());
				if (!sst)
				{
					//there is a name collision for this Sound Script
					LogManager::getSingleton().logMessage("SoundScriptManager: Error, this sound script is already defined: "+line);
					skipToNextOpenBrace(stream);
					skipToNextCloseBrace(stream);
					continue;
				}
				// Skip to and over next {
				skipToNextOpenBrace(stream);
			}
			else
			{
				// Already in a ss
				if (line == "}")
				{
					// Finished ss
					sst = 0;
				}
				else
				{
					// Attribute
					// Split params on space
					Ogre::vector<String>::type vecparams = StringUtil::split(line, "\t ", 0);

					if (!sst->setParameter(vecparams))
					{
						LogManager::getSingleton().logMessage("Bad SoundScript attribute line: '"
							+ line + "' in " + stream->getName());
					}
				}
			}
		}
	}
}


void SoundScriptManager::skipToNextCloseBrace(DataStreamPtr& stream)
{
	String line;
	while (!stream->eof() && line != "}")
	{
		line = stream->getLine();
	}
}

void SoundScriptManager::skipToNextOpenBrace(DataStreamPtr& stream)
{
	String line;
	while (!stream->eof() && line != "{")
	{
		line = stream->getLine();
	}
}

void SoundScriptManager::soundEnable(bool state)
{
	if (state)
		sm->resumeAllSounds();
	else
		sm->pauseAllSounds();
}

//=====================================================================

SoundScriptTemplate::SoundScriptTemplate(String name, String groupname, String filename)
{
	this->name=name;
	this->groupname=groupname;
	this->filename=filename;
	trigger_source=SS_TRIG_NONE;
	pitch_source=SS_MOD_NONE;
	gain_source=SS_MOD_NONE;
	start_sound_pitch=0;
	has_start_sound=false;
	stop_sound_pitch=0;
	has_stop_sound=false;
	unpitchable=false;
	free_sound=0;
	pitch_multiplier=1;
	pitch_square=0;
	pitch_offset=0;
	gain_multiplier=1;
	gain_square=0;
	gain_offset=0;
}

SoundScriptTemplate::~SoundScriptTemplate()
{
}

bool SoundScriptTemplate::setParameter(Ogre::StringVector vec)
{
//	for (int i=0; i<vec.size(); i++) LogManager::getSingleton().logMessage("SoundScriptManager: Parsing line '"+vec[i]+"'");

	if (vec.size()<1) return false;
	if (vec[0]==String("trigger_source"))
	{
		if (vec.size()<2) return false;
		if (vec[1]==String("engine")) {trigger_source=SS_TRIG_ENGINE;return true;};
		if (vec[1]==String("aeroengine1")) {trigger_source=SS_TRIG_AEROENGINE1;return true;};
		if (vec[1]==String("aeroengine2")) {trigger_source=SS_TRIG_AEROENGINE2;return true;};
		if (vec[1]==String("aeroengine3")) {trigger_source=SS_TRIG_AEROENGINE3;return true;};
		if (vec[1]==String("aeroengine4")) {trigger_source=SS_TRIG_AEROENGINE4;return true;};
		if (vec[1]==String("aeroengine5")) {trigger_source=SS_TRIG_AEROENGINE5;return true;};
		if (vec[1]==String("aeroengine6")) {trigger_source=SS_TRIG_AEROENGINE6;return true;};
		if (vec[1]==String("aeroengine7")) {trigger_source=SS_TRIG_AEROENGINE7;return true;};
		if (vec[1]==String("aeroengine8")) {trigger_source=SS_TRIG_AEROENGINE8;return true;};
		if (vec[1]==String("horn")) {trigger_source=SS_TRIG_HORN;return true;};
		if (vec[1]==String("brake")) {trigger_source=SS_TRIG_BRAKE;return true;};
		if (vec[1]==String("pump")) {trigger_source=SS_TRIG_PUMP;return true;};
		if (vec[1]==String("starter")) {trigger_source=SS_TRIG_STARTER;return true;};
		if (vec[1]==String("always_on")) {trigger_source=SS_TRIG_ALWAYSON;return true;};
		if (vec[1]==String("repair")) {trigger_source=SS_TRIG_REPAIR;return true;};
		if (vec[1]==String("air")) {trigger_source=SS_TRIG_AIR;return true;};
		if (vec[1]==String("gpws_ap_disconnect")) {trigger_source=SS_TRIG_GPWS_APDISCONNECT;return true;};
		if (vec[1]==String("gpws_10")) {trigger_source=SS_TRIG_GPWS_10;return true;};
		if (vec[1]==String("gpws_20")) {trigger_source=SS_TRIG_GPWS_20;return true;};
		if (vec[1]==String("gpws_30")) {trigger_source=SS_TRIG_GPWS_30;return true;};
		if (vec[1]==String("gpws_40")) {trigger_source=SS_TRIG_GPWS_40;return true;};
		if (vec[1]==String("gpws_50")) {trigger_source=SS_TRIG_GPWS_50;return true;};
		if (vec[1]==String("gpws_100")) {trigger_source=SS_TRIG_GPWS_100;return true;};
		if (vec[1]==String("gpws_pull_up")) {trigger_source=SS_TRIG_GPWS_PULLUP;return true;};
		if (vec[1]==String("gpws_minimums")) {trigger_source=SS_TRIG_GPWS_MINIMUMS;return true;};
		if (vec[1]==String("air_purge")) {trigger_source=SS_TRIG_AIR_PURGE;return true;};
		if (vec[1]==String("shift")) {trigger_source=SS_TRIG_SHIFT;return true;};
		if (vec[1]==String("gear_slide")) {trigger_source=SS_TRIG_GEARSLIDE;return true;};
		if (vec[1]==String("creak") && (SETTINGS.getSetting("Creak Sound")=="Yes")) {trigger_source=SS_TRIG_CREAK;return true;};
		if (vec[1]==String("break")) {trigger_source=SS_TRIG_BREAK;return true;};
		if (vec[1]==String("screetch")) {trigger_source=SS_TRIG_SCREETCH;return true;};
		if (vec[1]==String("parking_brake")) {trigger_source=SS_TRIG_PARK;return true;};
		if (vec[1]==String("afterburner1")) {trigger_source=SS_TRIG_AFTERBURNER1;return true;};
		if (vec[1]==String("afterburner2")) {trigger_source=SS_TRIG_AFTERBURNER2;return true;};
		if (vec[1]==String("afterburner3")) {trigger_source=SS_TRIG_AFTERBURNER3;return true;};
		if (vec[1]==String("afterburner4")) {trigger_source=SS_TRIG_AFTERBURNER4;return true;};
		if (vec[1]==String("afterburner5")) {trigger_source=SS_TRIG_AFTERBURNER5;return true;};
		if (vec[1]==String("afterburner6")) {trigger_source=SS_TRIG_AFTERBURNER6;return true;};
		if (vec[1]==String("afterburner7")) {trigger_source=SS_TRIG_AFTERBURNER7;return true;};
		if (vec[1]==String("afterburner8")) {trigger_source=SS_TRIG_AFTERBURNER8;return true;};
		if (vec[1]==String("aoa_horn")) {trigger_source=SS_TRIG_AOA;return true;};
		if (vec[1]==String("ignition")) {trigger_source=SS_TRIG_IGNITION;return true;};
		if (vec[1]==String("reverse_gear")) {trigger_source=SS_TRIG_REVERSE_GEAR;return true;};
		if (vec[1]==String("turn_signal")) {trigger_source=SS_TRIG_TURN_SIGNAL;return true;};
		return false;
	}
	if (vec[0]==String("pitch_source"))
	{
		if (vec.size()<2) return false;
		int mod=parseModulation(vec[1]);
		if (mod>=0) {pitch_source=mod;return true;}
		return false;
	}
	if (vec[0]==String("pitch_factors"))
	{
		if (vec.size()<3) return false;
		pitch_offset=StringConverter::parseReal(vec[1]);
		pitch_multiplier=StringConverter::parseReal(vec[2]);
		if (vec.size()==4) pitch_square=StringConverter::parseReal(vec[3]);
		return true;
	}
	if (vec[0]==String("gain_source"))
	{
		if (vec.size()<2) return false;
		int mod=parseModulation(vec[1]);
		if (mod>=0) {gain_source=mod;return true;}
		return false;
	}
	if (vec[0]==String("gain_factors"))
	{
		if (vec.size()<3) return false;
		gain_offset=StringConverter::parseReal(vec[1]);
		gain_multiplier=StringConverter::parseReal(vec[2]);
		if (vec.size()==4) gain_square=StringConverter::parseReal(vec[3]);
		return true;
	}
	if (vec[0]==String("start_sound"))
	{
		if (vec.size()<3) return false;
		start_sound_pitch=StringConverter::parseReal(vec[1]); //unparsable (e.g. "unpitched") will result in value 0.0
		start_sound_name=vec[2];
		has_start_sound=true;
		return true;
	}
	if (vec[0]==String("stop_sound"))
	{
		if (vec.size()<3) return false;
		stop_sound_pitch=StringConverter::parseReal(vec[1]); //unparsable (e.g. "unpitched") will result in value 0.0
		stop_sound_name=vec[2];
		has_stop_sound=true;
		return true;
	}
	if (vec[0]==String("sound"))
	{
		if (vec.size()<3) return false;
		sound_pitches[free_sound]=StringConverter::parseReal(vec[1]); //unparsable (e.g. "unpitched") will result in value 0.0
		if (sound_pitches[free_sound]==0) unpitchable=true;
		if (free_sound>0 && !unpitchable && sound_pitches[free_sound]<=sound_pitches[free_sound-1]) return false;
		sound_names[free_sound]=vec[2];
		free_sound++;
		return true;
	}
	return false;
}

int SoundScriptTemplate::parseModulation(String str)
{
	if (str==String("none")) return SS_MOD_NONE;
	if (str==String("engine_rpm")) return SS_MOD_ENGINE;
	if (str==String("turbo_rpm")) return SS_MOD_TURBO;
	if (str==String("aeroengine1_rpm")) return SS_MOD_AEROENGINE1;
	if (str==String("aeroengine2_rpm")) return SS_MOD_AEROENGINE2;
	if (str==String("aeroengine3_rpm")) return SS_MOD_AEROENGINE3;
	if (str==String("aeroengine4_rpm")) return SS_MOD_AEROENGINE4;
	if (str==String("aeroengine5_rpm")) return SS_MOD_AEROENGINE5;
	if (str==String("aeroengine6_rpm")) return SS_MOD_AEROENGINE6;
	if (str==String("aeroengine7_rpm")) return SS_MOD_AEROENGINE7;
	if (str==String("aeroengine8_rpm")) return SS_MOD_AEROENGINE8;
	if (str==String("wheel_speed_kmph")) return SS_MOD_WHEELSPEED;
	if (str==String("injector_ratio")) return SS_MOD_INJECTOR;
	if (str==String("torque_nm")) return SS_MOD_TORQUE;
	if (str==String("gearbox_rpm")) return SS_MOD_GEARBOX;
	if (str==String("creak")) return SS_MOD_CREAK;
	if (str==String("break")) return SS_MOD_BREAK;
	if (str==String("screetch")) return SS_MOD_SCREETCH;
	if (str==String("pump_rpm")) return SS_MOD_PUMP;
	if (str==String("aeroengine1_throttle")) return SS_MOD_THROTTLE1;
	if (str==String("aeroengine2_throttle")) return SS_MOD_THROTTLE2;
	if (str==String("aeroengine3_throttle")) return SS_MOD_THROTTLE3;
	if (str==String("aeroengine4_throttle")) return SS_MOD_THROTTLE4;
	if (str==String("aeroengine5_throttle")) return SS_MOD_THROTTLE5;
	if (str==String("aeroengine6_throttle")) return SS_MOD_THROTTLE6;
	if (str==String("aeroengine7_throttle")) return SS_MOD_THROTTLE7;
	if (str==String("aeroengine8_throttle")) return SS_MOD_THROTTLE8;
	if (str==String("air_speed_knots")) return SS_MOD_AIRSPEED;
	if (str==String("angle_of_attack_degree")) return SS_MOD_AOA;
	return -1;
}

//====================================================================

SoundScriptInstance::SoundScriptInstance(int truck, SoundScriptTemplate *templ, SoundManager* sm, String instancename)
{
	this->templ=templ;
	this->truck=truck;
	this->sm=sm;
	startSound=NULL;
	stopSound=NULL;
	//create sounds
	if (templ->has_start_sound)
		startSound=sm->createSound(templ->start_sound_name);
	if (templ->has_stop_sound)
		stopSound=sm->createSound(templ->stop_sound_name);
	for (int i=0; i<templ->free_sound; i++)
		sounds[i]=sm->createSound(templ->sound_names[i]);
	lastgain=1.0;
	setPitch(0.0);
	setGain(1.0);
	LogManager::getSingleton().logMessage("SoundScriptInstance: instance created: "+instancename);
}

void SoundScriptInstance::setPitch(float value)
{
	if (startSound)
	{
		startSound_pitchgain=pitchgain_cutoff(templ->start_sound_pitch, value);
		if (startSound_pitchgain!=0.0 && templ->start_sound_pitch!=0.0)
			startSound->setPitch(value/templ->start_sound_pitch);
	}
	if (templ->free_sound)
	{
		//searching the interval
		int up=0;
		for (up=0; up<templ->free_sound; up++)
		{
			if (templ->sound_pitches[up]>value) break;
		}
		if (up==0)
		{
			//low sound case
			sounds_pitchgain[0]=pitchgain_cutoff(templ->sound_pitches[0], value);
			if (sounds_pitchgain[0]!=0.0 && templ->sound_pitches[0]!=0.0 && sounds[0])
				sounds[0]->setPitch(value/templ->sound_pitches[0]);
			for (int i=1; i<templ->free_sound; i++)
			{
				if (templ->sound_pitches[i]!=0.0)
				{
					sounds_pitchgain[i]=0.0;
					//pause?
				}
				else
				{
					sounds_pitchgain[i]=1.0; //unpitched
				}
			}
		}
		else if (up==templ->free_sound)
		{
			//high sound case
			for (int i=0; i<templ->free_sound-1; i++)
			{
				if (templ->sound_pitches[i]!=0.0)
				{
					sounds_pitchgain[i]=0.0;
					//pause?
				}
				else
				{
					sounds_pitchgain[i]=1.0; //unpitched
				}
			}
			sounds_pitchgain[templ->free_sound-1]=1.0;
			if (templ->sound_pitches[templ->free_sound-1]!=0.0 && sounds[templ->free_sound-1])
			{
				sounds[templ->free_sound-1]->setPitch(value/templ->sound_pitches[templ->free_sound-1]);
			}
		}
		else
		{
			//middle sound case
			int low=up-1;
			for (int i=0; i<low; i++)
			{
				if (templ->sound_pitches[i]!=0.0)
				{
					sounds_pitchgain[i]=0.0;
					//pause?
				}
				else
				{
					sounds_pitchgain[i]=1.0; //unpitched
				}
			}
			if (templ->sound_pitches[low]!=0.0 && sounds[low])
			{
				sounds_pitchgain[low]=(templ->sound_pitches[up]-value)/(templ->sound_pitches[up]-templ->sound_pitches[low]);
				sounds[low]->setPitch(value/templ->sound_pitches[low]);
			}
			else
			{
				sounds_pitchgain[low]=1.0; //unpitched
			}
			if (templ->sound_pitches[up]!=0.0 && sounds[up])
			{
				sounds_pitchgain[up]=(value-templ->sound_pitches[low])/(templ->sound_pitches[up]-templ->sound_pitches[low]);
				sounds[up]->setPitch(value/templ->sound_pitches[up]);
			}
			else
			{
				sounds_pitchgain[up]=1.0; //unpitched
			}
			for (int i=up+1; i<templ->free_sound; i++)
			{
				if (templ->sound_pitches[i]!=0.0)
				{
					sounds_pitchgain[i]=0.0;
					//pause?
				}
				else
				{
					sounds_pitchgain[i]=1.0; //unpitched
				}
			}
		}
	}

	if (stopSound)
	{
		stopSound_pitchgain=pitchgain_cutoff(templ->stop_sound_pitch, value);
		if (stopSound_pitchgain!=0.0 && templ->stop_sound_pitch!=0.0)
			stopSound->setPitch(value/templ->stop_sound_pitch);
	}
	//propagate new gains
	setGain(lastgain);
}

float SoundScriptInstance::pitchgain_cutoff(float sourcepitch, float targetpitch)
{
	if (sourcepitch==0.0) return 1.0; //unpitchable
	if (targetpitch>sourcepitch/PITCHDOWN_FADE_FACTOR) return 1.0; //pass
	if (targetpitch<sourcepitch/PITCHDOWN_CUTOFF_FACTOR) return 0.0; //cutoff
	//linear fading
	return (targetpitch-sourcepitch/PITCHDOWN_CUTOFF_FACTOR)/(sourcepitch/PITCHDOWN_FADE_FACTOR-sourcepitch/PITCHDOWN_CUTOFF_FACTOR);
}

void SoundScriptInstance::setGain(float value)
{
	if (startSound) startSound->setGain(value*startSound_pitchgain);
	for (int i=0; i<templ->free_sound; i++)
		if (sounds[i]) sounds[i]->setGain(value*sounds_pitchgain[i]);
	if (stopSound) stopSound->setGain(value*stopSound_pitchgain);
	lastgain=value;
}

void SoundScriptInstance::setPosition(Vector3 pos, Vector3 velocity)
{
	if (startSound)
	{
		startSound->setPosition(pos);
		startSound->setVelocity(velocity);
	}
	for (int i=0; i<templ->free_sound; i++)
	{
		if (sounds[i])
		{
			sounds[i]->setPosition(pos);
			sounds[i]->setVelocity(velocity);
		}
	}
	if (stopSound)
	{
		stopSound->setPosition(pos);
		stopSound->setVelocity(velocity);
	}
}

void SoundScriptInstance::runOnce()
{
	if (startSound)
	{
		if (startSound->isPlaying()) return;
		startSound->play();
	}
	for (int i=0; i<templ->free_sound; i++)
	{
		if (sounds[i])
		{
			if (sounds[i]->isPlaying()) continue;
			sounds[i]->setLoop(false);
			sounds[i]->play();
		}
	}
	if (stopSound)
	{
		if (stopSound->isPlaying()) return;
		stopSound->play();
	}
}

void SoundScriptInstance::start()
{
	if (startSound)
	{
		startSound->stop();
		startSound->play();
	}
	for (int i=0; i<templ->free_sound; i++)
	{
		if (sounds[i])
		{
			sounds[i]->setLoop(true);
			sounds[i]->play();
		}
	}
}

void SoundScriptInstance::stop()
{
	for (int i=0; i<templ->free_sound; i++)
	{
		if (sounds[i]) sounds[i]->stop();
	}
	if (stopSound)
	{
		stopSound->stop();
		stopSound->play();
	}
}
#endif //OPENAL

