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
#ifndef __FlexAirfoil_H__
#define __FlexAirfoil_H__

#include <math.h>
#include "Ogre.h"
#include "Beam.h"
#include "Airfoil.h"
#include "aeroengine.h"
class Aeroengine;

using namespace Ogre;


class FlexAirfoil : public MemoryAllocatedObject
{
private:
	float airfoilpos[90];
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

	SubMesh* subcup;
	SubMesh* subcdn;

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


	size_t faceibufCount;
	size_t bandibufCount;
	size_t cupibufCount;
	size_t cdnibufCount;
	unsigned short *facefaces;
	unsigned short *bandfaces;
	unsigned short *cupfaces;
	unsigned short *cdnfaces;
	node_t *nodes;
	SceneManager *smanager;


	float sref;

	float deflection;
	float chordratio;
	bool hascontrol;
	bool isstabilator;
	bool stabilleft;
	float lratio;
	float rratio;
	float mindef;
	float maxdef;
	float thickness;
	bool useInducedDrag;
	float idSpan;
	float idArea;
	bool idLeft;

	Airfoil *airfoil;
	AeroEngine **aeroengines;
	int free_wash;
	int washpropnum[MAX_AEROENGINES];
	float washpropratio[MAX_AEROENGINES];

public:
	float aoa;
	char type;
	int nfld;
	int nfrd;
	int nflu;
	int nfru;
	int nbld;
	int nbrd;
	int nblu;
	int nbru;

//	int innan;
	bool broken;
	bool breakable;
	float liftcoef;

	char debug[256];

	FlexAirfoil(SceneManager *manager, char* name, node_t *nds, int pnfld, int pnfrd, int pnflu, int pnfru, int pnbld, int pnbrd, int pnblu, int pnbru, char* texband, Vector2 texlf, Vector2 texrf, Vector2 texlb, Vector2 texrb, char mtype, float controlratio, float mind, float maxd, char* afname, float lift_coef, AeroEngine** tps, bool break_able);
	~FlexAirfoil();

	Vector3 updateVertices();
	Vector3 updateShadowVertices();
	void setControlDeflection(float val);

	Vector3 flexit();

	void enableInducedDrag(float span, float area, bool l);

	void addwash(int propid, float ratio);

	void updateForces();
};


#endif
