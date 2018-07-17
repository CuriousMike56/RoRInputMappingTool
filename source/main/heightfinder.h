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
#ifndef __HeightFinder_H__
#define __HeightFinder_H__

#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include "rormemory.h"

#include "OgreTerrain.h"
#include "OgreTerrainGroup.h"
#include "DotSceneLoader.h"

//using namespace Ogre;
/**
 * This is the common Interface for all Scenemanager Specific Implementations of the Heightfinder
 */
class HeightFinder
{
public:
	HeightFinder() {};
	virtual ~HeightFinder() {};

	virtual float getHeightAt(float x, float z) = 0;
	virtual Ogre::Vector3 getNormalAt(float x, float y, float z, float precision=0.1)
	{
		Ogre::Vector3 left(-precision, getHeightAt( x - precision, z ) - y, 0.0f);
		Ogre::Vector3 down( 0.0f, getHeightAt( x, z + precision ) - y, precision);
		down = left.crossProduct( down );
		down.normalise();
		return down;
	}
};

// new terrain height finder. For the new terrain from Ogre 1.7
class NTHeightFinder : public HeightFinder, public MemoryAllocatedObject
{
protected:
	Ogre::TerrainGroup *mTerrainGroup;
	Ogre::Vector3 mTerrainPos;
public:
	NTHeightFinder(Ogre::TerrainGroup *tg, Ogre::Vector3 tp) : mTerrainGroup(tg), mTerrainPos(tp)
	{
	}
	
	~NTHeightFinder()
	{
	}

	float getHeightAt(float x, float z)
	{
		return mTerrainGroup->getHeightAtWorldPosition(x, 1000, z);
	}
};

// new terrain height finder adapted to Ogitor scene loading
class OgitorSceneHeightFinder : public HeightFinder, public MemoryAllocatedObject
{
protected:
	DotSceneLoader *mLoader;
public:
	OgitorSceneHeightFinder(DotSceneLoader *loader) : mLoader(loader)
	{
	}
	
	~OgitorSceneHeightFinder()
	{
	}

	float getHeightAt(float x, float z)
	{
		return mLoader->getTerrainGroup()->getHeightAtWorldPosition(x, 1000, z);
	}
};



// Scene-Manager Specific implementations

/**
 * Heightfinder for the standart Ogre Terrain Mnager
 */
class TSMHeightFinder : public HeightFinder, public MemoryAllocatedObject
{
protected:
	Ogre::Vector3 scale;
	Ogre::Vector3 inverse_scale;
	int size;
	int size1;
	char cfgfilename[256];
	unsigned short *data;
	float defaulth;
	float dx, dz;
	bool flipped;
	void loadSettings();

public:
	TSMHeightFinder(char *cfgfilename, char *fname, float defaultheight);
	~TSMHeightFinder();

	float getHeightAt(float x, float z);

};

#endif
