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
// created by thomas fischer, 4th of January 2009
#include "Settings.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#include <shlobj.h>
#endif
#include "language.h"
#include "errorutils.h"

using namespace std;
using namespace Ogre;

// singleton pattern
Settings* Settings::myInstance = 0;

Settings & Settings::Instance ()
{
	if (myInstance == 0)
		myInstance = new Settings;
	return *myInstance;
}

Settings::Settings()
{
}

Settings::~Settings()
{
}

Ogre::String Settings::getSetting(Ogre::String key)
{
	return settings[key];
}

Ogre::String Settings::getSettingScriptSafe(Ogre::String key)
{
	// hide certain settings for scripts
	if(key == "User Token" || key == "Config Root" || key == "Cache Path" || key == "Log Path" || key == "Resources Path" || key == "Streams Path" || key == "Program Path")
		return "";

	return settings[key];
}

void Settings::setSetting(Ogre::String key, Ogre::String value)
{
	settings[key] = value;
}

void Settings::checkGUID()
{
	if(getSetting("GUID").empty())
		createGUID();
}

void Settings::createGUID()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	GUID *g = new GUID();
	CoCreateGuid(g);

	char buf[120];
	sprintf(buf,"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", g->Data1,g->Data2,g->Data3,UINT(g->Data4[0]),UINT(g->Data4[1]),UINT(g->Data4[2]),UINT(g->Data4[3]),UINT(g->Data4[4]),UINT(g->Data4[5]),UINT(g->Data4[6]),UINT(g->Data4[7]));
	delete g;

	String guid = String(buf);

	// save in settings
	setSetting("GUID", guid);
	saveSettings();

#endif //OGRE_PLATFORM
}

void Settings::saveSettings()
{
	saveSettings(getSetting("Config Root")+"RoR.cfg");
}

void Settings::saveSettings(Ogre::String configFile)
{
	//printf("saving Config to file: %s\n", configFile.c_str());
	FILE *fd = fopen(configFile.c_str(), "w");
	if (!fd)
		return;

	// now save the settings to RoR.cfg
	std::map<std::string, std::string>::iterator it;
	for(it = settings.begin(); it != settings.end(); it++)
	{
		fprintf(fd, "%s=%s\n", it->first.c_str(), it->second.c_str());
	}

	fclose(fd);
}

void Settings::loadSettings(Ogre::String configFile, bool overwrite)
{
	//printf("trying to load configfile: %s...\n", configFile.c_str());
	Ogre::ConfigFile cfg;
	cfg.load(configFile, "=:\t", false);

	// load all settings into a map!
	Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
	Ogre::String svalue, sname;
	while (i.hasMoreElements())
	{
		sname = i.peekNextKey();
		svalue = i.getNext();
		if(!overwrite && settings[sname] != "") continue;
		settings[sname] = svalue;
		//logfile->AddLine(conv("### ") + conv(sname) + conv(" : ") + conv(svalue));logfile->Write();
	}
	// add a GUID if not there
	checkGUID();
}

bool Settings::get_system_paths(char *program_path, char *user_path)
{

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// note: we enforce usage of the non-UNICODE interfaces (since its easier to integrate here)
	if (!GetModuleFileNameA(NULL, program_path, 512))
	{
		showError(_L("Startup error"), _L("Error while retrieving program space path"));
		return false;
	}
	GetShortPathNameA(program_path, program_path, 512); //this is legal
	path_descend(program_path);

	if (getSetting("userpath").empty())
	{
		if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, user_path)!=S_OK)
		{
			showError(_L("Startup error"), _L("Error while retrieving user space path"));
			return false;
		}
		GetShortPathNameA(user_path, user_path, 512); //this is legal
		sprintf(user_path, "%s\\Rigs of Rods\\", user_path);
	} else
	{
		strcpy(user_path, getSetting("userpath").c_str());
	}

#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	//true program path is impossible to get from POSIX functions
	//lets hack!
	pid_t pid = getpid();
	char procpath[256];
	sprintf(procpath, "/proc/%d/exe", pid);
	int ch = readlink(procpath,program_path,240);
	if (ch != -1)
	{
		program_path[ch] = 0;
		path_descend(program_path);
	} else return false;
	//user path is easy
	char home_path[255];
	strncpy(home_path, getenv ("HOME"), 240);
	//sprintf(user_path, "%s/RigsOfRods/", home_path); // old version
	sprintf(user_path, "%s/.rigsofrods/", home_path);
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	//found this code, will look later
	std::string path = "./";
	ProcessSerialNumber PSN;
	ProcessInfoRec pinfo;
	FSSpec pspec;
	FSRef fsr;
	OSStatus err;
	/* set up process serial number */
	PSN.highLongOfPSN = 0;
	PSN.lowLongOfPSN = kCurrentProcess;
	/* set up info block */
	pinfo.processInfoLength = sizeof(pinfo);
	pinfo.processName = NULL;
	pinfo.processAppSpec = &pspec;
	/* grab the vrefnum and directory */
	
	//path = "~/RigsOfRods/";
	//strcpy(user_path, path.c_str());
	
	err = GetProcessInformation(&PSN, &pinfo);
	if (! err ) {
		char c_path[2048];
		FSSpec fss2;
		int tocopy;
		err = FSMakeFSSpec(pspec.vRefNum, pspec.parID, 0, &fss2);
		if ( ! err ) {
			err = FSpMakeFSRef(&fss2, &fsr);
			if ( ! err ) {
				err = (OSErr)FSRefMakePath(&fsr, (UInt8*)c_path, 2048);
				if (! err ) {
					path = c_path;
					path += "/";
					strcpy(program_path, path.c_str());
				}
				
				err = FSFindFolder(kOnAppropriateDisk, kDocumentsFolderType, kDontCreateFolder, &fsr);
				if (! err ) {
					FSRefMakePath(&fsr, (UInt8*)c_path, 2048);
					if (! err ) {
						path = c_path;
						path += "/Rigs\ of\ Rods/";
						strcpy(user_path, path.c_str());
					}
				}
			}
		}
	}
#endif
	return true;
}

bool Settings::setupPaths()
{
	char program_path[1024]="";
	char resources_path[1024]="";
	char streams_path[1024]="";
	char user_path[1024]="";
	char config_root[1024]="";
	char dirsep='/';

	if(!get_system_paths(program_path, user_path))
		return false;

	//NEXT, derive the resources and stream paths from the base paths (depends on configuration)
	//from now we are mostly platform neutral
	//default mode (released)
	strcpy(resources_path, program_path);
	path_add(resources_path, "resources");
	strcpy(streams_path, program_path);
	path_add(streams_path, "streams");

	//setup config files names
	char plugins_fname[1024];
	strcpy(plugins_fname, program_path);
	strcat(plugins_fname, "plugins.cfg");

	char ogreconf_fname[1024];
	strcpy(ogreconf_fname, user_path);
	path_add(ogreconf_fname, "config");
	strcpy(config_root, ogreconf_fname); //setting the config root here
	strcat(ogreconf_fname, "ogre.cfg");

	char ogrelog_fname[1024];
	strcpy(ogrelog_fname, user_path);
	path_add(ogrelog_fname, "logs");

	char ogrelog_path[1024];
	strcpy(ogrelog_path, ogrelog_fname);
	strcat(ogrelog_fname, "RoR.log");

	// now update our settings with the results:
	String dsStr = "\\";
#if OGRE_PLATFORM != OGRE_PLATFORM_WIN32
	dsStr="/";
#endif
	settings["dirsep"] = dsStr;
	settings["Config Root"] = String(config_root);
	settings["Cache Path"] = String(user_path) + "cache" + dsStr;
	settings["Log Path"] = String(ogrelog_path);
	settings["Resources Path"] = String(resources_path);
	settings["Streams Path"] = String(streams_path);
	settings["User Path"] = String(user_path);
	settings["Program Path"] = String(program_path);
	settings["plugins.cfg"] = String(plugins_fname);
	settings["ogre.cfg"] = String(ogreconf_fname);
	settings["ogre.log"] = String(ogrelog_fname);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// XXX maybe use StringUtil::standardisePath here?
	// windows is case insensitive, so norm here
	StringUtil::toLowerCase(settings["Config Root"]);
	StringUtil::toLowerCase(settings["Cache Path"]);
	StringUtil::toLowerCase(settings["Log Path"]);
	StringUtil::toLowerCase(settings["Resources Path"]);
	StringUtil::toLowerCase(settings["Streams Path"]);
	StringUtil::toLowerCase(settings["Program Path"]);
#endif
	// now enable the user to override that:
	try
	{
		String configfile = settings["configfile"];
		if(!configfile.size()) configfile = "config.cfg";
		loadSettings(configfile, true);
	} catch(...)
	{
		// file not found or other error
	}

	printf(" * config path:      %s\n", settings["Config Root"].c_str());
	printf(" * user path:        %s\n", settings["User Path"].c_str());
	printf(" * program path:     %s\n", settings["Program Path"].c_str());
	printf(" * used plugins.cfg: %s\n", settings["plugins.cfg"].c_str());
	printf(" * used ogre.cfg:    %s\n", settings["ogre.cfg"].c_str());
	printf(" * used ogre.log:    %s\n", settings["ogre.log"].c_str());

	return true;
}


void Settings::path_descend(char* path)
{
	char dirsep='/';
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep='\\';
#endif
	char* pt=path+strlen(path)-1;
	if (pt>=path && *pt==dirsep) pt--;
	while (pt>=path && *pt!=dirsep) pt--;
	if (pt>=path) *(pt+1)=0;
}

void Settings::path_add(char* path, const char* dirname)
{
	char tmp[1024];
	char dirsep='/';
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep='\\';
#endif
	sprintf(tmp, "%s%s%c", path, dirname, dirsep);
	strcpy(path, tmp);
}
