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
#ifndef __Road2_H__
#define __Road2_H__

#include <stdio.h>
#include <math.h>
#include <vector>

#include "Ogre.h"
using namespace Ogre;

#include "heightfinder.h"
#include "collisions.h"
//dynamic roads

#define MAX_VERTEX 50000
#define MAX_TRIS 50000

#define ROAD_AUTOMATIC 0
#define ROAD_FLAT 1
#define ROAD_LEFT 2
#define ROAD_RIGHT 3
#define ROAD_BOTH 4
#define ROAD_BRIDGE 5
#define ROAD_MONORAIL 6

#define TEXFIT_NONE 0
#define TEXFIT_BRICKWALL 1
#define TEXFIT_ROADS1 2
#define TEXFIT_ROADS2 3
#define TEXFIT_ROAD 4
#define TEXFIT_ROADS3 5
#define TEXFIT_ROADS4 6
#define TEXFIT_CONCRETEWALL 7
#define TEXFIT_CONCRETEWALLI 8
#define TEXFIT_CONCRETETOP 9
#define TEXFIT_CONCRETEUNDER 10

class Road2 : public MemoryAllocatedObject
{
protected:
	typedef struct
	{
		Vector3 vertex;
		Vector3 normal;
		Vector2 texcoord;
	} CoVertice_t;



	MeshPtr msh;
	SubMesh* mainsub;
	SceneManager *smanager;


	Vector3 vertex[MAX_VERTEX];
	Vector2 tex[MAX_VERTEX];
	int vertexcount;
	short tris[MAX_TRIS*3];
	int tricount;

	bool first;
	Vector3 lastpos;
	Quaternion lastrot;
	float lastwidth;
	float lastbheight;
	float lastbwidth;
	int lasttype;
	HeightFinder* hfinder;
	Collisions *coll;
	int mid;
	SceneNode *snode;
	std::vector<int> registeredCollTris;
public:
	Road2(SceneManager *manager, HeightFinder *hf, Collisions *collisions, int id);
	void finish();
	void addBlock(Vector3 pos, Quaternion rot, int type, float width, float bwidth, float bheight, int pillartype=1);
	void computePoints(Vector3 *pts, Vector3 pos, Quaternion rot, int type, float width, float bwidth, float bheight);

	inline Vector3 baseOf(Vector3 p);
	//the two firsts must be the "high" points
	void addQuad(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, int texfit, bool collision, Vector3 pos, Vector3 lastpos, float width, bool flip=false);
	void textureFit(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, int texfit, Vector2 *texc, Vector3 pos, Vector3 lastpos, float width);
	void addCollisionQuad(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, ground_model_t* gm, bool flip=false);
	void createMesh();
	~Road2();
};

#endif


