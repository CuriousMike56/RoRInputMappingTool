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
#ifndef __Buoyance_H__
#define __Buoyance_H__

#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif
#include <math.h>
#include <stdio.h>
#include "water.h"
//#include "DustPool.h"
#include "Beam.h"

#include "DustPool.h"
#include "DustManager.h"

#define PI 3.1415926535897932384626433832795
using namespace Ogre;

#define BUOY_NORMAL 0
#define BUOY_DRAGONLY 1
#define BUOY_DRAGLESS 2

class Buoyance : public MemoryAllocatedObject
{
private:
	Water *w;
	int update;
	int sink;
	DustPool *splashp, *ripplep;

public:

	Buoyance(Water *water);
	~Buoyance();

	//compute tetrahedron volume
	inline float computeVolume(Vector3 o, Vector3 a, Vector3 b, Vector3 c);

	//compute pressure and drag force on a submerged triangle
	Vector3 computePressureForceSub(Vector3 a, Vector3 b, Vector3 c, Vector3 vel, int type);
	
	//compute pressure and drag forces on a random triangle
	Vector3 computePressureForce(Vector3 a, Vector3 b, Vector3 c, Vector3 vel, int type);
	
	void computeNodeForce(node_t *a, node_t *b, node_t *c, int doupdate, int type);

	void setsink(int v);
};


#endif
