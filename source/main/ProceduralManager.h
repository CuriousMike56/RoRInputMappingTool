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

// thomas, 17th of June 2008

#ifndef __ProceduralManager_H__
#define __ProceduralManager_H__


#include <Ogre.h>
#include <vector>
#include "rormemory.h"

class Road2;
class HeightFinder;
class Collisions;


class ProceduralPoint
{
public:
	Ogre::Vector3 position;
	Ogre::Quaternion rotation;
	int type;
	float width;
	float bwidth;
	float bheight;
	int pillartype;
};

class ProceduralObject
{
public:
	ProceduralObject() : loadingState(-1), name(std::string("")), road(0)
	{
	}
	int loadingState;
	std::string name;
	std::vector<ProceduralPoint> points;
	
	// runtime
	Road2 *road;
};

class ProceduralManager : public MemoryAllocatedObject
{
protected:
	std::vector<ProceduralObject> pObjects;
	Ogre::SceneManager *mSceneMgr;
	HeightFinder *hfinder;
	Collisions *collisions;
	int objectcounter;
	
public:
	ProceduralManager(Ogre::SceneManager *manager, HeightFinder *hf, Collisions *collisions);
	~ProceduralManager();
	int addObject(ProceduralObject &po);

	int updateAllObjects();
	int updateObject(ProceduralObject &po);
	
	
	int deleteAllObjects();
	int deleteObject(ProceduralObject &po);

	std::vector<ProceduralObject> &getObjects();
};

#endif
