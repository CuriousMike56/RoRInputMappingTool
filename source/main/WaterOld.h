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
#ifndef __WaterOld_H__
#define __WaterOld_H__

#define WATER_FULL_QUALITY 0
#define WATER_FULL_SPEED 1
#define WATER_REFLECT 2
#define WATER_BASIC 3

#define WAVEREZ 100

#define MAX_WAVETRAINS 10

#include "water.h"
#include "Ogre.h"
#include "rormemory.h"

//#include "DustPool.h"
using namespace Ogre;

//class DustPool;

extern float mrtime;


typedef struct
{
	float amplitude;
	float maxheight;
	float wavelength;
	float wavespeed;
	float direction;
} wavetrain_t;

class WaterOld : public Water, public MemoryAllocatedObject
{
private:
	Camera *mReflectCam;
	Camera *mRefractCam;
	Camera *mCamera;
	int mType;
	float mScale;
	bool haswaves;
	RenderTexture* rttTex1;
	RenderTexture* rttTex2;
	int framecounter;
	SceneNode *pTestNode;
	SceneNode *pBottomNode;
	float height, orgheight;
	wavetrain_t wavetrains[MAX_WAVETRAINS];
	int free_wavetrain;
	float maxampl;
	HardwareVertexBufferSharedPtr wbuf;
	float *wbuffer;
	float *mapsizex, *mapsizez;
	Ogre::Viewport *vRtt1, *vRtt2;
public:
	WaterOld();
	WaterOld(int type, Camera *camera, SceneManager *mSceneMgr, RenderWindow *mWindow, float wheight, float *mapsizex, float *mapsizez, bool usewaves);
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
	void setHeight(float value);
};



#endif
