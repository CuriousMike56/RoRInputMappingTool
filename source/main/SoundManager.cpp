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
#include "SoundManager.h"
#include "Settings.h"
#include "pstdint.h"
#include "rormemory.h"

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX


using namespace Ogre;

SoundManager::SoundManager()
{
	device=NULL;
	context=NULL;
	free_input_source=0;
	free_buffer=0;
	num_hardware_sources=0;
	used_hardware_sources=0;
	maxDistance=500.0;
	rolloffFactor=1.0;
	referenceDistance=7.5;
	pthread_mutex_init(&audio_mutex, NULL);

	for (int i=0; i<MAX_HARDWARE_SOURCES; i++) hardware_sources_input_map[i]=-1;

	if (SETTINGS.getSetting("3D Sound renderer") == "No sound") return;

    char str[256];
	if (SETTINGS.getSetting("3D Sound renderer") == "Default") str[0]=0;
	else sprintf(str, "DirectSound Software on %s", SETTINGS.getSetting("3D Sound renderer").c_str());

	LogManager::getSingleton().logMessage("Opening Device: '"+String(str)+"'");

	//we loop alcOpenDevice() because there is a potential race condition with the asynchronous DSound enumeration callback
	for (int i=0; i<100; i++)
	{
		device=alcOpenDevice(str);
		if (device) break; //all right we got it
		else
		{
			ALint error;
			error=alGetError();
			if (error!=ALC_INVALID_VALUE)
			{
				//this is unexpected error
				LogManager::getSingleton().logMessage("Could not create OpenAL device, error code: "+StringConverter::toString(error));
				return;
			} //else just loop
		}
	}
	if (!device) //we looped and we got nothing
	{
		//last ditch attempt with the default sound device, in case the user has a messed-up config file
		device=alcOpenDevice("");
		if (!device)
		{
			LogManager::getSingleton().logMessage("Could not create OpenAL device after many tries.");
			return;
		}
		else
			LogManager::getSingleton().logMessage("Warning: invalid sound device configuration, I will use the default sound source. Run configurator!");
	}
	context=alcCreateContext(device, NULL);
	if (!context)
	{
		ALint error;
		error=alGetError();
		LogManager::getSingleton().logMessage("Could not create OpenAL context, error code: "+StringConverter::toString(error));
		alcCloseDevice(device);
		device=NULL;
		return;
	}
	alcMakeContextCurrent(context);
	//generate the AL sources
	for (num_hardware_sources=0; num_hardware_sources<MAX_HARDWARE_SOURCES; num_hardware_sources++)
	{
		alGetError();
		alGenSources(1, &hardware_sources[num_hardware_sources]);
		if(alGetError()!=AL_NO_ERROR)
			break;
		alSourcef(hardware_sources[num_hardware_sources], AL_REFERENCE_DISTANCE, referenceDistance);
		alSourcef(hardware_sources[num_hardware_sources], AL_ROLLOFF_FACTOR, rolloffFactor);
		alSourcef(hardware_sources[num_hardware_sources], AL_MAX_DISTANCE, maxDistance);
	}
}

Sound* SoundManager::createSound(String filename)
{
	if (!device) return NULL;
	pthread_mutex_lock(&audio_mutex);
	ALuint buffer=0;
	//first, search if we need to create the buffer
	for (int i=0; i<free_buffer; i++)
		if (filename==buffer_filenames[i])
		{
			buffer=buffers[i];
			break;
		}
	if (!buffer)
	{
		//load the file
		if (free_buffer==MAX_BUFFERS)
		{
			pthread_mutex_unlock(&audio_mutex);
			return NULL;
		}
		alGenBuffers(1, &buffers[free_buffer]);
		buffer_filenames[free_buffer]=filename;
		if (loadWAVFile(filename, buffers[free_buffer]))
		{
			//there was an error!
			alDeleteBuffers(1, &buffers[free_buffer]);
			buffer_filenames[free_buffer]=String("");
			pthread_mutex_unlock(&audio_mutex);
			return NULL;
		}
		buffer=buffers[free_buffer];
		free_buffer++;
	}
	if (free_input_source==MAX_INPUT_SOURCES)
	{
		pthread_mutex_unlock(&audio_mutex);
		return NULL;
	}
	input_sources[free_input_source]=new Sound(free_input_source, buffer, this);
	free_input_source++;
	//don't recompute, because sounds do not play automatically
	//recomputeSource(free_input_source-1);
	pthread_mutex_unlock(&audio_mutex);
	return input_sources[free_input_source-1];
}

void SoundManager::setCamera(Ogre::Vector3 position, Ogre::Vector3 direction, Ogre::Vector3 up, Ogre::Vector3 velocity)
{
	if (!device) return;
	pthread_mutex_lock(&audio_mutex);
	cameraPosition=position;
	recomputeAllSources();
	float dir[6];
	//at
	dir[0]=direction.x;
	dir[1]=direction.y;
	dir[2]=direction.z;
	//up
	dir[3]=up.x;
	dir[4]=up.y;
	dir[5]=up.z;
	alListener3f(AL_POSITION, position.x,position.y, position.z);
	alListener3f(AL_VELOCITY, velocity.x,velocity.y,velocity.z);
	alListenerfv(AL_ORIENTATION, dir);
	pthread_mutex_unlock(&audio_mutex);
}

void SoundManager::pauseAllSounds()
{
	if (!device) return;
	//no need for mutex
	alListenerf(AL_GAIN, 0.0);
}

void SoundManager::resumeAllSounds()
{
	if (!device) return;
	//no need for mutex
	alListenerf(AL_GAIN, 1.0);
}

int SoundManager::maxSources()
{
	//no need for mutex
	return num_hardware_sources;
}

void SoundManager::recomputeAllSources()
{
	if (!device) return;
	/*LogManager::getSingleton().logMessage("==========Table dump===================");
	for (int i=0; i<num_hardware_sources; i++)
		LogManager::getSingleton().logMessage("   "+StringConverter::toString(hardware_sources_input_map[i]));
		*/
	//mutex is supposed to be already taken

	//todo: use a high performance selection algorithm
	//http://en.wikipedia.org/wiki/Selection_algorithm

	//this function is called when the camera moves
	//first, compute all audibility
	for (int i=0; i<free_input_source; i++) input_sources[i]->computeAudibility(cameraPosition);
	//next, sort sources by audibility
	//suboptimal algorithm
	//this source index table contains the num_hardware_sources most audible sources
	// it can contain indexes -1 if not enough sources are audible
	int most_audibles[MAX_HARDWARE_SOURCES];
	for (int i=0; i<MAX_HARDWARE_SOURCES; i++) most_audibles[i]=-1;
	for (int i=0; i<num_hardware_sources; i++)
	{
		float maxa=0.0;
		for (int j=0; j<free_input_source; j++)
		{
			if (input_sources[j]->audibility>maxa)
			{
				int k;
				//search in the table if it was not already recorded
				for (k=0; k<i; k++) if (most_audibles[k]==j) break;
				if (k==i)
				{
					//nope, its really a potential hit!
					most_audibles[i]=j;
					maxa=input_sources[j]->audibility;
				}
			}
		}
	}
	//oookay
	//check if we must retire sources first
	for (int i=0; i<free_input_source; i++)
	{
		if (input_sources[i]->hardware_index!=-1)
		{
			//search in the list
			int j;
			for (j=0; j<num_hardware_sources; j++) if (most_audibles[j]==i) break;
			if (j==num_hardware_sources)
			{
				//it was not found!
				retire(i);
			}
		}
	}
	//next, assign new sources
	for (int i=0; i<num_hardware_sources; i++)
	{
		if (most_audibles[i]!=-1)
		{
			if (input_sources[most_audibles[i]]->hardware_index==-1)
			{
				//its not assigned!
				//find a freshly retired hardware buffer: there MUST be at least one!
				int j;
				for (j=0; j<num_hardware_sources; j++) if (hardware_sources_input_map[j]==-1) break;
				assign(most_audibles[i], j);
			}
		}
	}
	//yes, thats it!
}

void SoundManager::recomputeSource(int input_index, int reason, float vfl, Vector3 *vvec)
{
	if (!device) return;
	//mutex is supposed to be already taken

	//something has changed in a source
	input_sources[input_index]->computeAudibility(cameraPosition);
	if (input_sources[input_index]->audibility==0)
	{
		//this is an extinct source, retire if currently playing
		if (input_sources[input_index]->hardware_index!=-1) retire(input_index);
	}
	else
	{
		//this is a potentially audible source
		if (input_sources[input_index]->hardware_index!=-1)
		{
			//source already playing
			//update the AL settings
			switch (reason)
			{
				case REASON_PLAY: alSourcePlay(hardware_sources[input_sources[input_index]->hardware_index]);break;
				case REASON_STOP: alSourceStop(hardware_sources[input_sources[input_index]->hardware_index]);break;
				case REASON_GAIN: alSourcef(hardware_sources[input_sources[input_index]->hardware_index], AL_GAIN, vfl);break;
				case REASON_LOOP: alSourcei(hardware_sources[input_sources[input_index]->hardware_index], AL_LOOPING, (vfl>0.5)?AL_TRUE:AL_FALSE);break;
				case REASON_PTCH: alSourcef(hardware_sources[input_sources[input_index]->hardware_index], AL_PITCH, vfl);break;
				case REASON_POSN: alSource3f(hardware_sources[input_sources[input_index]->hardware_index], AL_POSITION, vvec->x,vvec->y,vvec->z);break;
				case REASON_VLCT: alSource3f(hardware_sources[input_sources[input_index]->hardware_index], AL_VELOCITY, vvec->x,vvec->y,vvec->z);break;
				default:break;
			}
		}
		else
		{
			//try to make it play by the hardware
			//check if there is one free source in the pool
			if (used_hardware_sources<num_hardware_sources)
			{
				//yes!
				for (int i=0; i<num_hardware_sources; i++)
					if (hardware_sources_input_map[i]==-1)
					{
						assign(input_index, i);
						break;
					}
			}
			else
			{
				//no, compute who is the faintest
				//note: we know the table hardware_sources_input_map is full!
				float fv=1.0;
				int al_faintest=0;
				for (int i=0; i<num_hardware_sources; i++)
					if (input_sources[hardware_sources_input_map[i]]->audibility<fv)
					{
						fv=input_sources[hardware_sources_input_map[i]]->audibility;
						al_faintest=i;
					}
				//find if the the faintest source is louder or not
				if (fv<input_sources[input_index]->audibility)
				{
					//this new source is louder than the faintest!
					retire(hardware_sources_input_map[al_faintest]);
					assign(input_index, al_faintest);
				}
				//else this source is too faint, we don't play it!
			}
		}
	}
}

void SoundManager::assign(int input_index, int hardware_index)
{
	if (!device) return;
	//mutex is supposed to be taken
	input_sources[input_index]->hardware_index=hardware_index;
	hardware_sources_input_map[hardware_index]=input_index;
	used_hardware_sources++;

	//the hardware index is supposed to be stopped!
	alSourcei(hardware_sources[hardware_index], AL_BUFFER, input_sources[input_index]->buffer);
	alSourcef(hardware_sources[hardware_index], AL_GAIN, input_sources[input_index]->gain);
	alSourcei(hardware_sources[hardware_index], AL_LOOPING, (input_sources[input_index]->loop)?AL_TRUE:AL_FALSE);
	alSourcef(hardware_sources[hardware_index], AL_PITCH, input_sources[input_index]->pitch);
	alSource3f(hardware_sources[hardware_index], AL_POSITION, input_sources[input_index]->position.x,input_sources[input_index]->position.y,input_sources[input_index]->position.z);
	alSource3f(hardware_sources[hardware_index], AL_VELOCITY, input_sources[input_index]->velocity.x,input_sources[input_index]->velocity.y,input_sources[input_index]->velocity.z);
	if (input_sources[input_index]->should_play)
	{
		alSourcePlay(hardware_sources[hardware_index]);
		//suboptimal, but better than nothing <-does not work
		//if (!input_sources[input_index]->loop) input_sources[input_index]->should_play=false;
	}
}

void SoundManager::retire(int input_index)
{
	if (!device) return;
	//mutex is supposed to be taken
	int hardware_index=input_sources[input_index]->hardware_index;
	if (hardware_index==-1) return;
	alSourceStop(hardware_sources[hardware_index]);
	hardware_sources_input_map[hardware_index]=-1;
	input_sources[input_index]->hardware_index=-1;
	used_hardware_sources--;
}

int SoundManager::loadWAVFile(String filename, ALuint buffer)
{
	if (!device) return -1;
	LogManager::getSingleton().logMessage("Loading WAV file "+filename);
	//creating Stream
	ResourceGroupManager *rgm=ResourceGroupManager::getSingletonPtr();
	String group=rgm->findGroupContainingResource(filename);
	DataStreamPtr stream=rgm->openResource(filename, group);
	//load RIFF/WAVE
	char magic[5];
	magic[4]=0;
	uint32_t lbuf;
	uint16_t sbuf;

	// check magic
	if (stream->read(magic, 4)!=4) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	if (String(magic)!=String("RIFF")) {LogManager::getSingleton().logMessage("Invalid WAV file (no RIFF): "+filename);return -1;}
	//skip filesize
	stream->skip(4);
	// check file format
	if (stream->read(magic, 4)!=4) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	if (String(magic)!=String("WAVE")) {LogManager::getSingleton().logMessage("Invalid WAV file (no WAVE): "+filename);return -1;}
	// check 'fmt ' sub chunk (1)
	if (stream->read(magic, 4)!=4) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	if (String(magic)!=String("fmt ")) {LogManager::getSingleton().logMessage("Invalid WAV file (no fmt): "+filename);return -1;}
	// read (1)'s size
	if (stream->read(&lbuf, 4)!=4) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	unsigned long subChunk1Size=lbuf;
	if (subChunk1Size<16) {LogManager::getSingleton().logMessage("Invalid WAV file (invalid subChunk1Size): "+filename);return -1;}
	// check PCM audio format
	if (stream->read(&sbuf, 2)!=2) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	unsigned short audioFormat=sbuf;
	if (audioFormat!=1) {LogManager::getSingleton().logMessage("Invalid WAV file (invalid audioformat "+StringConverter::toString(audioFormat)+"): "+filename);return -1;}
	// read number of channels
	if (stream->read(&sbuf, 2)!=2) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	unsigned short channels=sbuf;
	// read frequency (sample rate)
	if (stream->read(&lbuf, 4)!=4) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	unsigned long freq=lbuf;
	// skip 6 bytes (Byte rate (4), Block align (2))
	stream->skip(6);
	// read bits per sample
	if (stream->read(&sbuf, 2)!=2) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	unsigned short bps=sbuf;
	// check 'data' sub chunk (2)
	if (stream->read(magic, 4)!=4) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	if (String(magic)!=String("data") && String(magic)!=String("fact")) {LogManager::getSingleton().logMessage("Invalid WAV file (no data/fact): "+filename);return -1;}
	// fact is an option section we don't need to worry about
	if(String(magic)==String("fact"))
	{
		stream->skip(8);
		// Now we shoudl hit the data chunk
		if (stream->read(magic, 4)!=4) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
		if (String(magic)!=String("data")) {LogManager::getSingleton().logMessage("Invalid WAV file (no data): "+filename);return -1;}
	}
	// The next four bytes are the remaining size of the file
	if (stream->read(&lbuf, 4)!=4) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}
	unsigned long dataSize=lbuf;

	int format;
	if (channels==1 && bps==8)
	{
		format=AL_FORMAT_MONO8;
	} else
	if (channels==1 && bps==16)
	{
		format=AL_FORMAT_MONO16;
	} else
	if (channels==2 && bps==8)
	{
		format=AL_FORMAT_STEREO16;
	} else
	if (channels==2 && bps==16)
	{
		format=AL_FORMAT_STEREO16;
	} else
	{
		LogManager::getSingleton().logMessage("Invalid WAV file (wrong channels/bps): "+filename);
		return -1;
	}
	if(channels != 1)
	{
		LogManager::getSingleton().logMessage("Invalid WAV file: the file needs to be mono, and nothing else. Will try to continue anyways ...");
	}
	//okay, creating buffer
	void* bdata=ror_malloc(dataSize);
	if (!bdata) {LogManager::getSingleton().logMessage("Memory error reading file "+filename);return -1;}
	if (stream->read(bdata, dataSize)!=dataSize) {LogManager::getSingleton().logMessage("Could not read file "+filename);return -1;}

//	LogManager::getSingleton().logMessage("alBufferData: format "+StringConverter::toString(format)+" size "+StringConverter::toString(dataSize)+" freq "+StringConverter::toString(freq));
	alGetError(); //reset errors
	ALint error;
	alBufferData(buffer, format, bdata, dataSize, freq);
	error=alGetError();
	if(error!=AL_NO_ERROR)
	{
		LogManager::getSingleton().logMessage("OpenAL error while loading buffer for "+filename+" : "+StringConverter::toString(error));
		ror_free(bdata);
		return -1;
	}

	ror_free(bdata);
	//stream will be closed by itself

	return 0;
}



//-----------------------------------------------------------------------------------

Sound::Sound(int input_index, ALuint buffer, SoundManager *sm)
{
	//mutex is supposed to be taken
	this->sm=sm;
	this->buffer=buffer;
	this->input_index=input_index;
	hardware_index=-1;
	should_play=false;
	gain=1.0;
	pitch=1.0;
	position=Vector3::ZERO;
	velocity=Vector3::ZERO;
	loop=false;
}

void Sound::computeAudibility(Vector3 from)
{
	//mutex is supposed to be taken
	//first check if the sound is finished!
	if (!loop && hardware_index!=-1 && should_play)
	{
		int value;
		alGetSourcei(sm->hardware_sources[hardware_index], AL_SOURCE_STATE, &value);
		if (value!=AL_PLAYING) should_play=false;
	}
	if (!should_play) {audibility=0.0; return;}
	if (gain==0.0) {audibility=0.0; return;}
	float distance=(from-position).length();
	if (distance>sm->maxDistance) {audibility=0.0; return;}
	if (distance<sm->referenceDistance) {audibility=gain; return;}
	audibility=gain*(sm->referenceDistance/(sm->referenceDistance+ (sm->rolloffFactor*(distance-sm->referenceDistance))));
}

bool Sound::isPlaying()
{
	pthread_mutex_lock(&sm->audio_mutex);
	if (hardware_index!=-1)
	{
		int value;
		alGetSourcei(sm->hardware_sources[hardware_index], AL_SOURCE_STATE, &value);
		pthread_mutex_unlock(&sm->audio_mutex);
		return (value==AL_PLAYING);
	}
	pthread_mutex_unlock(&sm->audio_mutex);
	return false;
}

void Sound::play()
{
	pthread_mutex_lock(&sm->audio_mutex);
	should_play=true;
	sm->recomputeSource(input_index, REASON_PLAY, 0, NULL);
	pthread_mutex_unlock(&sm->audio_mutex);
}

void Sound::stop()
{
	pthread_mutex_lock(&sm->audio_mutex);
	should_play=false;
	sm->recomputeSource(input_index, REASON_STOP, 0, NULL);
	pthread_mutex_unlock(&sm->audio_mutex);
}

void Sound::setGain(float gain)
{
	pthread_mutex_lock(&sm->audio_mutex);
	this->gain=gain;
	sm->recomputeSource(input_index, REASON_GAIN, gain, NULL);
	pthread_mutex_unlock(&sm->audio_mutex);
}

void Sound::setLoop(bool loop)
{
	pthread_mutex_lock(&sm->audio_mutex);
	this->loop=loop;
	sm->recomputeSource(input_index, REASON_LOOP, (loop)?1.0:0.0, NULL);
	pthread_mutex_unlock(&sm->audio_mutex);
}

void Sound::setPitch(float pitch)
{
	pthread_mutex_lock(&sm->audio_mutex);
	this->pitch=pitch;
	sm->recomputeSource(input_index, REASON_PTCH, pitch, NULL);
	pthread_mutex_unlock(&sm->audio_mutex);
}

void Sound::setPosition(Ogre::Vector3 pos)
{
	pthread_mutex_lock(&sm->audio_mutex);
	this->position=pos;
	sm->recomputeSource(input_index, REASON_POSN, 0, &pos);
	pthread_mutex_unlock(&sm->audio_mutex);
}

void Sound::setVelocity(Ogre::Vector3 vel)
{
	pthread_mutex_lock(&sm->audio_mutex);
	this->velocity=vel;
	sm->recomputeSource(input_index, REASON_VLCT, 0, &vel);
	pthread_mutex_unlock(&sm->audio_mutex);
}

#endif //OPENAL
