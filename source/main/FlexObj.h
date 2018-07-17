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
#ifndef __FlexObj_H__
#define __FlexObj_H__

//body

#include "Ogre.h"
#include "Beam.h"

using namespace Ogre;


//class Beam;

class FlexObj : public MemoryAllocatedObject
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
	SubMesh** subs;
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
	int *nodeIDs;

	size_t ibufCount;
	unsigned short *faces;
	node_t *nodes;
	int nbrays;
	SceneManager *smanager;

	float *sref;
	int triangleCount;

public:


	FlexObj(SceneManager *manager, node_t *nds, int numtexcoords, Vector3* texcoords, int numtriangles, int* triangles, int numsubmeshes, int* subtexindex, int* subtriindex, char* texname, char* name, int* subisback, char* backtexname, char* transtexname);
	~FlexObj();

	//find the zeroed id of the node v in the context of the tidx triangle
	int findID(int tidx, int v, int numsubmeshes, int* subtexindex, int* subtriindex);
	//with normals
	Vector3 updateVertices();
	//with normals
	Vector3 updateShadowVertices();
	Vector3 flexit();
	void scale(float factor);
};



#endif
