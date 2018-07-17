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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 12th of August 2009

#ifndef STREAMABLEFACTORY_H__
#define STREAMABLEFACTORY_H__

#include "OgrePrerequisites.h"
#include "pthread.h"
#ifdef USE_SOCKETW
#include "SocketW.h"
#endif //SOCKETW
#include "rornet.h"
#include "NetworkStreamManager.h"
#include "StreamableFactoryInterface.h"
#include <map>

#include <Ogre.h>

class Network;
class Streamable;

// this is the master swith to debug the stream locking/unlocking
//#define DEBUGSTREAMFACTORIES

#define OGREFUNCTIONSTRING  String(__FUNCTION__)+" @ "+String(__FILE__)+":"+StringConverter::toString(__LINE__)

#ifdef FEAT_DEBUGSTREAMFACTORIES
# define LOCKSTREAMS()       do { LogManager::getSingleton().logMessage("***LOCK:   "+OGREFUNCTIONSTRING); lockStreams();   } while(0)
# define UNLOCKSTREAMS()     do { LogManager::getSingleton().logMessage("***UNLOCK: "+OGREFUNCTIONSTRING); unlockStreams(); } while(0)
# ifdef WIN32
// __debugbreak will break into the debugger in visual studio
#  define MYASSERT(x)       do { if(!x) { LogManager::getSingleton().logMessage("***ASSERT FAILED: "+OGREFUNCTIONSTRING); __debugbreak(); }; } while(0)
# else //!WIN32
#  define MYASSERT(x)       assert(x)
# endif //WIN32
#else //!DEBUGSTREAMFACTORIES
# define LOCKSTREAMS()       lockStreams();
# define UNLOCKSTREAMS()     unlockStreams();
# define MYASSERT(x)         ((void)0)
#endif //DEBUGSTREAMFACTORIES

template<class T, class X> class StreamableFactory : public StreamableFactoryInterface
{
	friend class Network;
public:
	// constructor, destructor and singleton
	StreamableFactory( void ) : locked(false)
	{
		MYASSERT( !ms_Singleton );
		ms_Singleton = static_cast< T* >( this );
		pthread_mutex_init(&stream_reg_mutex, NULL);

		// add self to factory list
		NetworkStreamManager::getSingleton().addFactory(this);
	}

	~StreamableFactory( void )
	{
		MYASSERT( ms_Singleton );
		ms_Singleton = 0;
	}

	static T& getSingleton( void )
	{
		MYASSERT( ms_Singleton );
		return ( *ms_Singleton );
	}

	static T* getSingletonPtr( void )
	{
		return ms_Singleton;
	}

	// useful functions
	virtual X *createLocal(int colour) = 0;
	virtual void netUserAttributesChanged(int source, int streamid) = 0;
	virtual X *createRemoteInstance(stream_reg_t *reg) = 0;

	// common functions
	void createRemote(int sourceid, int streamid, stream_register_t *reg, int colour)
	{
		LOCKSTREAMS();

		stream_reg_t registration;
		registration.sourceid = sourceid;
		registration.streamid = streamid;
		registration.reg      = *reg; // really store the data
		registration.colour   = colour;
		stream_registrations.push_back(registration);

		UNLOCKSTREAMS();
	}

	void deleteRemote(int sourceid, int streamid)
	{
		LOCKSTREAMS();

		stream_del_t deletion;
		deletion.sourceid = sourceid;
		deletion.streamid = streamid;
		stream_deletions.push_back(deletion);

		UNLOCKSTREAMS();
	}

	virtual bool syncRemoteStreams()
	{
		LOCKSTREAMS();
		// first registrations
		int changes = 0;
		while (!stream_registrations.empty())
		{
			stream_reg_t reg = stream_registrations.front();
			Streamable *s = createRemoteInstance(&reg);
			if(s)
			{
				// add it to the streams list
				NetworkStreamManager::getSingleton().addRemoteStream(s, reg.sourceid, reg.streamid);
				reg.reg.status = 1;
			} else
			{
				// error creating stream, tell the sourceid that it failed
				reg.reg.status = -1;
			}
			// fixup the registration information
			reg.reg.origin_sourceid = reg.sourceid;
			reg.reg.origin_streamid = reg.streamid;

			// only save registration results for beam streams
			// TODO: maybe enforce general design to allow all stream types to
			// have a feedback channel
			if(reg.reg.type == 0)
			{
				stream_creation_results.push_back(reg);
			}
			// remove registration from list
			stream_registrations.pop_front();
			changes++;
		}

		// count the stream creation results into the changes
		changes += stream_creation_results.size();

		// then deletions:
		// first registrations
		while (!stream_deletions.empty())
		{
			stream_del_t del = stream_deletions.front();
			removeInstance(&del);
			stream_deletions.pop_front();
			changes++;
		}
		UNLOCKSTREAMS();
		return (changes > 0);
	}

	void removeInstance(stream_del_t *del)
	{
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		for(it1=streamables.begin(); it1!=streamables.end();it1++)
		{
			if(it1->first != del->sourceid) continue;

			for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
			{
				if(del->streamid == -1 || del->streamid == (int)it2->first)
				{
					// deletes the stream
					delete it2->second;
					it2->second = 0;
				}
			}
			break;
		}
	}

	int checkStreamsOK(int sourceid)
	{
		// walk client and the streams and checks for errors
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		int ok = 0;
		int num = 0;
		for(it1=streamables.begin(); it1!=streamables.end();it1++)
		{
			if(it1->first != sourceid) continue;
			for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
			{
				num++;
				if(it2->second != 0)
				{
					ok = 1;
					break;
				}
			}
			break;
		}
		if(!num)
			ok = 2;
		return ok;
	}

	int checkStreamsResultsChanged()
	{
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		for(it1=streamables.begin(); it1!=streamables.end();it1++)
		{
			for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
			{
				if(!it2->second) continue;
				if(it2->second->getStreamResultsChanged())
				{
					return 1;
				}
			}
		}
		return 0;
	}

	int checkStreamsRemoteOK(int sourceid)
	{
		// walk client and the streams and checks for errors
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		int ok = 0;
		int originstreams = 0;
		for(it1=streamables.begin(); it1!=streamables.end();it1++)
		{
			for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
			{
				if(!it2->second)
					continue;
				if(!it2->second->getIsOrigin())
					continue;
				originstreams++;
				stream_register_t reg;
				reg.status = -2;
				int res = it2->second->getStreamRegisterResultForSource(sourceid, &reg);
				if(!res)
				{
					if(reg.status == 1)
						ok = 1;
				}
				break;
			}
			break;
		}
		if(!originstreams)
			ok = 2;
		return ok;
	}

	int clearStreamRegistrationResults()
	{
		LOCKSTREAMS();
		stream_creation_results.clear();
		UNLOCKSTREAMS();
		return 0;
	}

	int getStreamRegistrationResults(std::deque < stream_reg_t > *net_results)
	{
		LOCKSTREAMS();
		// move list entries over to the list in the networking thread
		int res = 0;
		while (!stream_creation_results.empty())
		{
			stream_reg_t reg = stream_creation_results.front();
			net_results->push_back(reg);
			stream_creation_results.pop_front();
			res++;
		}
		UNLOCKSTREAMS();
		return res;
	}

	int addStreamRegistrationResults(int sourceid, stream_register_t *reg)
	{
		typename std::map < int, std::map < unsigned int, X *> > &streamables = getStreams();
		typename std::map < int, std::map < unsigned int, X *> >::iterator it1;
		typename std::map < unsigned int, X *>::iterator it2;

		int res = 0;
		for(it1=streamables.begin(); it1!=streamables.end();it1++)
		{
			for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
			{
				if(!it2->second)
					continue;
				if(!it2->second->getIsOrigin())
					continue;
				int sid = it2->second->getSourceID();
				int stid = it2->second->getStreamID();
				// only use our locally created streams
				if(stid == reg->origin_streamid)
				{
					it2->second->addStreamRegistrationResult(sourceid, *reg);
					if(reg->status == 1)
						Ogre::LogManager::getSingleton().logMessage("Client " + Ogre::StringConverter::toString(sourceid) + " successfully loaded stream " + Ogre::StringConverter::toString(reg->origin_streamid) + " with name '" + reg->name + "', result code: " + Ogre::StringConverter::toString(reg->status));
					else
						Ogre::LogManager::getSingleton().logMessage("Client " + Ogre::StringConverter::toString(sourceid) + " could not load stream " + Ogre::StringConverter::toString(reg->origin_streamid) + " with name '" + reg->name + "', result code: " + Ogre::StringConverter::toString(reg->status));
					res++;
					break;
				}
			}
			break;
		}
		return res;
	}

protected:
	static T* ms_Singleton;
	pthread_mutex_t stream_reg_mutex;
	bool locked;

	std::deque < stream_reg_t > stream_registrations;
	std::deque < stream_del_t > stream_deletions;
	std::deque < stream_reg_t > stream_creation_results;

	std::map < int, std::map < unsigned int, X *> > &getStreams()
	{
		// ensure we only access the map when we locked it before
		MYASSERT(locked);
		return mStreamables;
	}

	void lockStreams()
	{
		// double locking is not healty!
		//MYASSERT(!this->locked);
		pthread_mutex_lock(&stream_reg_mutex);
		this->locked=true;
	}

	void unlockStreams()
	{
		pthread_mutex_unlock(&stream_reg_mutex);
		this->locked=false;
	}

private:
	// no direct access to it, helps with locking it before using it
	std::map < int, std::map < unsigned int, X *> > mStreamables;

};



#endif //STREAMABLEFACTORY_H__
