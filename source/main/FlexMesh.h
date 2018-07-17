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
#ifndef __FlexMesh_H__
#define __FlexMesh_H__

#include "Ogre.h"
#include "Beam.h"
#include "materialFunctionMapper.h"

using namespace Ogre;

class Flexable
{
public:
	virtual Vector3 flexit()=0;
	virtual void setVisible(bool visible) = 0;

};

//class Beam;

class FlexMesh: public Flexable, public MemoryAllocatedObject
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
	SubMesh* subface;
	SubMesh* subband;
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

	size_t bandibufCount;
	size_t faceibufCount;
	unsigned short *facefaces;
	unsigned short *bandfaces;
	node_t *nodes;
	int nbrays;
	SceneManager *smanager;
	bool is_rimmed;
	float rim_ratio;

public:
	FlexMesh(SceneManager *manager, char* name, node_t *nds, int n1, int n2, int nstart, int nrays, char* texface, char* texband, bool rimmed=false, float rimratio=1.0);
	Vector3 updateVertices();
	Vector3 updateShadowVertices();
	Vector3 flexit();
	void setVisible(bool visible);
};



#endif
