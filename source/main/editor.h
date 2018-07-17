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
#ifndef __Editor_H__
#define __Editor_H__

#include <stdio.h>
#include <math.h>

#include "Ogre.h"
#include "RoRFrameListener.h"
using namespace Ogre;


typedef struct _ObjectType
{
	char name[256];
	SceneNode *node;
	bool classic;
} ObjectType_t;

class Editor : public MemoryAllocatedObject
{
protected:
	SceneManager *mSceneMgr;
	SceneNode *tenode;
	bool classic;
	ObjectType_t rtypes[50];
	int free_rtype;
	int cur_rtype;

public:
	float ppitch;
	float pturn;
	Vector3 ppos;
	char curtype[256];

	Editor(SceneManager *scm, RoRFrameListener *efl);
	void addObjectType(char* name);

	void updatePending();

	void setPos(Vector3 pos);

	void toggleType();

	void dpitch(float v);

	void dturn(float v);

	~Editor();
};

#endif


