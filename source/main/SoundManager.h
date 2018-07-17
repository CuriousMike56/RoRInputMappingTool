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
#ifndef __SoundManager_H__
#define __SoundManager_H__

#include "Ogre.h"
using namespace Ogre;

#include "AL/al.h"
#include "AL/alc.h"

#include "pthread.h"
#include "rormemory.h"

//maximum number of really mixed sources
#define MAX_HARDWARE_SOURCES 32
//maximum number of potential sound sources
#define MAX_INPUT_SOURCES 8192
//maximum number of sound files
#define MAX_BUFFERS 2048

#define REASON_PLAY 0
#define REASON_STOP 1
#define REASON_GAIN 2
#define REASON_LOOP 3
#define REASON_PTCH 4
#define REASON_POSN 5
#define REASON_VLCT 6

class SoundManager;

class Sound : public MemoryAllocatedObject
{
public:
	Sound(int input_index, ALuint buffer, SoundManager* sm);
	void setPitch(float pitch);
	void setGain(float gain);
	void setPosition(Vector3 pos);
	void setVelocity(Vector3 vel);
	bool isPlaying();
	void setLoop(bool loop);
	void play();
	void stop();
	void computeAudibility(Vector3 from);
	float audibility;
	//the hardware index, this value is dynamically updated as this input is played or not
	int hardware_index;
	ALuint buffer;
	bool should_play;
	float gain;
	float pitch;
	bool loop;
	Vector3 position;
	Vector3 velocity;
private:
	SoundManager* sm;
	//the input index, this value should not change in the lifetime of the object
	int input_index;
};

class SoundManager : public MemoryAllocatedObject
{
public:
	SoundManager();
	int maxSources();
	void setCamera(Vector3 position, Vector3 direction, Vector3 up, Vector3 velocity);
	void pauseAllSounds();
	void resumeAllSounds();
	Sound* createSound(String filename);

	void recomputeAllSources();
	void recomputeSource(int input_index, int reason, float vfl, Vector3 *vvec);
	float maxDistance;
	float rolloffFactor;
	float referenceDistance;
	//AL hardware sources : this buffer contains valid AL handles up to num_hardware_sources
	ALuint hardware_sources[MAX_HARDWARE_SOURCES];
	//access mutex
	pthread_mutex_t audio_mutex;
private:
	void assign(int input_index, int hardware_index);
	void retire(int input_index);
	int loadWAVFile(String filename, ALuint buffer);


	//input sources: array of Sound
	int free_input_source;
	Sound* input_sources[MAX_INPUT_SOURCES];

	//buffers: array of AL buffers and filenames
	int free_buffer;
	ALuint buffers[MAX_BUFFERS];
	String buffer_filenames[MAX_BUFFERS];

	//total number of hardware sources available. This number is inferior or equal to MAX_HARDWARE_SOURCES
	int num_hardware_sources;
	//number of used hardware sources
	int used_hardware_sources;
	//This gives the input index for each hardware index. Value -1 means unmapped hardware source
	int hardware_sources_input_map[MAX_HARDWARE_SOURCES];

	Vector3 cameraPosition;
	ALCdevice *device;
	ALCcontext *context;

};

#endif

#endif //OPENAL

