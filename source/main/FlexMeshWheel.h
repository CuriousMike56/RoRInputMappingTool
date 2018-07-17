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
#ifndef __FlexMeshWheel_H__
#define __FlexMeshWheel_H__

//wheels only!

#include "Ogre.h"
#include "Beam.h"
#include "FlexMesh.h"
#include "materialFunctionMapper.h"

using namespace Ogre;

#define NLIMIT 65536

//class Beam;

class FlexMeshWheel: public Flexable, public MemoryAllocatedObject
{
private:
typedef struct
{
	Vector3 vertex;
	Vector3 normal;
//	Vector3 color;
	Vector2 texcoord;
} CoVertice_t;

typedef struct
{
	Vector3 vertex;
} posVertice_t;

typedef struct
{
	Vector3 normal;
//	Vector3 color;
	Vector2 texcoord;
} norVertice_t;

	Ogre::MeshPtr msh;
	SubMesh* sub;
	VertexDeclaration* decl;
	HardwareVertexBufferSharedPtr vbuf;

	size_t nVertices;
	size_t vbufCount;
	//shadow
	union
	{
	float *shadowposvertices;
	posVertice_t *coshadowposvertices;
	};
	union
	{
	float *shadownorvertices;
	norVertice_t *coshadownorvertices;
	};
	union
	{
	float *vertices;
	CoVertice_t *covertices;
	};
	//nodes
	//int *nodeIDs;
	int id0;
	int id1;
	int idstart;

	size_t ibufCount;
	unsigned short *faces;
	node_t *nodes;
	int nbrays;
	SceneManager *smanager;
	float rim_radius;
	SceneNode *rnode;
	float normy;
	bool revrim;
	Entity *rimEnt;
public:
	FlexMeshWheel(SceneManager *manager, char* name, node_t *nds, int n1, int n2, int nstart, int nrays, char* meshname, char* texband, float rimradius, bool rimreverse, MaterialFunctionMapper *mfm, Skin *usedSkin);

	Vector3 updateVertices();
	Vector3 updateShadowVertices();
	Vector3 flexit();
	Entity *getRimEntity() { return rimEnt; };
	void setVisible(bool visible);
};



#endif
