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

#ifndef MATERIALTRUCKMAPPING_H_
#define MATERIALTRUCKMAPPING_H_

#include "Ogre.h"
using namespace Ogre;
#include "rormemory.h"

typedef struct
{
	int type;
	Ogre::String originalmaterial;
	Ogre::String material;
	Ogre::ColourValue emissiveColour;
	bool laststate;
} materialmapping_t;

class MaterialFunctionMapper : public MemoryAllocatedObject
{
public:
	MaterialFunctionMapper();
	~MaterialFunctionMapper();
	void addMaterial(int flareid, materialmapping_t t);
	void toggleFunction(int flareid, bool enabled);
	// this function searchs and replaces materials in meshes
	void replaceMeshMaterials(Ogre::Entity *e);
	static void replaceSimpleMeshMaterials(Ogre::Entity *e, Ogre::ColourValue c = Ogre::ColourValue::White);

protected:
	bool useSSAO;
	static int simpleMaterialCounter;
	void addSSAOToEntity(Ogre::Entity *e);
	Ogre::MaterialPtr addSSAOToMaterial(Ogre::MaterialPtr material);
	std::map <int, std::vector<materialmapping_t> > materialBindings;
};

#endif
