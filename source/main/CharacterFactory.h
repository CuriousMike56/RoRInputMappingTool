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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 17th of August 2009

#ifndef CHARACTERFACTORY_H__
#define CHARACTERFACTORY_H__

#include "OgrePrerequisites.h"
#include "StreamableFactory.h"
#include <map>

class Collisions;
class Network;
class HeightFinder;
class Water;
class MapControl;

//class Character;
#include "Character.h"

class CharacterFactory : public StreamableFactory < CharacterFactory, Character >, public MemoryAllocatedObject
{
	friend class Network;
public:
	CharacterFactory(Ogre::Camera *cam, Network *net, Collisions *c, HeightFinder *h, Water *w, MapControl *m, Ogre::SceneManager *scm);
	~CharacterFactory();

	Character *createLocal(int playerColour);

	Character *createRemoteInstance(stream_reg_t *reg);
	void setNetwork(Network *net) { this->net = net; };

	void updateCharacters(float dt);
	void updateLabels();
protected:
	Ogre::Camera *cam;
	Collisions *c;
	Network *net;
	HeightFinder *h;
	Water *w;
	MapControl *m;
	Ogre::SceneManager *scm;

	// functions used by friends
	void netUserAttributesChanged(int source, int streamid);
	void localUserAttributesChanged(int newid);
};



#endif //CHARACTERFACTORY_H__
