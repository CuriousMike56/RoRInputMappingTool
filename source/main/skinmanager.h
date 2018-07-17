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
#ifndef _SkinManager_H__
#define _SkinManager_H__

#include "OgrePrerequisites.h"
#include "OgreSingleton.h"
#include "OgreResourceManager.h"
#include "skin.h"

class Cache_Entry;

/** Manages Skin resources, parsing .skin files and generally organising them.*/
class SkinManager : public Ogre::ResourceManager, public Ogre::Singleton< SkinManager >
{
public:
	SkinManager();
	~SkinManager();

	void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName);
	static SkinManager& getSingleton(void);
	static SkinManager* getSingletonPtr(void);

	int getMaterialAlternatives(Ogre::String materialName, std::vector<Skin *> &skin);
	int getUsableSkins(Cache_Entry *e, std::vector<Skin *> &skins);

	int getSkinCount();
	int serialize(Ogre::String &dst);
	int clear();

	void parseAttribute(const Ogre::String& line, Skin *pSkin);
	void unload(const Ogre::String& name);
	void unload(Ogre::ResourceHandle handle);
	void unloadAll(bool reloadableOnly = true);
	void unloadUnreferencedResources(bool reloadableOnly = true);
	void remove(Ogre::ResourcePtr& r);
	void remove(const Ogre::String& name);
	void remove(Ogre::ResourceHandle handle);
	void removeAll(void);
	void reloadAll(bool reloadableOnly = true);
	void reloadUnreferencedResources(bool reloadableOnly = true);
	Ogre::String joinString(Ogre::vector<Ogre::String>::type params, Ogre::String del=" ", int skipNo=1);

protected:

	/// Internal methods
	Ogre::Resource* createImpl(const Ogre::String& name, Ogre::ResourceHandle handle,
		const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
		const Ogre::NameValuePairList* params);

	void logBadAttrib(const Ogre::String& line, Skin *pSkin);


};

#endif
