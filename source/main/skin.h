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
#ifndef _Skin_H__
#define _Skin_H__

#include "OgrePrerequisites.h"
#include "OgreResource.h"
#include "OgreTexture.h"
#include "OgreMaterial.h"
#include "OgreCommon.h"

class Skin : public Ogre::Resource
{
public:
	/** Constructor.
	@see Resource
	*/
	Skin(Ogre::ResourceManager* creator, const Ogre::String& name, Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual = false, Ogre::ManualResourceLoader* loader = 0);
	virtual ~Skin();

	// we are lazy and wont use separate get/set
	Ogre::String name;
	Ogre::String thumbnail;
	Ogre::String description;
	Ogre::String authorName;
	Ogre::String skintype;
	Ogre::String source;
	Ogre::String sourcetype;
	Ogre::String origin;
	int authorID;
	bool loaded;

	int addMaterialReplace(Ogre::String from, Ogre::String to);
	int hasReplacementForMaterial(Ogre::String material);
	Ogre::String getReplacementForMaterial(Ogre::String material);
	void replaceMeshMaterials(Ogre::Entity *e);

	int serialize(Ogre::String &dst);
	bool operator==(const Skin& other) const;
protected:
	std::map<Ogre::String, Ogre::String> replaceMaterials;

	void loadImpl(void);
	void unloadImpl(void);
	size_t calculateSize(void) const;
};


#endif
