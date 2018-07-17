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
// Thomas Fischer, 23 of April 2008

#ifdef USE_HYDRAX

#ifndef HYDRAXWATER_H_
#define HYDRAXWATER_H_

// this is a workaround for fixing compilation/linking issues in conjunction with Caelum
#undef DllExport
#include "Hydrax/Prerequisites.h"
#include "Hydrax/Hydrax.h"
#include "Hydrax/MaterialManager.h"
#include "Hydrax/Noise/Noise.h"
#include "Hydrax/Noise/Perlin/Perlin.h"
#include "Hydrax/Modules/ProjectedGrid/ProjectedGrid.h"
#include "Hydrax/Modules/SimpleGrid/SimpleGrid.h"
#undef DllExport

#include "Ogre.h"
#include "water.h"
#include "rormemory.h"
using namespace Ogre;

class HydraxWater : public Water, public MemoryAllocatedObject
{
protected:
	Hydrax::Hydrax *mHydrax;
	Hydrax::Noise::Noise *waternoise;
	Ogre::Camera *mCamera;
	float wheight;
	float waveStrength;

public:
	HydraxWater(int type, Camera *camera, SceneManager *mSceneMgr, RenderWindow *mWindow, float wheight, float *mapsizex, float *mapsizez, bool usewaves);
	void moveTo(Camera *cam, float centerheight);
	void showWave(Vector3 refpos);
	void update();
	void prepareShutdown();
	float getHeight();
	bool visible;
	void setVisible(bool value);
	float getHeightWaves(Vector3 pos);
	Vector3 getVelocity(Vector3 pos);
	void updateReflectionPlane(float h);
	void setFadeColour(ColourValue ambient);
	void setSunPosition(Ogre::Vector3);
	void framestep(float dt);
	bool allowUnderWater();
	int loadConfig(Ogre::String configfile);
	void setHeight(float value);
};



#endif
#endif //HYDRAX
