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

#ifndef BEAMFACTORY_H__
#define BEAMFACTORY_H__

#include "OgrePrerequisites.h"
#include "StreamableFactory.h"
#include <map>

class Collisions;
class Network;
class HeightFinder;
class Water;
class MapControl;
class Network;
class RoRFrameListener;

#include "Beam.h"

class BeamFactory : public StreamableFactory < BeamFactory, Beam >, public MemoryAllocatedObject
{
	friend class Network;
	friend class RoRFrameListener;
	public:
	BeamFactory(RoRFrameListener *efl, Beam **trucks, SceneManager *manager, SceneNode *parent, RenderWindow* win, Network *net, float *mapsizex, float *mapsizez, Collisions *icollisions, HeightFinder *mfinder, Water *w, Camera *pcam, Mirrors *mmirror0);
	~BeamFactory();

	Beam *createLocal(int slotid);
	Beam *createLocal(Ogre::Vector3 pos, Ogre::Quaternion rot, Ogre::String fname, collision_box_t *spawnbox=NULL, bool ismachine=false, int flareMode=0, std::vector<Ogre::String> *truckconfig=0, Skin *skin=0, bool freePosition=false);
	int removeBeam(Beam *b);

	Beam *createRemoteInstance(stream_reg_t *reg);

	Beam *getBeam(int source, int streamid); // used by character
protected:
	RoRFrameListener *efl;
	Ogre::SceneManager *manager;
	Ogre::SceneNode *parent;
	Ogre::RenderWindow* win;
	Network *net;
	float *mapsizex, *mapsizez;
	Collisions *icollisions;
	HeightFinder *mfinder;
	Water *w;
	Ogre::Camera *pcam;
	Mirrors *mmirror0;
	Beam **trucks;


	// functions used by friends
	void netUserAttributesChanged(int source, int streamid);
	void localUserAttributesChanged(int newid);


	bool syncRemoteStreams();
	void updateGUI();
};



#endif //BEAMFACTORY_H__
