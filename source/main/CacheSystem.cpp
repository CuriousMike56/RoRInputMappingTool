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
// created on 21th of May 2008 by Thomas

#include "CacheSystem.h"
#include "sha1.h"
#include "ImprovedConfigFile.h"
#include "skinmanager.h"
#include "Settings.h"
#include "language.h"
#include "rormemory.h"
#include "SoundScriptManager.h"
#include "BeamData.h" // for authorinfo_t

#ifdef USE_MYGUI 
#include "LoadingWindow.h"
#endif //MYGUI

using namespace RoR; //CSHA1

// singleton pattern
CacheSystem* CacheSystem::myInstance = 0;

CacheSystem &CacheSystem::Instance ()
{
	if (myInstance == 0)
		myInstance = new CacheSystem;
	return *myInstance;
}

CacheSystem::CacheSystem()
{
	rgcounter = 0;
	changedFiles = 0;
	newFiles = 0;
	deletedFiles = 0;
	smgr = 0;

	// register the extensions
	known_extensions.push_back("machine");
	known_extensions.push_back("fixed");
	known_extensions.push_back("terrn");
	known_extensions.push_back("truck");
	known_extensions.push_back("car");
	known_extensions.push_back("boat");
	known_extensions.push_back("airplane");
	known_extensions.push_back("trailer");
	known_extensions.push_back("load");
	// skins:
	known_extensions.push_back("skin");

	if(SETTINGS.getSetting("streamCacheGenerationOnly") == "Yes")
	{
		writeStreamCache();
		exit(0);
	}
}

CacheSystem::~CacheSystem()
{
}

void CacheSystem::setLocation(String cachepath, String configpath)
{
	location=cachepath;
	configlocation=configpath;
}

void CacheSystem::startup(SceneManager *smgr, bool forcecheck)
{
	this->smgr = smgr;

	// read valid categories from file
	readCategoryTitles();

	// calculate sha1 over all the content
	currentSHA1 = filenamesSHA1();

	int valid = isCacheValid();
	if(forcecheck)
		valid = -1;
	// valid == -1 : try incemental update
	// valid == -2 : must update fully
	if(valid<0)
	{
		LogManager::getSingleton().logMessage("cache invalid, updating ...");
		// generate the cache
		generateCache((valid < -1));

		LogManager::getSingleton().logMessage("unloading unused resource groups ...");
		// important: unload everything again!
		//unloadUselessResourceGroups();
	}

	LogManager::getSingleton().logMessage("loading cache...");
	// load the cache finally!
	loadCache();

	LogManager::getSingleton().logMessage("cache loaded!");
}

std::map<int, Category_Entry> *CacheSystem::getCategories()
{
	return &categories;
}

std::vector<Cache_Entry> *CacheSystem::getEntries()
{
	return &entries;
}


void CacheSystem::unloadUselessResourceGroups()
{
	StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
	StringVector::iterator it;
	for(it = sv.begin(); it!=sv.end(); it++)
	{
		if(it->substr(0, 8) == "General-")
		{
			try
			{
#ifdef USE_OPENAL
				SoundScriptManager *ssm = SoundScriptManager::getSingleton();
				if(ssm) ssm->clearTemplates();
#endif //OPENAL
				// we cannot fix this problem below Ogre version 1.7
				//ParticleSystemManager::getSingleton().removeTemplatesByResourceGroup(*it);
				ResourceGroupManager::getSingleton().clearResourceGroup(*it);
				ResourceGroupManager::getSingleton().unloadResourceGroup(*it);
				ResourceGroupManager::getSingleton().destroyResourceGroup(*it);
			} catch(Ogre::Exception& e)
			{
				LogManager::getSingleton().logMessage("error while unloading resource groups: " + e.getFullDescription());
				LogManager::getSingleton().logMessage("trying to continue ...");
			}
		}
	}
}


String CacheSystem::getCacheConfigFilename(bool full)
{
	if (full) return location+String(CACHE_FILE);
	return String(CACHE_FILE);
}

// we implement this on our own, since we cannot reply on the ogre version
bool CacheSystem::resourceExistsInAllGroups(Ogre::String filename)
{
	String group;
	bool exists=true;
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
	}catch(...)
	{
		exists=false;
	}
	if(group.empty())
		exists = false;
	return exists;
}

int CacheSystem::isCacheValid()
{
	String cfgfilename = getCacheConfigFilename(false);
	ImprovedConfigFile cfg;
	ConfigFile ff;
	if(!resourceExistsInAllGroups(cfgfilename))
	{
		LogManager::getSingleton().logMessage("unable to load config file: "+cfgfilename);
		return -2;
	}

	String group = ResourceGroupManager::getSingleton().findGroupContainingResource(cfgfilename);
	DataStreamPtr stream=ResourceGroupManager::getSingleton().openResource(cfgfilename, group);
	cfg.load(stream, "\t:=", false);
	String shaone = cfg.getSetting("shaone");
	String cacheformat = cfg.getSetting("cacheformat");

	if(shaone == "" || shaone != currentSHA1)
	{
		LogManager::getSingleton().logMessage("* mod cache is invalid (not up to date), regenerating new one ...");
		return -1;
	}
	if(cacheformat != String(CACHE_FILE_FORMAT))
	{
		entries.clear();
		LogManager::getSingleton().logMessage("* mod cache has invalid format, trying to regenerate");
		return -1;
	}
	LogManager::getSingleton().logMessage("* mod cache is valid, using it.");
	return 0;
}


void CacheSystem::logBadTruckAttrib(const String& line, Cache_Entry& t)
{
	LogManager::getSingleton().logMessage("Bad Mod attribute line: " + line + " in mod " + t.dname);
}

void CacheSystem::parseModAttribute(const String& line, Cache_Entry& t)
{
	Ogre::vector<String>::type params = StringUtil::split(line, "\x09\x0a=,");
	String& attrib = params[0];
	StringUtil::toLowerCase(attrib);
	if (attrib == "number")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.number = StringConverter::parseInt(params[1]);
	}
	else if (attrib == "deleted")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.deleted = StringConverter::parseBool(params[1]);
	}
	else if (attrib == "usagecounter")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.usagecounter = StringConverter::parseInt(params[1]);
	}
	else if (attrib == "addtimestamp")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.addtimestamp = StringConverter::parseInt(params[1]);
	}
	else if (attrib == "minitype")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.minitype = params[1];
	}
	else if (attrib == "type")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.type = params[1];
	}
	else if (attrib == "dirname")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.dirname = params[1];
	}
	else if (attrib == "filecachename")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.filecachename = params[1];
	}
	else if (attrib == "fname")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.fname = params[1];
	}
	else if (attrib == "fname_without_uid")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.fname_without_uid = params[1];
	}
	else if (attrib == "fext")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.fext = params[1];
	}
	else if (attrib == "filetime")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.filetime = params[1];
	}
	else if (attrib == "dname")
	{
		// Check params
		if (params.size() < 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.dname = params[1];
	}
	else if (attrib == "hash")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.hash = params[1];
	}
	else if (attrib == "categoryid")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.categoryid = StringConverter::parseInt(params[1]);
		category_usage[t.categoryid] = category_usage[t.categoryid] + 1;
		t.categoryname=categories[t.categoryid].title;
	}
	else if (attrib == "uniqueid")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.uniqueid = params[1];
	}
	else if (attrib == "version")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.version = StringConverter::parseInt(params[1]);
	}
	/*
	else if (attrib == "numauthors")
	{
		// Check params
		if (params.size() != 1)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		t.authors.resize(StringConverter::parseInt(params[1]))
	}
	*/
	else if (attrib == "author")
	{
		// Check params
		if (params.size() < 5)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		// Set
		authorinfo_t *ai = new authorinfo_t();
		ai->id = StringConverter::parseInt(params[2]);
		strncpy(ai->type, params[1].c_str(), 255);
		strncpy(ai->name, params[3].c_str(), 255);
		strncpy(ai->email, params[4].c_str(), 255);
		t.authors.push_back(ai);
	}
	else if (attrib == "sectionconfig")
	{
		// Check params
		if (params.size() < 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		t.sectionconfigs.push_back(params[1]);
	}

	// TRUCK detail parameters below
	else if (attrib == "description")
		if (params.size() < 2) { logBadTruckAttrib(line, t); return; } else  t.description = deNormalizeText(params[1]);
	else if (attrib == "tags")
		if (params.size() < 2) { logBadTruckAttrib(line, t); return; } else  t.tags = params[1];
	else if (attrib == "fileformatversion")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.fileformatversion = StringConverter::parseInt(params[1]);
	else if (attrib == "hasSubmeshs")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.hasSubmeshs = StringConverter::parseBool(params[1]);
	else if (attrib == "nodecount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.nodecount = StringConverter::parseInt(params[1]);
	else if (attrib == "beamcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.beamcount = StringConverter::parseInt(params[1]);
	else if (attrib == "shockcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.shockcount = StringConverter::parseInt(params[1]);
	else if (attrib == "fixescount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.fixescount = StringConverter::parseInt(params[1]);
	else if (attrib == "hydroscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.hydroscount = StringConverter::parseInt(params[1]);
	else if (attrib == "wheelcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.wheelcount = StringConverter::parseInt(params[1]);
	else if (attrib == "propwheelcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.propwheelcount = StringConverter::parseInt(params[1]);
	else if (attrib == "commandscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.commandscount = StringConverter::parseInt(params[1]);
	else if (attrib == "flarescount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.flarescount = StringConverter::parseInt(params[1]);
	else if (attrib == "propscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.propscount = StringConverter::parseInt(params[1]);
	else if (attrib == "wingscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.wingscount = StringConverter::parseInt(params[1]);
	else if (attrib == "turbopropscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.turbopropscount = StringConverter::parseInt(params[1]);
	else if (attrib == "turbojetcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.turbojetcount = StringConverter::parseInt(params[1]);
	else if (attrib == "rotatorscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.rotatorscount = StringConverter::parseInt(params[1]);
	else if (attrib == "exhaustscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.exhaustscount = StringConverter::parseInt(params[1]);
	else if (attrib == "flexbodiescount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.flexbodiescount = StringConverter::parseInt(params[1]);
	else if (attrib == "materialflarebindingscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.materialflarebindingscount = StringConverter::parseInt(params[1]);
	else if (attrib == "soundsourcescount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.soundsourcescount = StringConverter::parseInt(params[1]);
	else if (attrib == "managedmaterialscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.managedmaterialscount = StringConverter::parseInt(params[1]);
	else if (attrib == "truckmass")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.truckmass = StringConverter::parseReal(params[1]);
	else if (attrib == "loadmass")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.loadmass = StringConverter::parseReal(params[1]);
	else if (attrib == "minrpm")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.minrpm = StringConverter::parseReal(params[1]);
	else if (attrib == "maxrpm")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.maxrpm = StringConverter::parseReal(params[1]);
	else if (attrib == "torque")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.torque = StringConverter::parseReal(params[1]);
	else if (attrib == "customtach")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.customtach = StringConverter::parseBool(params[1]);
	else if (attrib == "custom_particles")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.custom_particles = StringConverter::parseBool(params[1]);
	else if (attrib == "forwardcommands")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.forwardcommands = StringConverter::parseBool(params[1]);
	else if (attrib == "rollon")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.rollon = StringConverter::parseBool(params[1]);
	else if (attrib == "rescuer")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.rescuer = StringConverter::parseBool(params[1]);
	else if (attrib == "driveable")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.driveable = StringConverter::parseInt(params[1]);
	else if (attrib == "numgears")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.numgears = StringConverter::parseInt(params[1]);
	else if (attrib == "enginetype")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.enginetype = StringConverter::parseInt(params[1]);
	else if (attrib == "materials")
		if (params.size() < 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		else
		{
			String mat = params[1];
			Ogre::vector < Ogre::String >::type ar = StringUtil::split(mat," ");
			for(Ogre::vector < Ogre::String >::type::iterator it = ar.begin(); it!=ar.end(); it++)
				t.materials.insert(*it);
		}
}

bool CacheSystem::loadCache()
{
	// clear possible generated entries
	entries.clear();

	String cfgfilename = getCacheConfigFilename(false);
	ImprovedConfigFile cfg;

	if(!resourceExistsInAllGroups(cfgfilename))
	{
		LogManager::getSingleton().logMessage("unable to load config file: "+cfgfilename);
		return false;
	}

	String group = ResourceGroupManager::getSingleton().findGroupContainingResource(String(cfgfilename));
	DataStreamPtr stream=ResourceGroupManager::getSingleton().openResource(cfgfilename, group);

	String line;
	LogManager::getSingleton().logMessage("CacheSystem::loadCache2");

	Cache_Entry t;
	Skin *pSkin;
	int mode = 0;
	while( !stream->eof() )
	{
		line = stream->getLine();
		if(StringUtil::startsWith(line, "shaone=") || StringUtil::startsWith(line, "modcount=") || StringUtil::startsWith(line, "cacheformat=") || StringUtil::startsWith(line, "skincount="))
			// ignore here
			continue;

		// Ignore blanks & comments
		if( !line.length() || line.substr( 0, 2 ) == "//" )
		{
			continue;
		}
		else
		{
			if (!mode)
			{
				// No current entry
				if(line == "mod")
				{
					mode = 1;
					t = Cache_Entry();
					t.resourceLoaded=false;
					t.deleted=false;
					t.changedornew=false; // default upon loading
					// Skip to and over next {
					stream->skipLine("{");
				}
				if(line.substr(0,4) == "skin")
				{
					String sname = "skin";
					if(line.size() > 7)
					{
						sname = line.substr(5);
						StringUtil::trim(sname);
					}
					// "skin" + StringConverter::toString(SkinManager::getSingleton().getSkinCount())
					try
					{
						pSkin = (Skin *)SkinManager::getSingleton().create(sname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).getPointer();
						pSkin->_notifyOrigin(stream->getName());
						// Skip to and over next {
						stream->skipLine("{");
						mode = 2;
					} catch(...)
					{
						continue;
					}
				}
			} else if(mode == 1)
			{
				// Already in mod
				if (line == "}")
				{
					// Finished
					if(mode == 1 && !t.deleted)
						entries.push_back(t);
					mode = 0;
				}
				else
				{
					parseModAttribute(line, t);
				}
			} else if(mode == 2)
			{
				// skin
				if (line == "}")
				{
					// Finished
					pSkin = 0;
					mode = 0;
				}
				else
				{
					SkinManager::getSingleton().parseAttribute(line, pSkin);
				}
			}
		}
	}
	return true;
}
bool CacheSystem::fileExists(String filename)
{
	std::fstream file;
	file.open(filename.c_str());
	return file.is_open();
}

Ogre::String CacheSystem::fileTime(String filename)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	HANDLE hFile = CreateFileA(filename.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	// Set the file time on the file
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME st;
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
		return "";

	if(!FileTimeToSystemTime(&ftWrite, &st))
		return "";

	char tmp[256] = "";
	memset(tmp, 0, 256);
	sprintf(tmp, "%d/%d/%d/%d/%d/%d/%d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return String(tmp);
#else
	// XXX TODO: implement linux filetime!
	// not yet implemented for other platforms
	return "";
#endif
}
String CacheSystem::getRealPath(String path)
{
	// this shall convert the path names to fit the operating system's flavor
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// this is required for windows since we are case insensitive ...
	std::replace( path.begin(), path.end(), '/', '\\' );
	StringUtil::toLowerCase(path);
#endif
	return path;
}

String CacheSystem::getVirtualPath(String path)
{
	std::replace( path.begin(), path.end(), '\\', '/' );
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// this is required for windows since we are case insensitive ...
	StringUtil::toLowerCase(path);
#endif
	return path;
}

int CacheSystem::incrementalCacheUpdate()
{
	entries.clear();

	if(!loadCache())
		//error loading cache!
		return -1;

	LogManager::getSingleton().logMessage("* incremental check starting ...");
	LogManager::getSingleton().logMessage("* incremental check (1/5): deleted and changed files ...");
#ifdef USE_MYGUI 
	LoadingWindow::get()->setProgress(20, _L("incremental check: deleted and changed files"));
#endif //MYGUI
	std::vector<Cache_Entry>::iterator it;
	int counter=0;
	std::vector<Cache_Entry> changed_entries;
	for(it = entries.begin(); it != entries.end(); it++, counter++)
	{
		int progress = ((float)counter/(float)(entries.size()))*100;
#ifdef USE_MYGUI 
		LoadingWindow::get()->setProgress(progress, _L("incremental check: deleted and changed files\n" + it->type + ": " + it->fname));
#endif //MYGUI
		if(it->type == "FileSystem")
		{
			String fn = getRealPath(it->dirname + "/" + it->fname);
			if(!fileExists(fn))
			{
				LogManager::getSingleton().logMessage("- "+fn+" is not existing");
#ifdef USE_MYGUI 
				LoadingWindow::get()->setProgress(20, _L("incremental check: deleted and changed files\n")+it->fname+_L(" not existing"));
#endif // MYGUI
				removeFileFromFileCache(it);
				//entries.erase(it);
				it->deleted=true;
				deletedFiles++;
				if(it == entries.end())
					break;
				continue;
			}
		}
		else if(it->type == "Zip")
		{
			String fn = getRealPath(it->dirname);
			if(!fileExists(fn))
			{
				LogManager::getSingleton().logMessage("- "+fn+_L(" not existing"));
#ifdef USE_MYGUI 
				LoadingWindow::get()->setProgress(20, _L("incremental check: deleted and changed files\n")+it->fname+_L(" not existing"));
#endif //MYGUI
				removeFileFromFileCache(it);
				//entries.erase(it);
				it->deleted = true;
				deletedFiles++;
				if(it == entries.end())
					break;
				continue;
			}

			// check file time, if that fails, fall back to sha1 (needed for platforms where filetime is not yet implemented!
			bool check = false;
			String ft = fileTime(fn);
			if(ft.empty() || it->filetime.empty() || it->filetime == "unkown")
			{
				// slow sha1 check
				char hash[255];
				memset(hash, 0, 255);

				CSHA1 sha1;
				sha1.HashFile(const_cast<char*>(fn.c_str()));
				sha1.Final();
				sha1.ReportHash(hash, CSHA1::REPORT_HEX_SHORT);
				check = (it->hash != String(hash));
			} else
			{
				// faster file time check
				check = (it->filetime != ft);
			}

			if(check)
			{
				changedFiles++;
				LogManager::getSingleton().logMessage("- "+fn+_L(" changed"));
				it->changedornew = true;
				it->deleted = true; // see below
				changed_entries.push_back(*it);
			}
		}

	}

	// we try to reload one zip only one time, not multiple times if it contains more resources at once
	std::vector<Ogre::String> reloaded_zips;
	std::vector<Ogre::String>::iterator sit;
	LogManager::getSingleton().logMessage("* incremental check (2/5): processing changed zips ...");
#ifdef USE_MYGUI 
	LoadingWindow::get()->setProgress(40, _L("incremental check: processing changed zips\n"));
#endif //MYGUI
	for(it = changed_entries.begin(); it != changed_entries.end(); it++)
	{
		bool found=false;
		for(sit=reloaded_zips.begin();sit!=reloaded_zips.end();sit++)
		{
			if(*sit == it->dirname)
			{
				found=true;
				break;
			}
		}
		if (!found)
		{
#ifdef USE_MYGUI 
			LoadingWindow::get()->setProgress(40, _L("incremental check: processing changed zips\n")+it->fname);
#endif //MYGUI
			loadSingleZip(*it);
			reloaded_zips.push_back(it->dirname);
		}
	}
	LogManager::getSingleton().logMessage("* incremental check (3/5): new content ...");
#ifdef USE_MYGUI 
	LoadingWindow::get()->setProgress(60, _L("incremental check: new content\n"));
#endif //MYGUI
	checkForNewContent();

	LogManager::getSingleton().logMessage("* incremental check (4/5): new files ...");
#ifdef USE_MYGUI 
	LoadingWindow::get()->setProgress(80, _L("incremental check: new files\n"));
#endif //MYGUI
	checkForNewKnownFiles();

	LogManager::getSingleton().logMessage("* incremental check (5/5): duplicates ...");
#ifdef USE_MYGUI 
	LoadingWindow::get()->setProgress(90, _L("incremental check: duplicates\n"));
#endif //MYGUI
	std::vector<Cache_Entry>::iterator it2;
	for(it = entries.begin(); it != entries.end(); it++)
	{
		if(it->deleted) continue;
		for(it2 = entries.begin(); it2 != entries.end(); it2++)
		{
			if(it2->deleted) continue;
			// clean paths, important since we compare them ...
			String basename, basepath;

			String dira = it->dirname;
			StringUtil::toLowerCase(dira);
			StringUtil::splitFilename(dira, basename, basepath);
			basepath = getVirtualPath(basepath);
			dira = basepath + basename;


			String dirb = it2->dirname;
			StringUtil::toLowerCase(dirb);
			StringUtil::splitFilename(dira, basename, basepath);
			basepath = getVirtualPath(basepath);
			dirb = basepath + basename;

			String dnameA = it->dname;
			StringUtil::toLowerCase(dnameA);
			StringUtil::trim(dnameA);
			String dnameB = it2->dname;
			StringUtil::toLowerCase(dnameB);
			StringUtil::trim(dnameB);

			String filenameA = it->fname;
			StringUtil::toLowerCase(filenameA);
			String filenameB = it2->fname;
			StringUtil::toLowerCase(filenameB);

			String filenameWUIDA = it->fname_without_uid;
			StringUtil::toLowerCase(filenameWUIDA);
			String filenameWUIDB = it2->fname_without_uid;
			StringUtil::toLowerCase(filenameWUIDB);

			// hard duplicate
			if(dira == dirb && dnameA == dnameB && filenameA == filenameB)
			{
				if(it->number == it2->number) continue; // do not delete self
				LogManager::getSingleton().logMessage("- "+ it2->dirname+"/" + it->fname + " hard duplicate");
				it2->deleted=true;
				continue;
			}
			// soft duplicates
			else if(dira == dirb && dnameA == dnameB && filenameWUIDA == filenameWUIDB)
			{
				if(it->number == it2->number) continue; // do not delete self
				LogManager::getSingleton().logMessage("- "+ it2->dirname+"/" + it->fname + " soft duplicate, resolving ...");
				// create sha1 and see whats the correct entry :)
				CSHA1 sha1;
				sha1.HashFile(const_cast<char*>(it2->dirname.c_str()));
				sha1.Final();
				char hashres[255]="";
				sha1.ReportHash(hashres, CSHA1::REPORT_HEX_SHORT);
				String hashstr = String(hashres);
				if(hashstr == it->hash)
				{
					LogManager::getSingleton().logMessage("  - entry 2 removed");
					it2->deleted=true;
				} else if(hashstr == it2->hash)
				{
					LogManager::getSingleton().logMessage("  - entry 1 removed");
					it->deleted=true;
				} else
				{
					LogManager::getSingleton().logMessage("  - entry 1 and 2 removed");
					it->deleted=true;
					it2->deleted=true;
				}
			}
		}
	}
#ifdef USE_MYGUI 
	LoadingWindow::get()->setAutotrack(_L("loading...\n"));
#endif //MYGUI

	//LogManager::getSingleton().logMessage("* incremental check (5/5): regenerating file cache ...");
	//generateFileCache(true);

	writeGeneratedCache();

#ifdef USE_MYGUI 
	LoadingWindow::get()->hide();
#endif // MYGUI
	LogManager::getSingleton().logMessage("* incremental check done.");
	return 0;
}

Cache_Entry *CacheSystem::getEntry(int modid)
{
	std::vector<Cache_Entry>::iterator it;
	//int counter=0;
	for(it = entries.begin(); it != entries.end(); it++)
		if(modid == it->number)
			return (Cache_Entry *)&*it;
	return 0;
}

void CacheSystem::generateCache(bool forcefull)
{
	//OverlayManager::getSingleton().getOverlayElement("tracks/Title/load")->setCaption("regenerating Mod Cache");
	//setProgress(1, "Opening all packs ...");
	modcounter=0;

	if(!forcefull)
	{
		int res = incrementalCacheUpdate();
		if(res == 0)
			// already incremental updated :)
			return;
	}

	// full regeneration below

	loadAllZips();

	writeGeneratedCache();

	//setProgress(1, "Loaded " + StringConverter::toString(entries.size()) + " mods. Saving Cache ...");

}

Ogre::String CacheSystem::formatInnerEntry(int counter, Cache_Entry t)
{
	String result = "";
	result += "\tnumber="+StringConverter::toString(counter)+"\n"; // always count linear!
	result += "\tdeleted="+StringConverter::toString(t.deleted)+"\n";
	if(!t.deleted)
	{
		// this ensures that we wont break the format with empty ("") values
		if(t.minitype.empty())
			t.minitype = "unkown";
		if(t.type.empty())
			t.type = "unkown";
		if(t.dirname.empty())
			t.dirname = "unkown";
		if(t.fname.empty())
			t.fname = "unkown";
		if(t.fext.empty())
			t.fext = "unkown";
		if(t.filetime.empty())
			t.filetime = "unkown";
		if(t.dname.empty())
			t.dname = "unkown";
		if(t.hash.empty())
			t.hash = "none";
		if(t.uniqueid.empty())
			t.uniqueid = "no-uid";
		if(t.fname_without_uid.empty())
			t.fname_without_uid = "unkown";
		if(t.filecachename.empty())
			t.filecachename = "none";

		result += "\tusagecounter="+StringConverter::toString(t.usagecounter)+"\n";
		result += "\taddtimestamp="+StringConverter::toString(t.addtimestamp)+"\n";
		result += "\tminitype="+t.minitype+"\n";
		result += "\ttype="+t.type+"\n";
		result += "\tdirname="+t.dirname+"\n";
		result += "\tfname="+t.fname+"\n";
		result += "\tfname_without_uid="+t.fname_without_uid+"\n";
		result += "\tfext="+t.fext+"\n";
		result += "\tfiletime="+t.filetime+"\n";
		result += "\tdname="+t.dname+"\n";
		result += "\thash="+t.hash+"\n";
		result += "\tcategoryid="+StringConverter::toString(t.categoryid)+"\n";
		result += "\tuniqueid="+t.uniqueid+"\n";
		result += "\tversion="+StringConverter::toString(t.version)+"\n";
		result += "\tfilecachename="+t.filecachename+"\n";
		//result += "\tnumauthors="+StringConverter::toString(t.authors.size())+"\n";

		if(t.authors.size() > 0)
		{
			for(int i=0;i<(int)t.authors.size();i++)
			{
				if(strnlen(t.authors[i]->type, 3) == 0)
					strcpy(t.authors[i]->type, "unkown");
				if(strnlen(t.authors[i]->name, 3) == 0)
					strcpy(t.authors[i]->name, "unkown");
				if(strnlen(t.authors[i]->email, 3) == 0)
					strcpy(t.authors[i]->email, "unkown");
				result += "\tauthor=" + String(t.authors[i]->type) + \
					"," + StringConverter::toString(t.authors[i]->id) + \
					"," + String(t.authors[i]->name) + "," + String(t.authors[i]->email) + "\n";
			}
		}

		// now add the truck details if existing
		if(t.description!="") result += "\tdescription="+normalizeText(t.description)+"\n";
		if(t.tags!="") result += "\ttags="+t.tags+"\n";
		if(t.fileformatversion!=0) result += "\tfileformatversion="+StringConverter::toString(t.fileformatversion)+"\n";
		if(t.hasSubmeshs) result += "\thasSubmeshs=1\n";
		if(t.nodecount!=0) result += "\tnodecount="+StringConverter::toString(t.nodecount)+"\n";
		if(t.beamcount!=0) result += "\tbeamcount="+StringConverter::toString(t.beamcount)+"\n";
		if(t.shockcount!=0) result += "\tshockcount="+StringConverter::toString(t.shockcount)+"\n";
		if(t.fixescount!=0) result += "\tfixescount="+StringConverter::toString(t.fixescount)+"\n";
		if(t.hydroscount!=0) result += "\thydroscount="+StringConverter::toString(t.hydroscount)+"\n";
		if(t.wheelcount!=0) result += "\twheelcount="+StringConverter::toString(t.wheelcount)+"\n";
		if(t.propwheelcount!=0) result += "\tpropwheelcount="+StringConverter::toString(t.propwheelcount)+"\n";
		if(t.commandscount!=0) result += "\tcommandscount="+StringConverter::toString(t.commandscount)+"\n";
		if(t.flarescount!=0) result += "\tflarescount="+StringConverter::toString(t.flarescount)+"\n";
		if(t.propscount!=0) result += "\tpropscount="+StringConverter::toString(t.propscount)+"\n";
		if(t.wingscount!=0) result += "\twingscount="+StringConverter::toString(t.wingscount)+"\n";
		if(t.turbopropscount!=0) result += "\tturbopropscount="+StringConverter::toString(t.turbopropscount)+"\n";
		if(t.turbojetcount!=0) result += "\tturbojetcount="+StringConverter::toString(t.turbojetcount)+"\n";
		if(t.rotatorscount!=0) result += "\trotatorscount="+StringConverter::toString(t.rotatorscount)+"\n";
		if(t.exhaustscount!=0) result += "\texhaustscount="+StringConverter::toString(t.exhaustscount)+"\n";
		if(t.flexbodiescount!=0) result += "\tflexbodiescount="+StringConverter::toString(t.flexbodiescount)+"\n";
		if(t.materialflarebindingscount!=0) result += "\tmaterialflarebindingscount="+StringConverter::toString(t.materialflarebindingscount)+"\n";
		if(t.soundsourcescount!=0) result += "\tsoundsourcescount="+StringConverter::toString(t.soundsourcescount)+"\n";
		if(t.managedmaterialscount!=0) result += "\tmanagedmaterialscount="+StringConverter::toString(t.managedmaterialscount)+"\n";
		if(t.truckmass>1) result += "\ttruckmass="+StringConverter::toString(t.truckmass)+"\n";
		if(t.loadmass>1) result += "\tloadmass="+StringConverter::toString(t.loadmass)+"\n";
		if(t.minrpm>1) result += "\tminrpm="+StringConverter::toString(t.minrpm)+"\n";
		if(t.maxrpm>1) result += "\tmaxrpm="+StringConverter::toString(t.maxrpm)+"\n";
		if(t.torque>1) result += "\ttorque="+StringConverter::toString(t.torque)+"\n";
		if(t.customtach) result += "\tcustomtach=1\n";
		if(t.custom_particles) result += "\tcustom_particles=1\n";
		if(t.forwardcommands) result += "\tforwardcommands=1\n";
		if(t.importcommands) result += "\timportcommands=1\n";
		if(t.rollon) result += "\trollon=1\n";
		if(t.rescuer) result += "\trescuer=1\n";
		if(t.driveable!=0) result += "\tdriveable="+StringConverter::toString(t.driveable)+"\n";
		if(t.numgears!=0) result += "\tnumgears="+StringConverter::toString(t.numgears)+"\n";
		if(t.enginetype!=0) result += "\tenginetype="+StringConverter::toString(t.enginetype)+"\n";
		if(t.materials.size())
		{
			String matStr = "";
			for(std::set < Ogre::String >::iterator it = t.materials.begin(); it != t.materials.end(); it++)
			{
				matStr += *it + " ";
			}
			result += "\tmaterials=" + matStr + "\n";
		}

		if(t.sectionconfigs.size() > 0)
		{
			for(int i=0;i<(int)t.sectionconfigs.size();i++)
				result += "\tsectionconfig="+t.sectionconfigs[i]+"\n";
		}
	}


	return result;
}

Ogre::String CacheSystem::normalizeText(Ogre::String text)
{
	String result = "";
	Ogre::vector <Ogre::String>::type str = Ogre::StringUtil::split(text, "\n");
	for(Ogre::vector <Ogre::String>::type::iterator it = str.begin(); it!=str.end(); it++)
		result += *it + "$";
	return result;
}

Ogre::String CacheSystem::deNormalizeText(Ogre::String text)
{
	String result = "";
	Ogre::vector <Ogre::String>::type str = Ogre::StringUtil::split(text, "$");
	for(Ogre::vector <Ogre::String>::type::iterator it = str.begin(); it!=str.end(); it++)
		result += *it + "\n";
	return result;
}

Ogre::String CacheSystem::formatEntry(int counter, Cache_Entry t)
{
	String result = "mod\n";
	result += "{\n";
	result += formatInnerEntry(counter, t);
	result += "}\n\n";

	return result;
}

Ogre::String CacheSystem::formatSkinEntry(int counter, Skin *skin)
{
	String result = "skin\n";
	result += "{\n";
	result += "\tname="+skin->name+"\n";
	result += "\tthumbimg="+skin->thumbnail+"\n";
	// todo: finish implementation ...
	result += "}\n\n";
	return result;
}

void CacheSystem::writeGeneratedCache()
{
	String path = getCacheConfigFilename(true);
	LogManager::getSingleton().logMessage("writing cache to file ("+path+")...");

	FILE *f = fopen(path.c_str(), "w");
	if(!f)
		return;
	fprintf(f, "shaone=%s\n", const_cast<char*>(currentSHA1.c_str()));
	fprintf(f, "modcount=%d\n", entries.size());
	fprintf(f, "skincount=%d\n", SkinManager::getSingleton().getSkinCount());
	fprintf(f, "cacheformat=%s\n", CACHE_FILE_FORMAT);

	// mods
	std::vector<Cache_Entry>::iterator it;
	int counter=0;
	for(it = entries.begin(); it != entries.end(); it++)
	{
		if(it->deleted) continue;
		fprintf(f, "%s", formatEntry(counter, *it).c_str());
		counter++;
	}


	// skins
	String skinString = "";
	SkinManager::getSingleton().serialize(skinString);
	fprintf(f, "%s", skinString.c_str());

	// close
	fclose(f);
	LogManager::getSingleton().logMessage("...done!");
}


void CacheSystem::writeStreamCache()
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

	FileInfoListPtr dirs = rgm.findResourceFileInfo("Streams", "*", true);
	for (FileInfoList::iterator itDir = dirs->begin(); itDir!= dirs->end(); ++itDir)
	{
		if (itDir->filename == String(".svn")) continue;
		String dirName = SETTINGS.getSetting("Streams Path") + (*itDir).filename;
		String cacheFilename = dirName + dirsep + "stream.cache";
		FILE *f = fopen(cacheFilename.c_str(), "w");

		// iterate through mods
		std::vector<Cache_Entry>::iterator it;
		int counter=0;
		for(it = entries.begin(); it != entries.end(); it++)
		{
			if(it->deleted) continue;
			if(it->dirname.substr(0, dirName.size()) == dirName)
			{
				if(f) fprintf(f, "%s", formatEntry(counter, *it).c_str());

				ResourceGroupManager::getSingleton().addResourceLocation(it->dirname, "Zip");
				generateFileCache(*it, dirName + dirsep);
				ResourceGroupManager::getSingleton().removeResourceLocation(it->dirname);
				counter++;
			}
		}
		if(f) fclose(f);
	}
}

void CacheSystem::updateSingleTruckEntryCache(int number, Cache_Entry t)
{
	// todo: to be implemented
}

char *CacheSystem::replacesSpaces(char *str)
{
	char *ptr = str;
	while (*ptr!=0) {if (*ptr==' ') *ptr='_';ptr++;};
	return str;
}

char *CacheSystem::restoreSpaces(char *str)
{
	char *ptr = str;
	while (*ptr!=0) {if (*ptr=='_') *ptr=' ';ptr++;};
	return str;
}

bool CacheSystem::stringHasUID(Ogre::String uidstr)
{
	size_t pos = uidstr.find("-");
	if(pos != String::npos && pos >= 3 && uidstr.substr(pos-3, 3) == "UID")
		return true;
	return false;
}

Ogre::String CacheSystem::stripUIDfromString(Ogre::String uidstr)
{
	size_t pos = uidstr.find("-");
	if(pos != String::npos && pos >= 3 && uidstr.substr(pos-3, 3) == "UID")
		return uidstr.substr(pos+1, uidstr.length()-pos);
	return uidstr;
}

Ogre::String CacheSystem::getUIDfromString(Ogre::String uidstr)
{
	size_t pos = uidstr.find("-");
	if(pos != String::npos && pos >= 3 && uidstr.substr(pos-3, 3) == "UID")
		return uidstr.substr(0, pos);
	return "";
}

void CacheSystem::addFile(Ogre::FileInfo f, String ext)
{
	String archiveType = "FileSystem";
	String archiveDirectory = "";
	if(f.archive)
	{
		archiveType = f.archive->getType();
		archiveDirectory = f.archive->getName();
	}

	addFile(f.filename, archiveType, archiveDirectory, ext);
}

void CacheSystem::addFile(String filename, String archiveType, String archiveDirectory, String ext)
{
	LogManager::getSingleton().logMessage("Preparing to add " + filename);

	if(ext == "skin")
	{
		skins_map[filename] = archiveDirectory;
		return;
	}
	//read first line
	Cache_Entry entry;
	if(!resourceExistsInAllGroups(filename))
		return;

	String group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
	if(ResourceGroupManager::getSingleton().resourceExists(group, filename))
	{
		try
		{
			//LogManager::getSingleton().logMessage("Read to add "+String(entry.dname)+" filename "+String(filename));
			DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(filename, group);
			entry.dname = ds->getLine();

			if(ext == "terrn")
				fillTerrainDetailInfo(entry, ds, filename);
			else
				fillTruckDetailInfo(entry, ds, filename);

			// ds closes automatically, so do _not_ close it explicitly below
			entry.fname = filename;
			entry.fname_without_uid = stripUIDfromString(filename);
			entry.fext = ext;
			entry.filetime = fileTime(archiveDirectory);
			entry.type = archiveType;
			entry.dirname = archiveDirectory;
			entry.number = modcounter++;
			entry.addtimestamp = getTimeStamp();
			entry.usagecounter=0;
			entry.deleted = false;
			String basen;
			String ext;
			StringUtil::splitBaseFilename(entry.fname, basen, ext);
			entry.minitype = detectFilesMiniType(basen+"-mini");
			entry.hash="none";
			entry.changedornew=true;
			generateFileCache(entry);
			if(archiveType == "Zip")
				entry.hash = zipHashes[getVirtualPath(archiveDirectory)];
			if(entry.hash == "")
				// fallback if no hash was found
				entry.hash="none";
			// read in author and category
			entries.push_back(entry);
		} catch(Ogre::Exception& e)
		{
			if(e.getNumber() == Ogre::Exception::ERR_DUPLICATE_ITEM)
			{
				LogManager::getSingleton().logMessage(" *** error opening archive '"+filename+"': some files are duplicates of existing files. The file will be ignored.");
				LogManager::getSingleton().logMessage("error while opening resource: " + e.getFullDescription());
			}else
			{
				LogManager::getSingleton().logMessage("error while opening resource: " + e.getFullDescription());
				LogManager::getSingleton().logMessage("error opening archive '"+String(filename)+"'. Is it corrupt?");
				LogManager::getSingleton().logMessage("trying to continue ...");
			}
		}
	}
}

void CacheSystem::fillTruckDetailInfo(Cache_Entry &entry, Ogre::DataStreamPtr ds, Ogre::String fname)
{

	// done initializing, now fill everything!
	// this is a stripped (and cleaned) version of Beam::loadtruck()
	char line[1024];
	int mode=0, savedmode=0, linecounter=0;
	while (!ds->eof())
	{
		linecounter++;
		size_t ll = ds->readLine(line, 1023);
		if (ll==0 || line[0]==';' || line[0]=='/')
			continue;

		if (!strcmp("end",line))
			break;

		if (!strcmp("end_commandlist",line) && mode == 35) {mode=0;continue;};
		if (!strcmp("end_description",line) && mode == 29) {mode=0;continue;};
		if (!strcmp("end_comment",line)  && mode == 30) {mode=savedmode;continue;};
		if (mode==29)
		{
			entry.description += String(line) + "\n";
			continue;
		}

		if (mode==30)
		{
			// comment
			// ignore everything
			continue;
		}
		if (!strcmp("nodes",line)) {mode=1;continue;};
		if (!strcmp("beams",line)) {mode=2;continue;};
		if (!strcmp("fixes",line)) {mode=3;continue;};
		if (!strcmp("shocks",line)) {mode=4;continue;};
		if (!strcmp("hydros",line)) {mode=5;continue;};
		if (!strcmp("wheels",line)) {mode=6;continue;};
		if (!strcmp("globals",line)) {mode=7;continue;};
		if (!strcmp("cameras",line)) {mode=8;continue;};
		if (!strcmp("engine",line)) {mode=9;continue;};
		if (!strcmp("texcoords",line)) {mode=10;continue;};
		if (!strcmp("cab",line)) {mode=11;continue;};
		if (!strcmp("commands",line)) {mode=12;continue;};
		if (!strcmp("commands2",line)) {mode=120;continue;};
		if (!strcmp("forwardcommands",line)) {entry.forwardcommands=true;continue;};
		if (!strcmp("importcommands",line)) {entry.importcommands=true;continue;};
		if (!strcmp("rollon",line)) {entry.rollon=true;continue;};
		if (!strcmp("rescuer",line)) {entry.rescuer=true;continue;};
		if (!strcmp("contacters",line)) {mode=13;continue;};
		if (!strcmp("ropes",line)) {mode=14;continue;};
		if (!strcmp("ropables",line)) {mode=15;continue;};
		if (!strcmp("ties",line)) {mode=16;continue;};
		if (!strcmp("help",line)) {mode=17;continue;};
		if (!strcmp("cinecam",line)) {mode=18;continue;};
		if (!strcmp("flares",line)) {mode=19;continue;};
		if (!strcmp("props",line)) {mode=20;continue;};
		if (!strcmp("globeams",line)) {mode=21;continue;};
		if (!strcmp("wings",line)) {mode=22;continue;};
		if (!strcmp("turboprops",line)) {mode=23;continue;};
		if (!strcmp("fusedrag",line)) {mode=24;continue;};
		if (!strcmp("engoption",line)) {mode=25;continue;};
		if (!strcmp("brakes",line)) {mode=26;continue;};
		if (!strcmp("rotators",line)) {mode=27;continue;};
		if (!strcmp("screwprops",line)) {mode=28;continue;};
		if (!strcmp("description",line)) {mode=29;continue;};
		if (!strcmp("comment",line)) {mode=30; savedmode=mode; continue;};
		if (!strcmp("wheels2",line)) {mode=31;continue;};
		if (!strcmp("guisettings",line)) {mode=32;continue;};
		if (!strcmp("minimass",line)) {mode=33;continue;};
		if (!strcmp("exhausts",line)) {mode=34;continue;};
		if (!strcmp("turboprops2",line)) {mode=35;continue;};
		if (!strcmp("pistonprops",line)) {mode=36;continue;};
		//apparently mode 37 is reserved for other use
		if (!strcmp("particles",line)) {mode=38;continue;};
		if (!strcmp("turbojets",line)) {mode=39;continue;};
		if (!strcmp("rigidifiers",line)) {mode=40;continue;};
		if (!strcmp("airbrakes",line)) {mode=41;continue;};
		if (!strcmp("meshwheels",line)) {mode=42;continue;};
		if (!strcmp("flexbodies",line)) {mode=43;continue;};
		if (!strncmp("hookgroup",line, 9)) {mode=44; /* NOT continue */;};
		if (!strncmp("gripnodes",line, 9)) {mode=45; continue;};
		if (!strncmp("materialflarebindings",line, 21)) {mode=46; continue;};
		if (!strcmp("disabledefaultsounds",line)) {continue;};
		if (!strcmp("soundsources",line)) {mode=47;continue;};
		if (!strcmp("envmap",line)) {mode=48;continue;};
		if (!strcmp("managedmaterials",line)) {mode=49;continue;};
		if (!strncmp("sectionconfig",line, 13)) {savedmode=mode;mode=50; /* NOT continue */};
		if (!strncmp("section",line, 7) && mode!=50) {mode=51; /* NOT continue */};
		/* 52 = reserved for ignored section */


		if (!strcmp("commandlist",line))
			continue;
		if (!strncmp("fileinfo", line, 8))
		{
			char uniquetruckid[255]="";
			int categoryid=0, truckversion=0;
			String lineStr = String(line);
			Ogre::vector<String>::type args = StringUtil::split(lineStr, ", ");
			if(args.size() < 1)
			{
				LogManager::getSingleton().logMessage("Error parsing File (fileinfo) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			mode=0;
			if(args.size() > 1)	entry.uniqueid = args[1];
			if(args.size() > 2) entry.categoryid = StringConverter::parseInt(args[2]);
			if(args.size() > 3)	entry.version = StringConverter::parseInt(args[3]);
			continue;
		}
		if (!strncmp("fileformatversion", line, 17))
		{
			int fileformatversion;
			int result = sscanf(line,"fileformatversion %i", &fileformatversion);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (fileformatversion) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			mode=0;
			entry.fileformatversion = fileformatversion;
			continue;
		}
			if (!strncmp("author", line, 6))
			{
				int authorid;
				char authorname[255], authoremail[255], authortype[255];
				authorinfo_t *author = new authorinfo_t();
				author->id = -1;
				strcpy(author->email, "unkown");
				strcpy(author->name,  "unkown");
				strcpy(author->type,  "unkown");

				int result = sscanf(line,"author %s %i %s %s", authortype, &authorid, authorname, authoremail);
				if (result < 1 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (author) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//replace '_' with ' '
				char *tmp = authorname;
				while (*tmp!=0) {if (*tmp=='_') *tmp=' ';tmp++;};
				//fill the struct now
				author->id = authorid;
				if(strnlen(authortype, 250) > 0)
					strncpy(author->type, authortype, 255);
				if(strnlen(authorname, 250) > 0)
					strncpy(author->name, authorname, 255);
				if(strnlen(authoremail, 250) > 0)
					strncpy(author->email, authoremail, 255);
				entry.authors.push_back(author);
				mode=0;
				continue;
			}


			if (!strncmp("set_beam_defaults", line, 17))
				continue;

			if (!strncmp("set_skeleton_settings", line, 21))
				continue;

			if (!strcmp("backmesh",line))
				continue;
			if (!strcmp("submesh",line))
			{
				entry.hasSubmeshs = true;
				continue;
			};
			if (mode==1)
			{
				//parse nodes
				int id=0;
				float x=0, y=0, z=0, mass=0;
				char options[50] = "n";
				int result = sscanf(line,"%i, %f, %f, %f, %s %f",&id,&x,&y,&z,options, &mass);
				// catch some errors
				if (result < 4 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					//LogManager::getSingleton().logMessage(strerror(errno));
					continue;
				}
				entry.nodecount++;
				continue;
			}
			else if (mode==2)
			{
				//parse beams
				int id1, id2;
				char options[50] = "v";
				int type=0;
				int result = sscanf(line,"%i, %i, %s",&id1,&id2,options);
				if (result < 2 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Beam) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.beamcount++;
				continue;
			}
			else if (mode==4)
			{
				//parse shocks
				int id1, id2;
				float s, d, sbound,lbound,precomp;
				char type='n';
				int result = sscanf(line,"%i, %i, %f, %f, %f, %f, %f, %c",&id1,&id2, &s, &d, &sbound, &lbound,&precomp,&type);
				if (result < 7 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Shock) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.shockcount++;
				continue;
			}
			else if (mode==3)
			{
				//parse fixes
				int id;
				int result = sscanf(line,"%i",&id);
				if (result < 1 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Fixes) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.fixescount++;
			}
			else if (mode==5)
			{
				//parse hydros
				int id1, id2;
				float ratio;
				char options[50] = "n";
				int result = sscanf(line,"%i, %i, %f, %s",&id1,&id2,&ratio, options);
				if (result < 3 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Hydro) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.hydroscount++;
				continue;
			}
			else if (mode==6)
			{
				//parse wheels
				float radius, width, mass, spring, damp;
				char texf[256];
				char texb[256];
				int rays, node1, node2, snode, braked, propulsed, torquenode;
				int result = sscanf(line,"%f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %s %s",&radius,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp, texf, texb);
				if (result < 14 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Wheel) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.wheelcount++;
				if(propulsed) entry.propwheelcount++;
				continue;
			}
			else if (mode==31)
			{
				//parse wheels2
				char texf[256];
				char texb[256];
				float radius, radius2, width, mass, spring, damp, spring2, damp2;
				int rays, node1, node2, snode, braked, propulsed, torquenode;
				int result = sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %f, %f, %s %s",&radius,&radius2,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp,&spring2,&damp2, texf, texb);
				if (result < 17 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Wheel2) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				if(propulsed) entry.propwheelcount++;
				entry.wheelcount++;
				continue;
			}
			else if (mode==42)
			{
				//parse meshwheels
				char meshw[256];
				char texb[256];
				float radius, rimradius, width, mass, spring, damp;
				char side;
				int rays, node1, node2, snode, braked, propulsed, torquenode;
				int result = sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %c, %s %s",&radius,&rimradius,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp, &side, meshw, texb);
				if (result < 16 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (MeshWheel) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				if(propulsed) entry.propwheelcount++;
				entry.wheelcount++;
				continue;
			}
			else if (mode==7)
			{
				float truckmass=0, loadmass=0;
				char texname[255];
				memset(texname,0, 255);
				//parse globals
				int result = sscanf(line,"%f, %f, %s",&truckmass, &loadmass, texname);
				if (result < 2 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Globals) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.truckmass = truckmass;
				entry.loadmass = loadmass;
				addUniqueString(entry.materials, texname);

				continue;
			}
			else if (mode==8)
			{
				//parse cameras
				continue;
			}
			else if (mode==9)
			{
				//parse engine
				float minrpm, maxrpm, torque, dratio, rear;
				float gears[16];
				int numgears;

				entry.driveable = 1; // 1 = TRUCK
				int result = sscanf(line,"%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", &minrpm, &maxrpm, &torque, &dratio, &rear, &gears[0],&gears[1],&gears[2],&gears[3],&gears[4],&gears[5],&gears[6],&gears[7],&gears[8],&gears[9],&gears[10],&gears[11],&gears[12],&gears[13],&gears[14],&gears[15]);
				if (result < 7 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Engine) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				for (numgears=0; numgears<16; numgears++)
					if (gears[numgears]<=0)
						break;
				if (numgears < 3)
				{
					LogManager::getSingleton().logMessage("Trucks with less than 3 gears are not supported! " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.numgears = numgears;
				entry.minrpm = minrpm;
				entry.maxrpm = maxrpm;
				entry.torque = torque;
				continue;
			}

			else if (mode==10)
			{
				//parse texcoords
				continue;
			}

			else if (mode==11)
			{
				//parse cab
				continue;
			}

			else if (mode==12 || mode==120)
			{
				//parse commands
				int id1, id2,keys,keyl;
				float rateShort, rateLong, shortl, longl;
				char options[250]="";
				char descr[200] = "";
				int result = 0;
				if(mode == 12)
				{
					char opt='n';
					result = sscanf(line,"%i, %i, %f, %f, %f, %i, %i, %c, %s", &id1, &id2, &rateShort, &shortl, &longl, &keys, &keyl, &opt, descr);
					if (result < 7 || result == EOF) {
						LogManager::getSingleton().logMessage("Error parsing File (Command) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
						continue;
					}
				}
				else if(mode == 120)
				{
					result = sscanf(line,"%i, %i, %f, %f, %f, %f, %i, %i, %s %s", &id1, &id2, &rateShort, &rateLong, &shortl, &longl, &keys, &keyl, options, descr);
					if (result < 8 || result == EOF) {
						LogManager::getSingleton().logMessage("Error parsing File (Command) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
						continue;
					}
				}
				entry.commandscount++;
				continue;
			}

			else if (mode==13)
			{
				//parse contacters
				continue;
			}
			else if (mode==14)
			{
				//parse ropes
				continue;
			}
			else if (mode==15)
			{
				//parse ropables
				continue;
			}
			else if (mode==16)
			{
				//parse ties
				continue;
			}
			else if (mode==17)
			{
				//help material
				continue;
			}
			else if (mode==18)
			{
				//cinecam
				continue;
			}

			else if (mode==19)
			{
				//parse flares
				int ref=-1, nx=0, ny=0, controlnumber=-1, blinkdelay=-2;
				float ox=0, oy=0, size=-2;
				char type='f';
				char matname[255]="";
				int result = sscanf(line,"%i, %i, %i, %f, %f, %c, %i, %i, %f %s", &ref, &nx, &ny, &ox, &oy, &type, &controlnumber, &blinkdelay, &size, matname);
				if (result < 5 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.flarescount++;
				continue;
			}
			else if (mode==20)
			{
				//parse props
				int ref, nx, ny;
				float ox, oy, oz;
				float rx, ry, rz;
				char meshname[256];
				int result = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname);
				if (result < 10 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Prop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				/*
				// XXX TODO fix for skins at some point
				Entity *te=0;
				String propMats="";
				try
				{
					if(smgr)
					{
						te = smgr->createEntity("CacheEntityMaterialTest", String(meshname));
						if(!te) continue;
						addMeshMaterials(entry, te);
						smgr->destroyEntity(te);
					}
				}catch(...)
				{
					LogManager::getSingleton().logMessage("error loading mesh: "+String(meshname));
				}
				*/
				entry.propscount++;
				continue;
			}
			else if (mode==21)
			{
				//parse globeams
				continue;
			}
			else if (mode==22)
			{
				//parse wings
				int nds[8];
				float txes[8];
				char type;
				float cratio, mind, maxd;
				char afname[256];
				int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %f, %f, %f, %f, %f, %c, %f, %f, %f, %s",
					&nds[0],
					&nds[1],
					&nds[2],
					&nds[3],
					&nds[4],
					&nds[5],
					&nds[6],
					&nds[7],
					&txes[0],
					&txes[1],
					&txes[2],
					&txes[3],
					&txes[4],
					&txes[5],
					&txes[6],
					&txes[7],
					&type,
					&cratio,
					&mind,
					&maxd,
					afname
					);
				//visuals
				if (result < 13 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Wing) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.wingscount++;
				continue;
			}
			else if (mode==23 || mode==35 || mode==36) //turboprops, turboprops2, pistonprops
			{
				//parse turboprops
				int ref,back,p1,p2,p3,p4;
				int couplenode=-1;
				float pitch=-10;
				bool isturboprops=true;
				float power;
				char propfoil[256];
				if (mode==23)
				{
					int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &power, propfoil);
					if (result < 8 || result == EOF) {
						LogManager::getSingleton().logMessage("Error parsing File (Turboprop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
						continue;
					}
				}
				if (mode==35)
				{
					int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &couplenode, &power, propfoil);
					if (result < 9 || result == EOF) {
						LogManager::getSingleton().logMessage("Error parsing File (Turboprop2) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
						continue;
					}
				}
				if (mode==36)
				{
					int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %f, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &couplenode, &power, &pitch, propfoil);
					if (result < 10 || result == EOF) {
						LogManager::getSingleton().logMessage("Error parsing File (Pistonprop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
						continue;
					}
				}
				entry.driveable = 2; // 2 = AIRPLANE
				entry.turbopropscount++;
				continue;
			}
			else if (mode==24)
			{
				//parse fusedrag
				continue;
			}
			else if (mode==25)
			{
				//parse engoption
				float inertia;
				char type;
				int result = sscanf(line,"%f, %c", &inertia, &type);
				if (result < 1 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Engoption) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.enginetype=type;
				continue;
			}
			else if (mode==26)
			{
				//parse brakes
				continue;
			}
			else if (mode==27)
			{
				//parse rotators
				int axis1, axis2,keys,keyl;
				int p1[4], p2[4];
				float rate;
				int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %f, %i, %i", &axis1, &axis2, &p1[0], &p1[1], &p1[2], &p1[3], &p2[0], &p2[1], &p2[2], &p2[3], &rate, &keys, &keyl);
				if (result < 13 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Rotators) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.rotatorscount++;
				continue;
			}
			else if (mode==28)
			{
				//parse screwprops
				int ref,back,up;
				float power;
				int result = sscanf(line,"%i, %i, %i, %f", &ref,&back,&up, &power);
				if (result < 4 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Screwprops) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.driveable=3; // 3 = BOAT;
				continue;
			}
			else if (mode==32)
			{
				// guisettings
				char keyword[255];
				char value[255];
				int result = sscanf(line,"%s %s", keyword, value);
				if (result < 2 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (guisettings) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.customtach=true;
				continue;
			}
			else if (mode==33)
			{
				//parse minimass
				continue;
			}
			else if (mode==34)
			{
				// parse exhausts
				int id1, id2;
				float factor;
				char material[50] = "";
				int result = sscanf(line,"%i, %i, %f %s", &id1, &id2, &factor, material);
				// catch some errors
				if (result < 4 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.exhaustscount++;
				continue;
			}
			else if (mode==37)
			{
				// command lists
			}
			else if (mode==38)
			{
				// parse particle
				int id1, id2;
				char psystem[250] = "";
				int result = sscanf(line,"%i, %i, %s", &id1, &id2, psystem);
				// catch some errors
				if (result < 3 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.custom_particles++;
				continue;
			}
			else if (mode==39) //turbojets
			{
				//parse turbojets
				int front,back,ref, rev;
				float len, fdiam, bdiam, drthrust, abthrust;
				int result = sscanf(line,"%i, %i, %i, %i, %f, %f, %f, %f, %f", &front, &back, &ref, &rev, &drthrust, &abthrust, &fdiam, &bdiam, &len);
				if (result < 9 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Turbojet) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.turbojetcount++;
				continue;
			}
			else if (mode==40)
			{
				//parse rigidifiers
				continue;
			}
			else if (mode==41)
			{
				//parse airbrakes
				continue;
			}
			else if (mode==43)
			{
				//parse flexbodies
				int ref, nx, ny;
				float ox, oy, oz;
				float rx, ry, rz;
				char meshname[256];
				int result = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname);
				if (result < 10 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Flexbodies) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				/*
				// XXX TODO fix for skins at some point
				Entity *te=0;
				String propMats="";
				try
				{
					if(smgr)
					{
						te = smgr->createEntity("CacheEntityMaterialTest", String(meshname));
						if(!te) continue;
						addMeshMaterials(entry, te);
						smgr->destroyEntity(te);
					}
				}catch(...)
				{
					LogManager::getSingleton().logMessage("error loading mesh: "+String(meshname));
				}
				*/
				entry.flexbodiescount++;
				continue;
			}
			else if (mode==44)
			{
				//parse hookgroups
				continue;
			}
			else if (mode==45)
			{
				//parse gripnodes
				continue;
			}
			else if (mode==46)
			{
				// parse materialflarebindings
				int flareid;
				char material[255]="";
				memset(material, 0, 255);
				int result = sscanf(line,"%d, %s", &flareid, material);
				if (result < 2 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (materialbindings) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.materialflarebindingscount++;
				continue;
			}
			else if (mode==47)
			{
				//parse soundsources
				int ref;
				char script[256];
				int result = sscanf(line,"%i, %s", &ref, script);
				if (result < 2 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (soundsource) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.soundsourcescount++;
				continue;
			}
			else if (mode==48)
			{
				// parse envmap
				// we do nothing of this for the moment
			}
			else if (mode==49)
			{
				// parse managedmaterials
				char material[255];
				material[0]=0;
				char type[255];
				type[0]=0;
				int result = sscanf(line,"%s %s", material, type);
				if (result < 2 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.managedmaterialscount++;
				continue;
			}
			else if (mode==50)
			{
				// parse sectionconfig
				int version;
				char sectionName[256];
				if(strnlen(line, 16) < 13)
					continue;
				int result = sscanf(line+13,"%i %s", &version, sectionName);
				if (result < 2 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (soundsource) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				entry.sectionconfigs.push_back(sectionName);
				mode = savedmode;
			}
			else if (mode==51)
			{
				// not used here, we wont respect sections for a quick parsing ...
			}
		};
}

int CacheSystem::addUniqueString(std::set<Ogre::String> &list, Ogre::String str)
{
	// ignore some render texture targets
	if(str == "mirror") return 0;
	if(str == "renderdash") return 0;

	str = stripUIDfromString(str);

	if (list.find(str) == list.end())
	{
		list.insert(str);
		return 1;
	}
	return 0;
}

Ogre::String CacheSystem::addMeshMaterials(Cache_Entry &entry, Ogre::Entity *e)
{
	String materials = "";
	MeshPtr m = e->getMesh();
	if(!m.isNull())
	{
		for(int n=0; n<(int)m->getNumSubMeshes();n++)
		{
			SubMesh *sm = m->getSubMesh(n);
			addUniqueString(entry.materials, sm->getMaterialName());
		}
	}

	for(int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		addUniqueString(entry.materials, subent->getMaterialName());
	}
	return materials;
}


Ogre::String CacheSystem::getSkinSource(Ogre::String filename)
{
	return skins_map[filename];
}

int CacheSystem::getTimeStamp()
{
	return (int)time(NULL); //this will overflow in 2038
}

void CacheSystem::deleteFileCache(char *filename)
{
	int res = remove(filename);
	if(res!=0)
		LogManager::getSingleton().logMessage("error deleting file '"+String(filename)+"'");
}

Ogre::String CacheSystem::detectFilesMiniType(String filename)
{
	//search if mini picture exists
	if(!resourceExistsInAllGroups(filename+".dds"))
	{
		if(!resourceExistsInAllGroups(filename+".png"))
			return "none";
		else
			return "png";
	}
	return "dds";
}

void CacheSystem::removeFileFromFileCache(std::vector<Cache_Entry>::iterator iter)
{
	//LogManager::getSingleton().logMessage("removing file cache number "+StringConverter::toString(iter->number));
	if(iter->minitype != "none")
	{
		String fn = location + iter->filecachename;
		deleteFileCache(const_cast<char*>(fn.c_str()));
	}

}

void CacheSystem::generateFileCache(Cache_Entry &entry, Ogre::String directory)
{
	try
	{

		if(directory.empty() && !entry.changedornew)
			return;

		if(entry.fname == "")
			return;

		LogManager::getSingleton().logMessage(" -"+entry.fname+" ...");
		if(entry.minitype == "none")
		{
			entry.filecachename = "none";
			return;
		}

		String outBasename = "";
		String outPath = "";
		StringUtil::splitFilename(entry.dirname, outBasename, outPath);

		if(directory.empty())
			directory = location;

		String dst = directory + outBasename + "_" + entry.fname + ".mini." + entry.minitype;

		// no path info in the cache file name ...
		entry.filecachename = outBasename + "_" + entry.fname + ".mini." + entry.minitype;

		String fbase = "", fext = "";
		StringUtil::splitBaseFilename(entry.fname, fbase, fext);
		String minifn = fbase + "-mini." + entry.minitype;

		String group = "";
		bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(minifn);
		if(!exists)
		{
			String base, ext;
			StringUtil::splitBaseFilename(entry.fname, base, ext);
			entry.minitype = detectFilesMiniType(base + "-mini");
			exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(minifn);
			if(!exists)
			{
				// no minipic found
				entry.filecachename = "none";
				return;
			} else
			{
				group = ResourceGroupManager::getSingleton().findGroupContainingResource(minifn);
			}
		} else
		{
			group = ResourceGroupManager::getSingleton().findGroupContainingResource(minifn);
		}

		FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, minifn);
		if (files->empty())
		{
			deleteFileCache(const_cast<char*>(dst.c_str()));
			return;
		}

		size_t fsize = 0;
		char *buffer=0;
		{
			DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(minifn, group);
			fsize = ds->size();
			buffer = (char*)ror_malloc(fsize);
			memset(buffer, 0, fsize);
			size_t read = ds->read(buffer, fsize);
			if(read!=fsize)
				return;
		}

		bool written=false;
		FILE *f = fopen(dst.c_str(),"wb");
		if(f)
		{
			fwrite(buffer, 1, fsize,  f);
			fclose(f);
			written=true;
		}
		if(buffer)
			ror_free(buffer);
		if(written)
		{
			//setProgress(counter/(float)size, "Caching Mini Files ...");
		} else
		{
			deleteFileCache(const_cast<char*>(dst.c_str()));
		}
	}catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("error while generating File cache: " + e.getFullDescription());
		LogManager::getSingleton().logMessage("trying to continue ...");
	}
	LogManager::getSingleton().logMessage("done generating file cache!");
}

void CacheSystem::parseKnownFilesAllRG()
{
	for(std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
		parseFilesAllRG(*sit);
}

void CacheSystem::parseFilesAllRG(Ogre::String ext)
{
	StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
	StringVector::iterator it;
	for(it = sv.begin(); it!=sv.end(); it++)
		parseFilesOneRG(ext, *it);

	LogManager::getSingleton().logMessage("* parsing files of all Resource Groups (" + ext + ") finished!");
}

void CacheSystem::parseKnownFilesOneRG(Ogre::String rg)
{
	for(std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
		parseFilesOneRG(*sit, rg);
}

void CacheSystem::parseKnownFilesOneRGDirectory(Ogre::String rg, Ogre::String dir)
{
	String dirb = dir;
	getVirtualPath(dirb);
	for(std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
	{
		FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(rg, String("*.")+*sit);
		for (FileInfoList::iterator iterFiles = files->begin(); iterFiles!= files->end(); ++iterFiles)
		{
			if(!iterFiles->archive) continue;

			String dira = iterFiles->archive->getName();
			getVirtualPath(dira);

			if(dira == dirb)
				addFile(*iterFiles, *sit);
		}
	}
}

void CacheSystem::parseFilesOneRG(Ogre::String ext, Ogre::String rg)
{
	//LogManager::getSingleton().logMessage("* parsing files ... (" + ext + ")");
	FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(rg, String("*.")+ext);
	FileInfoList::iterator iterFiles = files->begin();
	int i=0;
	//, size=files->end() - files->begin();
	for (; iterFiles!= files->end(); ++iterFiles, i++)
	{
		addFile(*iterFiles, ext);
	}
}

bool CacheSystem::isFileInEntries(Ogre::String filename)
{
	for(std::vector<Cache_Entry>::iterator it = entries.begin(); it!=entries.end(); it++)
	{
		if(it->fname == filename)
			return true;
	}
	return false;
}
void CacheSystem::generateZipList()
{
	zipCacheList.clear();
	for(std::vector<Cache_Entry>::iterator it = entries.begin(); it!=entries.end(); it++)
	{
		zipCacheList.insert(getVirtualPath(it->dirname));
		//LogManager::getSingleton().logMessage("zip path added: "+getVirtualPath(it->dirname));
	}
}

bool CacheSystem::isZipUsedInEntries(Ogre::String filename)
{
	if(zipCacheList.empty())
		generateZipList();
	//LogManager::getSingleton().logMessage("isZipUsedInEntries: "+getVirtualPath(filename));

	return (zipCacheList.find(getVirtualPath(filename)) != zipCacheList.end());
}

bool CacheSystem::isDirectoryUsedInEntries(Ogre::String directory)
{
	String dira = directory;
	dira = getVirtualPath(dira);

	std::vector<Cache_Entry>::iterator it;
	for(it = entries.begin(); it!=entries.end(); it++)
	{
		if(it->type != "FileSystem") continue;
		String dirb = it->dirname;
		dirb = getVirtualPath(dirb);
		if(dira == dirb)
			return true;
		if(dira.substr(0,dirb.size()) == dirb) //check if its a subdirectory
			return true;
	}
	return false;
}

void CacheSystem::checkForNewKnownFiles()
{
	for(std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
		checkForNewFiles(*sit);
}

void CacheSystem::checkForNewFiles(Ogre::String ext)
{
	char fname[256];
	sprintf(fname, "*.%s", ext.c_str());

	StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
	StringVector::iterator it;
	int rcounter=0;
	for(it = sv.begin(); it!=sv.end(); it++,rcounter++)
	{
		FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(*it, fname);
		FileInfoList::iterator iterFiles = files->begin();
		int i=0;
		for (; iterFiles!= files->end(); ++iterFiles, i++)
		{
			String fn = iterFiles->filename.c_str();
			if(!isFileInEntries(fn))
			{
				if(iterFiles->archive->getType() == "Zip")
					LogManager::getSingleton().logMessage("- " + fn + " is new (in zip)");
				else
					LogManager::getSingleton().logMessage("- " + fn + " is new");
				newFiles++;
				addFile(*iterFiles, ext);
			}
		}
	}
}

String CacheSystem::filenamesSHA1()
{
	String filenames = "";

	// get all Files
	/*
	StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
	StringVector::iterator it;
	for(it = sv.begin(); it!=sv.end(); it++)
	{
		StringVectorPtr files = ResourceGroupManager::getSingleton().listResourceNames(*it);
		for(StringVector::iterator i=files->begin(); i!=files->end(); i++)
		{
			// only use the important files :)
			for(std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
				if(i->find("."+*sit) != String::npos && i->find(".dds") == String::npos && i->find(".png") == String::npos)
					filenames += "General/" + *i + "\n";
		}
	}
	*/

	// and we use all folders and files for the hash
	String restype[3] = {"Packs", "TerrainFolders", "VehicleFolders"};
	for(int i=0;i<3;i++)
	{
		for(int b=0;b<2;b++)
		{
			FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(restype[i], (b==1));
			for(FileInfoList::iterator iterFiles = list->begin(); iterFiles!=list->end(); iterFiles++)
			{
				String name = restype[i] + "/";
				if(iterFiles->archive) name += iterFiles->archive->getName() + "/";

				if(b==0)
				{
					// special file handling, only add important files!
					bool vipfile=false;
					for(std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
					{
						if(
							(iterFiles->filename.find("."+*sit) != String::npos && iterFiles->filename.find(".dds") == String::npos && iterFiles->filename.find(".png") == String::npos)
							|| (iterFiles->filename.find(".zip") != String::npos)
						  )
						{
							vipfile=true;
							break;
						}
					}
					if(!vipfile) continue;
				}
				name += iterFiles->filename;
				filenames += name + "\n";
			}
		}
	}

	//LogManager::getSingleton().logMessage("hash string: "+filenames);
	char result[255]="";

	CSHA1 sha1;
	char *data = const_cast<char*>(filenames.c_str());
	sha1.UpdateHash((uint8_t *)data, strlen(data));
	sha1.Final();
	sha1.ReportHash(result, CSHA1::REPORT_HEX_SHORT);
	return String(result);
}

void CacheSystem::fillTerrainDetailInfo(Cache_Entry &entry, Ogre::DataStreamPtr ds, Ogre::String fname)
{
	char authorformat[255] = "//author %s %i %s %s";
	char authortag[255] = "//author";

	char categoryformat[255] = "//fileinfo %s %i, %i";
	char categorytag[255] = "//fileinfo";

	//parsing the current file
	entry.authors.clear();
	LogManager::getSingleton().logMessage("Parsing terrain for detail information: '"+String(fname)+"'");
	//LogManager::getSingleton().logMessage("Parsing for authors: '"+String(d.fname)+"'");
	char line[1024];
	int linecounter = 0;
	int categoryid=-1, version=-1;

	// try to get unique ID from the filename!
	String uniqueid = "no-uid";
	String fileuid = getUIDfromString(fname);
	if(fileuid != "") uniqueid = fileuid;

	while (!ds->eof())
	{
		size_t ll=ds->readLine(line, 1023);
		if (ll <= 1)
			// ignore empty lines
			continue;
		linecounter++;
		if (!strncmp(authortag, line, strnlen(authortag, 254))) {
			int authorid;
			char authorname[255], authoremail[255], authortype[255];
			memset(authorname, 0, 255);
			memset(authoremail, 0, 255);
			memset(authortype, 0, 255);
			int result = sscanf(line, authorformat, &authortype, &authorid, &authorname, &authoremail);
			if (result < 1 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (author) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//replace '_' with ' '
			authorinfo_t *author = new authorinfo_t();
			char *tmp = authorname;
			while (*tmp!=0) {if (*tmp=='_') *tmp=' ';tmp++;};
			//fill the struct now
			author->id = authorid;
			strncpy(author->type, authortype, 255);
			strncpy(author->name, authorname, 255);
			strncpy(author->email, authoremail, 255);
			entry.authors.push_back(author);
		} else if (!strncmp(categorytag, line, strnlen(categorytag, 254)))
		{
			char uidtmp[255] = "";
			int result = sscanf(line, categoryformat, uidtmp, &categoryid, &version);
			if (result < 3 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (fileinfo) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(uniqueid != "")
			{
				// remove trailing comma
				size_t slen = strnlen(uidtmp, 250);
				if(slen > 0 && uidtmp[slen-1] == ',')
					uidtmp[slen-1] = 0;

				// fix the -1 from the old format
				if(strnlen(uidtmp, 5) > 0 && strncmp(uidtmp, "-1", 2))
					uniqueid = String(uidtmp);
			}

		}
	}
	entry.categoryid = categoryid;
	entry.uniqueid = uniqueid;
	entry.version = version;
}

int CacheSystem::getCategoryUsage(int category)
{
	return category_usage[category];
}

void CacheSystem::readCategoryTitles()
{
	LogManager::getSingleton().logMessage("Loading category titles from "+configlocation+"categories.cfg");
	FILE *fd;
	char line[1024];
	String filename = configlocation + String("categories.cfg");
	fd=fopen(filename.c_str(), "r");
	if(!fd)
	{
		LogManager::getSingleton().logMessage("error opening file: "+configlocation+"categories.cfg");
		return;
	}
	while (!feof(fd))
	{
		int res = fscanf(fd," %[^\n\r]",line);
		if (line[0]==';')
		{
			continue;
		};
		int number=0;
		char title[255];
		const char delimiters[] = ",";
		char *token, str_work[1024]="";
		strncpy(str_work, line, 1024);
		token = strtok(str_work, delimiters);
		if(token != NULL)
			number = atoi(token);
		else
			continue;

		token = strtok(NULL, delimiters);
		if(token != NULL)
		{
			//strip spaces at the beginning
			while(*token == ' ') token++;
			strncpy(title, token, 255);
		}
		else
			continue;
		//LogManager::getSingleton().logMessage(String(title));
		Category_Entry ce;
		ce.title = Ogre::String(title);
		ce.number = number;
		categories[number] = ce;
	}
	fclose(fd);
}

bool CacheSystem::checkResourceLoaded(Ogre::String &filename)
{
	Ogre::String group = "";
	return checkResourceLoaded(filename, group);
}

Cache_Entry CacheSystem::getResourceInfo(Ogre::String &filename)
{
	Cache_Entry def;
	std::vector<Cache_Entry>::iterator it;
	for(it = entries.begin(); it != entries.end(); it++)
		if(it->fname == filename || it->fname_without_uid == filename)
			return *it;
	return def;
}

bool CacheSystem::checkResourceLoaded(Ogre::String &filename, Ogre::String &group)
{
	// check if we already loaded it via ogre ...
	bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(filename);
	if(exists)
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
		return true;
	}

	std::vector<Cache_Entry>::iterator it;
	//int counter=0;
	for(it = entries.begin(); it != entries.end(); it++)
	{
		// case insensitive comparison
		String fname = it->fname;
		String filename_lower = filename;
		String fname_without_uid_lower = it->fname_without_uid;
		StringUtil::toLowerCase(fname);
		StringUtil::toLowerCase(filename_lower);
		StringUtil::toLowerCase(fname_without_uid_lower);
		if(fname == filename_lower || fname_without_uid_lower == filename_lower)
		{
			// we found the file, load it
			filename = it->fname;
			bool res = checkResourceLoaded(*it);
			bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(filename);
			if(!exists)
				return false;
			group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
			return res;
		}
	}
	return false;
}

bool CacheSystem::checkResourceLoaded(Cache_Entry t)
{
	static int rgcountera = 0;
	static std::map<String, bool> loaded;
	if(t.resourceLoaded || loaded[t.dirname])
		// only load once
		return true;
	if(t.type == "FileSystem" || t.type == "Zip")
	{
		try
		{
			rgcountera++;
			String name = "General-Reloaded-"+StringConverter::toString(rgcountera);
			ResourceGroupManager::getSingleton().addResourceLocation(t.dirname, t.type, name);
			loaded[t.dirname]=true;
			ResourceGroupManager::getSingleton().initialiseResourceGroup(name);
			return true;
		} catch(Ogre::Exception& e)
		{
			if(e.getNumber() == Ogre::Exception::ERR_DUPLICATE_ITEM)
			{
				LogManager::getSingleton().logMessage(" *** error opening '"+t.dirname+"': some files are duplicates of existing files. The archive/directory will be ignored.");
				LogManager::getSingleton().logMessage("error while opening resource: " + e.getFullDescription());
			} else
			{
				LogManager::getSingleton().logMessage("error opening '"+t.dirname+"'.");
				if(t.type == "Zip")
					LogManager::getSingleton().logMessage("Is the zip archive corrupt? Error: " + e.getFullDescription());
				LogManager::getSingleton().logMessage("Error description : " + e.getFullDescription());
				LogManager::getSingleton().logMessage("trying to continue ...");
			}
		}
	}
	return false;
}

void CacheSystem::loadSingleZip(Cache_Entry e, bool unload, bool ownGroup)
{
	loadSingleZip(e.dirname, -1, unload, ownGroup);
}

void CacheSystem::loadSingleZip(Ogre::FileInfo f, bool unload, bool ownGroup)
{
	String zippath = f.archive->getName() + "/" + f.filename;
	int cfactor = -1;
	if(f.uncompressedSize > 0)
		cfactor = (f.compressedSize / f.uncompressedSize) * 100.0f;
	loadSingleZip(zippath, cfactor, unload, ownGroup);
}

void CacheSystem::loadSingleDirectory(String dirname, String group, bool alreadyLoaded)
{
	char hash[255];
	memset(hash, 0, 255);

	LogManager::getSingleton().logMessage("Adding directory " + dirname);

	rgcounter++;
	String rgname = "General-"+StringConverter::toString(rgcounter);

	try
	{
		if(alreadyLoaded)
		{
			parseKnownFilesOneRGDirectory(group, dirname);
		} else
		{
			LogManager::getSingleton().logMessage("Loading " + dirname);
			ResourceGroupManager::getSingleton().addResourceLocation(dirname, "FileSystem", rgname);
			ResourceGroupManager::getSingleton().initialiseResourceGroup(rgname);
			// parse everything
			parseKnownFilesOneRG(rgname);
			// unload it again
			LogManager::getSingleton().logMessage("UnLoading " + dirname);

#ifdef USE_OPENAL
			SoundScriptManager *ssm = SoundScriptManager::getSingleton();
			if(ssm) ssm->clearTemplates();
#endif //OPENAL
			//ParticleSystemManager::getSingleton().removeTemplatesByResourceGroup(rgname);
			ResourceGroupManager::getSingleton().clearResourceGroup(rgname);
			ResourceGroupManager::getSingleton().unloadResourceGroup(rgname);
			ResourceGroupManager::getSingleton().removeResourceLocation(dirname, rgname);
			ResourceGroupManager::getSingleton().destroyResourceGroup(rgname);
		}

	} catch(Ogre::Exception& e)
	{
		if(e.getNumber() == Ogre::Exception::ERR_DUPLICATE_ITEM)
		{
			LogManager::getSingleton().logMessage(" *** error opening directory '"+dirname+"': some files are duplicates of existing files. The directory will be ignored.");
			LogManager::getSingleton().logMessage("error while opening resource: " + e.getFullDescription());

		} else
		{
			LogManager::getSingleton().logMessage("error while loading directory: " + e.getFullDescription());
			LogManager::getSingleton().logMessage("error opening directory '"+dirname+"'");
			LogManager::getSingleton().logMessage("trying to continue ...");
		}
	}
}

void CacheSystem::loadSingleZip(String zippath, int cfactor, bool unload, bool ownGroup)
{
	String realzipPath = getRealPath(zippath);
	char hash[255];
	memset(hash, 0, 255);

	CSHA1 sha1;
	sha1.HashFile(const_cast<char*>(realzipPath.c_str()));
	sha1.Final();
	sha1.ReportHash(hash, CSHA1::REPORT_HEX_SHORT);
	zipHashes[getVirtualPath(zippath)] = String(hash);


	String compr = "";
	if(cfactor > 99)
		compr = "(No Compression)";
	else if (cfactor > 0)
		compr = "(Compression: " + StringConverter::toString(cfactor) + ")";
	LogManager::getSingleton().logMessage("Adding archive " + realzipPath + " (hash: "+String(hash)+") " + compr);

	rgcounter++;
	String rgname = "General-"+StringConverter::toString(rgcounter);

	// use general group?
	if(!ownGroup)
		rgname = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

	try
	{
		ResourceGroupManager &rgm = ResourceGroupManager::getSingleton();

		// load it into a new resource group
		LogManager::getSingleton().logMessage("Loading " + realzipPath);
		rgm.addResourceLocation(realzipPath, "Zip", rgname);
		rgm.initialiseResourceGroup(rgname);

		// parse everything
		parseKnownFilesOneRG(rgname);

		// unload it again
		if(unload)
		{
			LogManager::getSingleton().logMessage("Unloading " + realzipPath);
#ifdef USE_OPENAL
			SoundScriptManager *ssm = SoundScriptManager::getSingleton();
			if(ssm) ssm->clearTemplates();
#endif //OPENAL
			//ParticleSystemManager::getSingleton().removeTemplatesByResourceGroup(rgname);
			rgm.removeResourceLocation(realzipPath, rgname);
			rgm.clearResourceGroup(rgname);
			rgm.unloadResourceGroup(rgname);
			rgm.destroyResourceGroup(rgname);
		}
	} catch(Ogre::Exception& e)
	{
		if(e.getNumber() == Ogre::Exception::ERR_DUPLICATE_ITEM)
		{
			LogManager::getSingleton().logMessage(" *** error opening archive '"+realzipPath+"': some files are duplicates of existing files. The archive will be ignored.");
			LogManager::getSingleton().logMessage("error while opening resource: " + e.getFullDescription());

		}else
		{
			LogManager::getSingleton().logMessage("error while loading single Zip: " + e.getFullDescription());
			LogManager::getSingleton().logMessage("error opening archive '"+realzipPath+"'. Is it corrupt? Ignoring that archive ...");
			LogManager::getSingleton().logMessage("trying to continue ...");
		}
	}
}


void CacheSystem::loadAllZipsInResourceGroup(String group)
{
	std::map<String, bool> loadedZips;
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	FileInfoListPtr files = rgm.findResourceFileInfo(group, "*.zip");
	FileInfoList::iterator iterFiles = files->begin();
	int i=0, filecount=files->size();
	for (; iterFiles!= files->end(); ++iterFiles, i++)
	{
		if(loadedZips[iterFiles->filename])
		{
			LogManager::getSingleton().logMessage(" zip already loaded: " + iterFiles->filename);
			// already loaded for some strange reason
			continue;
		}
		// update loader
		int progress = ((float)i/(float)filecount)*100;
#ifdef USE_MYGUI 
		LoadingWindow::get()->setProgress(progress, _L("Loading zips in group ") + group + "\n" + iterFiles->filename + "\n" + StringConverter::toString(i) + "/" + StringConverter::toString(filecount));
#endif //MYGUI

		loadSingleZip((Ogre::FileInfo)*iterFiles);
		loadedZips[iterFiles->filename] = true;
	}
	// hide loader again
#ifdef USE_MYGUI 
	LoadingWindow::get()->hide();
#endif //MYGUI
}

void CacheSystem::loadAllDirectoriesInResourceGroup(String group)
{
	FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(group, true);
	int i=0, filecount=list->size();
	for (FileInfoList::iterator listitem = list->begin(); listitem!= list->end(); ++listitem,i++)
	{
		if(!listitem->archive) continue;
		String dirname = listitem->archive->getName() + SETTINGS.getSetting("dirsep") + listitem->filename;
		// update loader
		int progress = ((float)i/(float)filecount)*100;
#ifdef USE_MYGUI 
		LoadingWindow::get()->setProgress(progress, _L("Loading directory\n") + listitem->filename);
#endif //MYGUI
		loadSingleDirectory(dirname, group, true);
	}
	// hide loader again
#ifdef USE_MYGUI 
	LoadingWindow::get()->hide();
#endif //MYGUI
}

void CacheSystem::loadAllZips()
{
	static bool lodedalready=false;
	if(lodedalready)
		return;
	lodedalready=true;
	//setup zip packages
	//search zip in packs group
	loadAllZipsInResourceGroup("Packs");
	loadAllZipsInResourceGroup("VehicleFolders");
	loadAllZipsInResourceGroup("TerrainFolders");

	loadAllDirectoriesInResourceGroup("VehicleFolders");
	loadAllDirectoriesInResourceGroup("TerrainFolders");
}


void CacheSystem::checkForNewZipsInResourceGroup(String group)
{
	FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, "*.zip");
	FileInfoList::iterator iterFiles = files->begin();
	int i=0, filecount=files->size();
	for (; iterFiles!= files->end(); ++iterFiles, i++)
	{
		String zippath = iterFiles->archive->getName() + "\\" + iterFiles->filename;
		String zippath2="";
		if(iterFiles->archive->getName()[iterFiles->archive->getName().size()-1] != '/')
			zippath2 = iterFiles->archive->getName() + "/" + iterFiles->filename;
		else
			zippath2 = iterFiles->archive->getName() + iterFiles->filename;
		#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		//everything fine
		#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		zippath=zippath2;
		#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		zippath=zippath2;
		#endif
		int progress = ((float)i/(float)filecount)*100;
#ifdef USE_MYGUI 
		LoadingWindow::get()->setProgress(progress, _L("checking for new zips in ") + group + "\n" + iterFiles->filename + "\n" + StringConverter::toString(i) + "/" + StringConverter::toString(filecount));
#endif //MYGUI
		if(!isZipUsedInEntries(zippath2))
		{
#ifdef USE_MYGUI 
			LoadingWindow::get()->setProgress(progress, _L("checking for new zips in ") + group + "\n" + _L("loading new zip: ") + iterFiles->filename + "\n" + StringConverter::toString(i) + "/" + StringConverter::toString(filecount));
#endif //MYGUI
			LogManager::getSingleton().logMessage("- "+zippath+" is new");
			newFiles++;
			loadSingleZip((Ogre::FileInfo)*iterFiles);
		}
	}
#ifdef USE_MYGUI 
	LoadingWindow::get()->hide();
#endif //MYGUI
}

void CacheSystem::checkForNewDirectoriesInResourceGroup(String group)
{
	FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(group, true);
	int i=0, filecount=list->size();
	for (FileInfoList::iterator listitem = list->begin(); listitem!= list->end(); ++listitem, i++)
	{
		if(!listitem->archive) continue;
		String dirname = listitem->archive->getName() + SETTINGS.getSetting("dirsep") + listitem->filename;
		int progress = ((float)i/(float)filecount)*100;
#ifdef USE_MYGUI 
		LoadingWindow::get()->setProgress(progress, _L("checking for new directories in ") + group + "\n" + listitem->filename + "\n" + StringConverter::toString(i) + "/" + StringConverter::toString(filecount));
#endif // MYGUI
		if(!isDirectoryUsedInEntries(dirname))
		{
#ifdef USE_MYGUI 
			LoadingWindow::get()->setProgress(progress, _L("checking for new directories in ") + group + "\n" + _L("loading new directory: ") + listitem->filename + "\n" + StringConverter::toString(i) + "/" + StringConverter::toString(filecount));
#endif //MYGUI
			LogManager::getSingleton().logMessage("- "+dirname+" is new");
			loadSingleDirectory(dirname, group, true);
		}
	}
#ifdef USE_MYGUI 
	LoadingWindow::get()->hide();
#endif //MYGUI
}

void CacheSystem::checkForNewContent()
{
	checkForNewZipsInResourceGroup("Packs");
	checkForNewZipsInResourceGroup("VehicleFolders");
	checkForNewZipsInResourceGroup("TerrainFolders");

	checkForNewDirectoriesInResourceGroup("VehicleFolders");
	checkForNewDirectoriesInResourceGroup("TerrainFolders");
}
