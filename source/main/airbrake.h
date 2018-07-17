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
#ifndef __Airbrake_H__
#define __Airbrake_H__

#include "Ogre.h"
#include "Beam.h"

using namespace Ogre;


class Airbrake : public MemoryAllocatedObject
{
private:

	typedef struct
	{
		Vector3 vertex;
		Vector3 normal;
	//	Vector3 color;
		Vector2 texcoord;
	} CoVertice_t;

	Ogre::MeshPtr msh;
	SceneNode *snode;
	node_t *noderef;
	node_t *nodex;
	node_t *nodey;
	node_t *nodea;
	Vector3 offset;
	float ratio;
	float maxangle;
	float area;

public:


	Airbrake(SceneManager *manager, char* basename, int num, node_t *ndref, node_t *ndx, node_t *ndy, node_t *nda, Vector3 pos, float width, float length, float maxang, char* texname, float tx1, float tx2, float tx3, float tx4, float lift_coef);

	void updatePosition(float amount);
	void applyForce();
};



#endif
