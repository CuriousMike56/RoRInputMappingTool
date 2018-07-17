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

#ifndef __SoundScriptManager_H__
#define __SoundScriptManager_H__

#include "Ogre.h"
#include "Beam.h"
#include "OgreScriptLoader.h"
#include "OgreResourceGroupManager.h"
#include "SoundManager.h"
#include "rormemory.h"

#define MAX_SOUNDS_PER_SCRIPT 16
#define MAX_INSTANCES_PER_GROUP 256

#define PITCHDOWN_FADE_FACTOR   3.0
#define PITCHDOWN_CUTOFF_FACTOR 5.0

//list of sound triggers
#define SS_TRIG_NONE		-1
#define SS_TRIG_ENGINE		0
#define SS_TRIG_AEROENGINE1 1
#define SS_TRIG_AEROENGINE2 2
#define SS_TRIG_AEROENGINE3 3
#define SS_TRIG_AEROENGINE4 4
#define SS_TRIG_HORN		5
#define SS_TRIG_BRAKE		6
#define SS_TRIG_PUMP		7
#define SS_TRIG_STARTER		8
#define SS_TRIG_ALWAYSON	9
#define SS_TRIG_REPAIR		10
#define SS_TRIG_AIR			11
#define SS_TRIG_GPWS_APDISCONNECT 12
#define SS_TRIG_GPWS_10		13
#define SS_TRIG_GPWS_20		14
#define SS_TRIG_GPWS_30		15
#define SS_TRIG_GPWS_40		16
#define SS_TRIG_GPWS_50		17
#define SS_TRIG_GPWS_100	18
#define SS_TRIG_GPWS_PULLUP	19
#define SS_TRIG_GPWS_MINIMUMS	20
#define SS_TRIG_AIR_PURGE	21
#define SS_TRIG_SHIFT		22
#define SS_TRIG_GEARSLIDE	23
#define SS_TRIG_CREAK		24
#define SS_TRIG_BREAK		25
#define SS_TRIG_SCREETCH	26
#define SS_TRIG_PARK		27
#define SS_TRIG_AFTERBURNER1 28
#define SS_TRIG_AFTERBURNER2 29
#define SS_TRIG_AFTERBURNER3 30
#define SS_TRIG_AFTERBURNER4 31
#define SS_TRIG_AFTERBURNER5 32
#define SS_TRIG_AFTERBURNER6 33
#define SS_TRIG_AFTERBURNER7 34
#define SS_TRIG_AFTERBURNER8 35
#define SS_TRIG_AEROENGINE5 36
#define SS_TRIG_AEROENGINE6 37
#define SS_TRIG_AEROENGINE7 38
#define SS_TRIG_AEROENGINE8 39
#define SS_TRIG_AOA         40
#define SS_TRIG_IGNITION	41
#define SS_TRIG_REVERSE_GEAR 42
#define SS_TRIG_TURN_SIGNAL  43
#define SS_MAX_TRIG			44

//list of modulation sources
#define SS_MOD_NONE			0
#define SS_MOD_ENGINE		1
#define SS_MOD_TURBO		2
#define SS_MOD_AEROENGINE1  3
#define SS_MOD_AEROENGINE2  4
#define SS_MOD_AEROENGINE3  5
#define SS_MOD_AEROENGINE4  6
#define SS_MOD_WHEELSPEED	7
#define SS_MOD_INJECTOR		8
#define SS_MOD_TORQUE		9
#define SS_MOD_GEARBOX		10
#define SS_MOD_CREAK		11
#define SS_MOD_BREAK		12
#define SS_MOD_SCREETCH		13
#define SS_MOD_PUMP			14
#define SS_MOD_THROTTLE1    15
#define SS_MOD_THROTTLE2    16
#define SS_MOD_THROTTLE3    17
#define SS_MOD_THROTTLE4    18
#define SS_MOD_THROTTLE5    19
#define SS_MOD_THROTTLE6    20
#define SS_MOD_THROTTLE7    21
#define SS_MOD_THROTTLE8    22
#define SS_MOD_AEROENGINE5  23
#define SS_MOD_AEROENGINE6  24
#define SS_MOD_AEROENGINE7  25
#define SS_MOD_AEROENGINE8  26
#define SS_MOD_AIRSPEED     27
#define SS_MOD_AOA          28
#define SS_MAX_MOD			29


using namespace Ogre;

class SoundScriptTemplate : public MemoryAllocatedObject
{
public:
	SoundScriptTemplate(String name, String groupname, String filename);
	bool setParameter(StringVector vec);
	~SoundScriptTemplate();

//private:
	int parseModulation(String str);
	String name;
	String groupname;
	String filename;
	int trigger_source;
	int pitch_source;
	float pitch_offset;
	float pitch_multiplier;
	float pitch_square;
	int gain_source;
	float gain_offset;
	float gain_multiplier;
	float gain_square;
	bool has_start_sound;
	float start_sound_pitch;
	String start_sound_name;
	bool has_stop_sound;
	float stop_sound_pitch;
	String stop_sound_name;
	bool unpitchable;
	int free_sound;
	float sound_pitches[MAX_SOUNDS_PER_SCRIPT];
	String sound_names[MAX_SOUNDS_PER_SCRIPT];
};

class SoundScriptInstance : public MemoryAllocatedObject
{
public:
	SoundScriptInstance(int truck, SoundScriptTemplate* templ, SoundManager* sm, String instancename);
	void setPitch(float value);
	void setGain(float value);
	void setPosition(Vector3 pos, Vector3 velocity);
	void runOnce();
	void start();
	void stop();
	int truck;
	SoundScriptTemplate* templ;
private:
	SoundManager* sm;
	Sound *startSound;
	Sound *stopSound;
	Sound *sounds[MAX_SOUNDS_PER_SCRIPT];
	float startSound_pitchgain;
	float stopSound_pitchgain;
	float sounds_pitchgain[MAX_SOUNDS_PER_SCRIPT];
	float lastgain;
	float pitchgain_cutoff(float sourcepitch, float targetpitch);
};

class SoundScriptManager: public ScriptLoader, public MemoryAllocatedObject
{
public:
	SoundScriptManager();
	static SoundScriptManager *getSingleton();

	//ScriptLoader interface
    const StringVector& getScriptPatterns(void) const;
    void parseScript(DataStreamPtr& stream, const String& groupName);
    Real getLoadingOrder(void) const;

	SoundScriptInstance* createInstance(String templatename, int truck, SceneNode *toAttach);
	void unloadResourceGroup(String groupname);
	void clearTemplates();

	//values update
	void trigOnce(int truck, int trig);
	void trigStart(int truck, int trig);
	void trigStop(int truck, int trig);
	void trigToggle(int truck, int trig);
	bool getTrigState(int truck, int trig);
	void modulate(int truck, int mod, float value);
	void soundEnable(bool state);

	void setCamera(Vector3 position, Vector3 direction, Vector3 up, Vector3 velocity);

	float maxDistance;
	float rolloffFactor;
	float referenceDistance;
private:
    StringVector mScriptPatterns;
	int instance_counter;

	std::map <Ogre::String, SoundScriptTemplate*> templates;

	void skipToNextCloseBrace(DataStreamPtr& chunk);
	void skipToNextOpenBrace(DataStreamPtr& chunk);
	SoundScriptTemplate* createTemplate(String name, String groupname, String filename);
	//instances lookup tables
	int free_trigs[SS_MAX_TRIG];
	SoundScriptInstance* trigs[SS_MAX_TRIG*MAX_INSTANCES_PER_GROUP];
	int free_pitches[SS_MAX_MOD];
	SoundScriptInstance* pitches[SS_MAX_MOD*MAX_INSTANCES_PER_GROUP];
	int free_gains[SS_MAX_MOD];
	SoundScriptInstance* gains[SS_MAX_MOD*MAX_INSTANCES_PER_GROUP];
	//state map
	bool statemap[SS_MAX_TRIG*MAX_TRUCKS];

	SoundManager* sm;

};

#endif

#endif //OPENAL

