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
#ifndef __DustPool_H__
#define __DustPool_H__

#include <stdio.h>
#include <math.h>

#include "Ogre.h"
//#include "OgreDeflectorPlaneAffector.h"

class Water;

using namespace Ogre;

#define MAX_DUSTS 100

#define DUST_NORMAL 0
#define DUST_RUBBER 1
#define DUST_DRIP   2
#define DUST_VAPOUR   3
#define DUST_SPLASH 4
#define DUST_RIPPLE 5
#define DUST_SPARKS 6
#define DUST_CLUMP 7

class DustPool
{
protected:
	ParticleSystem* pss[MAX_DUSTS];
    SceneNode *sns[MAX_DUSTS];
	int size;
	int allocated;
	Vector3 positions[MAX_DUSTS];
	Vector3 velocities[MAX_DUSTS];
	ColourValue colours[MAX_DUSTS];
	int types[MAX_DUSTS];
	float rates[MAX_DUSTS];
	Water* w;

public:
	DustPool(char* dname, int dsize, SceneNode *parent, SceneManager *smgr, Water *mw);

	void setVisible(bool s);
	//Dust
	void alloc(Vector3 pos, Vector3 vel, ColourValue col=ColourValue(0.83, 0.71, 0.64, 1.0));
	//clumps
	void allocClump(Vector3 pos, Vector3 vel, ColourValue col=ColourValue(0.83, 0.71, 0.64, 1.0));
	//Rubber smoke
	void allocSmoke(Vector3 pos, Vector3 vel);
	//
	void allocSparks(Vector3 pos, Vector3 vel);
	//Water vapour
	void allocVapour(Vector3 pos, Vector3 vel, float time);

	void allocDrip(Vector3 pos, Vector3 vel, float time);

	void allocSplash(Vector3 pos, Vector3 vel);

	void allocRipple(Vector3 pos, Vector3 vel);

	void update(float gspeed);
	void setWater(Water *_w) { w = _w; };
	~DustPool();
};

extern int numdust;
extern DustPool* dusts[10];


#endif


