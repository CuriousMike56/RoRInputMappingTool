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
#ifndef __Road_H__
#define __Road_H__

#include <stdio.h>
#include <math.h>
#include "rormemory.h"

#include "Ogre.h"
using namespace Ogre;

/*typedef struct _RoadElement RoadElement_t;

typedef struct _RoadElement
{
	Vector3 pos;
	Quaternion rot;
	RoadElement_t *next;
} RoadElement_t;
*/

typedef struct _RoadType
{
	char name[256];
	SceneNode *node;
} RoadType_t;

class Road : public MemoryAllocatedObject
{
protected:
//	RoadElement_t *roadlink;
	float ppitch;
	float pturn;
	float lastpturn;
	Vector3 ppos;
	Vector3 protl;
	Vector3 protr;
	SceneManager *mSceneMgr;
//	Entity *te;
	SceneNode *tenode;
	RoadType_t rtypes[10];
	int free_rtype;
	int cur_rtype;

	void preparePending();

	void updatePending();

public:
	Road(SceneManager *scm, Vector3 start);
	void reset(Vector3 start);
	void addRoadType(const char* name);
	void toggleType();
	void dpitch(float v);
	void dturn(float v);
	Vector3 rpos;
	Vector3 rrot;
	char curtype[256];
	void append();
	~Road();
};

#endif


