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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 24th of August 2009

#include "BeamFactory.h"

#include "Ogre.h"
#include "network.h"
#include "utils.h"
#include "sha1.h"
#include "pthread.h"
#include "RoRFrameListener.h"
#include "mirrors.h"

#ifdef USE_MYGUI
#include "gui_mp.h"
#endif  // USE_MYGUI

using namespace Ogre;


template<> BeamFactory *StreamableFactory < BeamFactory, Beam >::ms_Singleton = 0;

BeamFactory::BeamFactory(RoRFrameListener *efl, Beam **trucks, SceneManager *manager, SceneNode *parent, RenderWindow* win, Network *net, float *mapsizex, float *mapsizez, Collisions *icollisions, HeightFinder *mfinder, Water *w, Camera *pcam, Mirrors *mmirror0) : efl(efl), trucks(trucks), manager(manager), parent(parent), win(win), net(net), mapsizex(mapsizex), mapsizez(mapsizez), icollisions(icollisions), mfinder(mfinder), pcam(pcam), mmirror0(mmirror0)
{
}

Beam *BeamFactory::createLocal(int slotid)
{
	// do not use this ...
	return 0;
}

int BeamFactory::removeBeam(Beam *b)
{
	LOCKSTREAMS();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			if(it2->second  == b)
			{

				delete it2->second;
				it2->second = 0;
				it1->second.erase(it2);
				UNLOCKSTREAMS();
				return 0;
			}
		}
	}
	UNLOCKSTREAMS();
	return 1;
}

Beam *BeamFactory::createLocal(Ogre::Vector3 pos, Ogre::Quaternion rot, Ogre::String fname, collision_box_t *spawnbox, bool ismachine, int flareMode, std::vector<Ogre::String> *truckconfig, Skin *skin, bool freePosition)
{
	bool networked=false, networking=false;
	if(net) networking = true;

	int truck_num = efl->getFreeTruckSlot();
	if(truck_num == -1)
	{
		LogManager::getSingleton().logMessage("ERROR: could not add beam to main list");
		return 0;
	}

	Beam *b = new Beam(truck_num,
		manager,
		manager->getRootSceneNode(),
		win,
		net,
		mapsizex,
		mapsizez,
		pos.x,
		pos.y,
		pos.z,
		rot,
		fname.c_str(),
		icollisions,
		mfinder,
		w,
		pcam,
		mmirror0,
		networked,
		networking,
		spawnbox,
		ismachine,
		flareMode,
		truckconfig,
		skin,
		freePosition);

	efl->trucks[truck_num] = b;

	// lock slidenodes after spawning the truck?
	if(b->getSlideNodesLockInstant())
		b->toggleSlideNodeLock(efl->trucks, efl->free_truck, efl->free_truck);

	LOCKSTREAMS();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	streamables[-1][10 + truck_num] = b; // 10 streams offset for beam constructions
	UNLOCKSTREAMS();

	return b;
}

Beam *BeamFactory::createRemoteInstance(stream_reg_t *reg)
{
	// NO LOCKS IN HERE, already locked

	stream_register_trucks_t *treg = (stream_register_trucks_t *)&reg->reg;

	LogManager::getSingleton().logMessage(" new beam truck for " + StringConverter::toString(reg->sourceid) + ":" + StringConverter::toString(reg->streamid));

	bool networked=true, networking=false;
	if(net) networking = true;

	// check if we got this truck installed
	String filename = String(treg->name);
	String group = "";
	if(!CACHE.checkResourceLoaded(filename, group))
	{
		LogManager::getSingleton().logMessage("wont add remote stream (truck not existing): '"+filename+"'");

		// add 0 to the map so we know its stream is existing but not usable for us
		std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
		streamables[reg->sourceid][reg->streamid] = 0;

		return 0;
	}

	// fill truckconfig
	std::vector<Ogre::String> truckconfig;
	for(int i = 0; i < 10; i++)
	{
		if(!strnlen(treg->truckconfig[i], 60))
			break;
		truckconfig.push_back(String(treg->truckconfig[i]));
	}


	// spawn the truck far off anywhere
	Vector3 pos = Vector3(1000000,1000000,1000000);

	int truck_num = efl->getFreeTruckSlot();
	if(truck_num == -1)
	{
		LogManager::getSingleton().logMessage("ERROR: could not add beam to main list");
		return 0;
	}

	Beam *b = new Beam(truck_num,
		manager,
		manager->getRootSceneNode(),
		win,
		net,
		mapsizex,
		mapsizez,
		pos.x,
		pos.y,
		pos.z,
		Quaternion::ZERO,
		reg->reg.name,
		icollisions,
		mfinder,
		w,
		pcam,
		mmirror0,
		networked,
		networking,
		0,
		false,
		0,
		&truckconfig,
		0);

	efl->trucks[truck_num] = b;

	b->setSourceID(reg->sourceid);
	b->setStreamID(reg->streamid);

	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	streamables[reg->sourceid][reg->streamid] = b;

	b->updateNetworkInfo();

	return b;
}

void BeamFactory::localUserAttributesChanged(int newid)
{
	LOCKSTREAMS();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	if(streamables.find(-1) == streamables.end())
	{
		UNLOCKSTREAMS();
		return;
	}

	Beam *b = streamables[-1][0];
	streamables[newid][0] = streamables[-1][0]; // add alias :)
	//b->setUID(newid);
	//b->updateNetLabel();
	UNLOCKSTREAMS();
}

void BeamFactory::netUserAttributesChanged(int source, int streamid)
{
	LOCKSTREAMS();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			if(it1->first == source)
			{
				Beam *b = it2->second;
				if(b) b->updateNetworkInfo();
				break;
			}
		}
	}
	UNLOCKSTREAMS();
}

Beam *BeamFactory::getBeam(int source, int streamid)
{
	LOCKSTREAMS();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for(it1=streamables.begin(); it1!=streamables.end();it1++)
	{
		for(it2=it1->second.begin(); it2!=it1->second.end();it2++)
		{
			if(it1->first == source)
			{
				UNLOCKSTREAMS();
				return it2->second;
			}
		}
	}
	UNLOCKSTREAMS();
	return 0;
}

bool BeamFactory::syncRemoteStreams()
{
	// we override this here, so we know if something changed and could update the player list
	bool changes = StreamableFactory <BeamFactory, Beam>::syncRemoteStreams();
	
	if(changes)
		updateGUI();

	return changes;
}

void BeamFactory::updateGUI()
{
#ifdef USE_MYGUI
#ifdef USE_SOCKETW
	GUI_Multiplayer::getSingleton().update();
#endif // USE_SOCKETW
#endif // USE_MYGUI	
}