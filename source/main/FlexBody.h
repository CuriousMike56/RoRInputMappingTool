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
#ifndef __FlexBody_H__
#define __FlexBody_H__

//body

#include "Ogre.h"
#include "Beam.h"
#include "materialFunctionMapper.h"

using namespace Ogre;

#define MAX_SET_INTERVALS 256

//class Beam;

class FlexBody : public MemoryAllocatedObject
{
private:

	typedef struct
	{
		int from;
		int to;
	} interval_t;

	typedef struct
	{
		int ref;
		int nx;
		int ny;
		int nz;
		Vector3 coords;
	} Locator_t;

	node_t *nodes;
	int numnodes;
	size_t vertex_count;//,index_count;
	Vector3* vertices;
	Vector3* dstpos;
	Vector3* srcnormals;
	Vector3* dstnormals;
	ARGB* srccolors;
//	unsigned* indices;
	Locator_t *locs; //1 loc per vertex

	int cref;
	int cx;
	int cy;
	Vector3 coffset;
	SceneNode *snode;

	interval_t nodeset[MAX_SET_INTERVALS];
	int freenodeset;

	bool hasshared;
	int sharedcount;
	HardwareVertexBufferSharedPtr sharedpbuf;
	HardwareVertexBufferSharedPtr sharednbuf;
	HardwareVertexBufferSharedPtr sharedcbuf;
	int numsubmeshbuf;
	int *submeshnums;
	int *subnodecounts;
	HardwareVertexBufferSharedPtr subpbufs[16];//positions
	HardwareVertexBufferSharedPtr subnbufs[16];//normals
	HardwareVertexBufferSharedPtr subcbufs[16];//colors




	bool haveshadows;
	bool havetangents;
	bool havetexture;
	bool haveblend;

	Ogre::MeshPtr msh;

public:
	FlexBody(SceneManager *manager, node_t *nds, int numnodes, char* meshname, char* uname, int ref, int nx, int ny, Vector3 offset, Quaternion rot, char* setdef, MaterialFunctionMapper *mfm, Skin *usedSkin);

/*	void getMeshInformation(Mesh* mesh,size_t &vertex_count,Vector3* &vertices,
											  size_t &index_count, unsigned* &indices,
											  const Vector3 &position,
											  const Quaternion &orient,const Vector3 &scale);*/

	void addinterval(int from, int to) {nodeset[freenodeset].from=from; nodeset[freenodeset].to=to; freenodeset++;};
	bool isinset(int n);
	void printMeshInfo(Mesh* mesh);
	void setVisible(bool visible);
	Vector3 flexit();
	void reset();
	void updateBlend();
	void writeBlend();
	SceneNode *getSceneNode() { return snode; };
};



#endif
