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

#include "OgreStableHeaders.h"

#include "skinmanager.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreStringVector.h"
#include "OgreException.h"
#include "OgreResourceGroupManager.h"
#include "CacheSystem.h"

using namespace Ogre;

//---------------------------------------------------------------------
template<> SkinManager * Singleton< SkinManager >::ms_Singleton = 0;
SkinManager* SkinManager::getSingletonPtr(void)
{
	return ms_Singleton;
}
SkinManager& SkinManager::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}
//---------------------------------------------------------------------
SkinManager::SkinManager() : ResourceManager()
{
	// Loading order
	mLoadOrder = 200.0f;
	// Scripting is supported by this manager
	mScriptPatterns.push_back("*.skin");
	// Register scripting with resource group manager
	ResourceGroupManager::getSingleton()._registerScriptLoader(this);

	// Resource type
	mResourceType = "RoRVehicleSkins";

	// Register with resource group manager
	ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);


}
//---------------------------------------------------------------------
SkinManager::~SkinManager()
{
	// Unregister with resource group manager
	ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
	// Unegister scripting with resource group manager
	ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);

}
//---------------------------------------------------------------------
Resource* SkinManager::createImpl(const String& name, ResourceHandle handle,
	const String& group, bool isManual, ManualResourceLoader* loader,
	const NameValuePairList* params)
{
	try
	{
		bool existing = (mResources.find(name) != mResources.end());
		if(existing)
			return mResources[name].getPointer();
		else
			return new Skin(this, name, handle, group, isManual, loader);
	} catch(Ogre::ItemIdentityException e)
	{
		return mResources[name].getPointer();
	}
}
//---------------------------------------------------------------------
void SkinManager::parseScript(DataStreamPtr& stream, const String& groupName)
{
	try
	{
		String line;
		Skin *pSkin=0;
		LogManager::getSingleton().logMessage("SkinManager::parseScript");

		while( !stream->eof() )
		{
			line = stream->getLine();
			// Ignore blanks & comments
			if( !line.length() || line.substr( 0, 2 ) == "//" )
			{
				continue;
			}
			else
			{
				if (!pSkin)
				{
					// No current skin
					// So first valid data should be skin name
					pSkin = (Skin *)create(line, groupName).getPointer();
					pSkin->_notifyOrigin(stream->getName());

					// Skip to and over next {
					stream->skipLine("{");
				}
				else
				{
					// Already in skin
					if (line == "}")
					{
						// Finished
						//this->addImpl((Ogre::ResourcePtr)pSkin);
						pSkin=0;
						// NB skin isn't loaded until required
					}
					else
					{
						parseAttribute(line, pSkin);
					}
				}
			}
		}
	} catch(Ogre::ItemIdentityException e)
	{
		// this catches duplicates -> to be ignored
		// this happens since we load the full skin data off the cache, so we dont need
		// to re-add it to the skinmanager
		return;
	}
}

//---------------------------------------------------------------------
String SkinManager::joinString(Ogre::vector<String>::type params, String del, int skipNo)
{
	String res="";
	int i=0;
	for(Ogre::vector<String>::type::iterator it=params.begin(); it!=params.end(); it++, i++)
	{
		if(i < skipNo) continue;
		res += *it;
		if(i) res += del;
	}
	return res;
}
//---------------------------------------------------------------------
void SkinManager::parseAttribute(const String& line, Skin *pSkin)
{
	Ogre::vector<String>::type params = StringUtil::split(line, "\x09\x0a\x20\x3d"); // 0x9 = tab, 0xa = \n, 0x20 = space, 0x3d = '='
	String& attrib = params[0];
	StringUtil::toLowerCase(attrib);

	if      (attrib == "replacematerial"    && params.size() == 3) pSkin->addMaterialReplace(params[1], params[2]);
	else if (attrib == "thumbnail"          && params.size() >= 2) pSkin->thumbnail = joinString(params);
	else if (attrib == "description"        && params.size() >= 2) pSkin->description = joinString(params);
	else if (attrib == "authorname"         && params.size() >= 2) pSkin->authorName = joinString(params);
	else if (attrib == "authorid"           && params.size() == 2) pSkin->authorID = StringConverter::parseInt(params[1]);
	else if (attrib == "skintype"           && params.size() >= 2) pSkin->skintype = joinString(params);
	else if (attrib == "name"               && params.size() >= 2)
	{
		pSkin->name = joinString(params);
		StringUtil::trim(pSkin->name);
	}
	else if (attrib == "origin"             && params.size() >= 2) pSkin->origin = joinString(params);
	else if (attrib == "source"             && params.size() >= 2) pSkin->source = joinString(params);
	else if (attrib == "sourcetype"         && params.size() == 2) pSkin->sourcetype = params[1];
}

//---------------------------------------------------------------------
void SkinManager::logBadAttrib(const String& line, Skin *pSkin)
{
	LogManager::getSingleton().logMessage("Bad attribute line: " + line +
		" in skin " + pSkin->getName());

}

int SkinManager::getMaterialAlternatives(Ogre::String materialName, std::vector<Skin *> &skinVector)
{
	Ogre::ResourceManager::ResourceMapIterator it = SkinManager::getSingleton().getResourceIterator();
	while (it.hasMoreElements())
	{
		Skin *skin = (Skin *)it.getNext().getPointer();

		if (skin->hasReplacementForMaterial(materialName))
		{
			skinVector.push_back(skin);
		}
	}
	return 0; // no errors
}


int SkinManager::getUsableSkins(Cache_Entry *e, std::vector<Skin *> &skins)
{
	Ogre::ResourceManager::ResourceMapIterator it = SkinManager::getSingleton().getResourceIterator();
	while (it.hasMoreElements())
	{
		Skin *skin = (Skin *)it.getNext().getPointer();
		std::set < Ogre::String >::iterator mit;
		for(mit = e->materials.begin(); mit != e->materials.end(); mit++)
		{
			if(skin->hasReplacementForMaterial(*mit))
			{
				bool found=false;
				for(std::vector<Skin *>::iterator sit = skins.begin(); sit != skins.end(); sit++)
				{
					if(*sit == skin) // operator== implemented
					{
						found=true;
						break;
					}
				}
				if(found)
					// already in there
					continue;
				skins.push_back(skin);
			}
		}
	}
	return 0; // no errors
}

int SkinManager::getSkinCount()
{
	return (int)mResourcesByHandle.size();
}

int SkinManager::serialize(Ogre::String &dst)
{
	ResourceManager::ResourceHandleMap::iterator it;
	for(it = mResourcesByHandle.begin(); it != mResourcesByHandle.end(); it++)
	{

		String source = CACHE.getSkinSource(it->second->getOrigin());
		if(!source.size()) continue;

		std::string::size_type loc = source.find(".zip", 0);
		if( loc != std::string::npos )
			((Skin *)(it->second.getPointer()))->sourcetype = "Zip";
		else
			((Skin *)(it->second.getPointer()))->sourcetype = "FileSystem";

		dst += "skin " + ((Skin *)(it->second.getPointer()))->name + "\n{\n";
		int sres = ((Skin *)(it->second.getPointer()))->serialize(dst);
		dst += "\torigin=" + it->second->getOrigin() + "\n";
		dst += "\tsource=" + source + "\n";
		dst += "}\n\n";
	}
	return 0;
}

int SkinManager::clear()
{
	mResourcesByHandle.clear();
	return 0;
}

//we wont unload skins once loaded!
void SkinManager::unload(const String& name) {}
void SkinManager::unload(ResourceHandle handle) {}
void SkinManager::unloadAll(bool reloadableOnly) {}
void SkinManager::unloadUnreferencedResources(bool reloadableOnly) {}
void SkinManager::remove(ResourcePtr& r) {}
void SkinManager::remove(const String& name) {}
void SkinManager::remove(ResourceHandle handle) {}
void SkinManager::removeAll(void) {}
void SkinManager::reloadAll(bool reloadableOnly) {}
void SkinManager::reloadUnreferencedResources(bool reloadableOnly) {}
