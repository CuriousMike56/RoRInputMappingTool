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
#include "RigsOfRods.h"
#include "language.h"
#include "errorutils.h"

Ogre::String getVersionString()
{
	char tmp[1024] = "";
	sprintf(tmp, "Rigs of Rods\n"
		" version: %s\n"
		" revision: %s\n"
		" full revision: %s\n"
		" protocol version: %s\n"
		" build time: %s, %s\n"
		, ROR_VERSION_STRING, SVN_REVISION, SVN_ID, RORNET_VERSION, __DATE__, __TIME__);
	return Ogre::String(tmp);
}

#ifdef USE_CRASHRPT
// see http://code.google.com/p/crashrpt/
#include "crashrpt.h"

// Define the crash callback
BOOL WINAPI crashCallback(LPVOID /*lpvState*/)
{
	// Now add these two files to the error report
	
	// logs
	crAddFile((SETTINGS.getSetting("Log Path") + "RoR.log").c_str(), "Rigs of Rods Log");
	crAddFile((SETTINGS.getSetting("Log Path") + "mygui.log").c_str(), "Rigs of Rods GUI Log");
	crAddFile((SETTINGS.getSetting("Log Path") + "configlog.txt").c_str(), "Rigs of Rods Configurator Log");
	crAddFile((SETTINGS.getSetting("Program Path") + "wizard.log").c_str(), "Rigs of Rods Installer Log");

	// cache
	crAddFile((SETTINGS.getSetting("Cache Path") + "mods.cache").c_str(), "Rigs of Rods Cache File");

	// configs
	crAddFile((SETTINGS.getSetting("Config Root") + "ground_models.cfg").c_str(), "Rigs of Rods Ground Configuration");
	crAddFile((SETTINGS.getSetting("Config Root") + "input.map").c_str(), "Rigs of Rods Input Configuration");
	crAddFile((SETTINGS.getSetting("Config Root") + "ogre.cfg").c_str(), "Rigs of Rods Renderer Configuration");
	crAddFile((SETTINGS.getSetting("Config Root") + "RoR.cfg").c_str(), "Rigs of Rods Configuration");

	crAddProperty("Version", ROR_VERSION_STRING);
	crAddProperty("Revision", SVN_REVISION);
	crAddProperty("full_revision", SVN_ID);
	crAddProperty("protocol_version", RORNET_VERSION);
	crAddProperty("build_date", __DATE__);
	crAddProperty("build_time", __TIME__);

	crAddProperty("System_GUID", SETTINGS.getSetting("GUID").c_str());
	crAddProperty("Multiplayer", (SETTINGS.getSetting("Network enable")=="Yes")?"1":"0");
	
	crAddScreenshot(CR_AS_MAIN_WINDOW);
	// Return TRUE to allow crash report generation
	return TRUE;
}

void install_crashrpt()
{
	// Install CrashRpt support
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);  
	info.pszAppName = "Rigs of Rods";
	info.pszAppVersion = ROR_VERSION_STRING;
	info.pszEmailSubject = "Error Report for Rigs of Rods";
	info.pszEmailTo = "thomas@rigsofrods.com";

	char tmp[512]="";
	sprintf(tmp, "http://api.rigsofrods.com/crashreport/?version=%s_%s", __DATE__, __TIME__);
	for(unsigned int i=0;i<strnlen(tmp, 512);i++)
	{
		if(tmp[i] == ' ')
			tmp[i] = '_';
	}

	info.pszUrl = tmp;
	info.pfnCrashCallback = crashCallback; 
	info.uPriorities[CR_HTTP]  = 3;  // Try HTTP the first
	info.uPriorities[CR_SMTP]  = 2;  // Try SMTP the second
	info.uPriorities[CR_SMAPI] = 1; // Try Simple MAPI the last  
	info.dwFlags = 0; // Install all available exception handlers
	info.pszPrivacyPolicyURL = "http://wiki.rigsofrods.com/pages/Crash_Report_Privacy_Policy"; // URL for the Privacy Policy link

	int nInstResult = crInstall(&info);
	if(nInstResult!=0)
	{
		// Something goes wrong!
		TCHAR szErrorMsg[512];
		szErrorMsg[0]=0;

		crGetLastErrorMsg(szErrorMsg, 512);
		printf("%s\n", szErrorMsg);


		showError("Exception handling registration problem", String(szErrorMsg));

		assert(nInstResult==0);
	}
}

void uninstall_crashrpt()
{
	// Unset crash handlers
	int nUninstResult = crUninstall();
	assert(nUninstResult==0);
}

void test_crashrpt()
{
	// emulate null pointer exception (access violation)
	crEmulateCrash(CR_WIN32_STRUCTURED_EXCEPTION);
}
#endif

// simpleopt by http://code.jellycan.com/simpleopt/
// license: MIT
#include "SimpleOpt.h"

#define HELPTEXT "--help (this)\n-map <map> (loads map on startup)\n-truck <truck> (loads truck on startup)\n-setup shows the ogre configurator\n-version shows the version information\n-enter enters the selected truck\n-userpath <path> sets the user directory\nFor example: RoR.exe -map oahu -truck semi"

// option identifiers
enum { OPT_HELP, OPT_MAP, OPT_TRUCK, OPT_SETUP, OPT_CMD, OPT_WDIR, OPT_ETM, OPT_BUILD, OPT_CONFIG, OPT_VER, OPT_CHECKCACHE, OPT_TRUCKCONFIG, OPT_ENTERTRUCK, OPT_BENCH, OPT_STREAMCACHEGEN, OPT_BENCHNUM, OPT_USERPATH, OPT_BENCHPOS, OPT_BENCHPOSERR, OPT_NOCRASHCRPT};

// option array
CSimpleOpt::SOption cmdline_options[] = {
	{ OPT_MAP,            ((char *)"-map"),         SO_REQ_SEP },
	{ OPT_MAP,            ((char *)"-terrain"),     SO_REQ_SEP },
	{ OPT_TRUCK,          ((char *)"-truck"),       SO_REQ_SEP },
	{ OPT_ENTERTRUCK,     ((char *)"-enter"),       SO_NONE },
	{ OPT_CMD,            ((char *)"-cmd"),         SO_REQ_SEP },
	{ OPT_WDIR,           ((char *)"-wd"),          SO_REQ_SEP },
	{ OPT_SETUP,          ((char *)"-setup"),       SO_NONE    },
	{ OPT_CONFIG,         ((char *)"-config"),      SO_NONE    },
	{ OPT_TRUCKCONFIG,    ((char *)"-truckconfig"), SO_REQ_SEP    },
	{ OPT_BUILD,          ((char *)"-build"),       SO_NONE    },
	{ OPT_HELP,           ((char *)"--help"),       SO_NONE    },
	{ OPT_HELP,           ((char *)"-help"),        SO_NONE    },
	{ OPT_CHECKCACHE,     ((char *)"-checkcache"),  SO_NONE    },
	{ OPT_VER,            ((char *)"-version"),     SO_NONE    },
	{ OPT_USERPATH,       ((char *)"-userpath"),   SO_REQ_SEP    },
	{ OPT_BENCH,          ((char *)"-benchmark"),   SO_REQ_SEP    },
	{ OPT_BENCHPOS,       ((char *)"-benchmark-final-position"),   SO_REQ_SEP    },
	{ OPT_BENCHPOSERR,    ((char *)"-benchmark-final-position-error"),   SO_REQ_SEP    },
	{ OPT_BENCHNUM,       ((char *)"-benchmarktrucks"),       SO_REQ_SEP },
	{ OPT_BENCHNUM,       ((char *)"-benchmark-trucks"),       SO_REQ_SEP },
	{ OPT_STREAMCACHEGEN, ((char *)"-streamcachegen"),   SO_NONE    },
	{ OPT_NOCRASHCRPT,    ((char *)"-nocrashrpt"),   SO_NONE    },
SO_END_OF_OPTIONS
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX // required to stop windows.h messing up std::min
#endif //NOMINMAX
#include "windows.h"
#include "ShellAPI.h"
#endif //OGRE_PLATFORM_WIN32

#ifdef __cplusplus
extern "C" {
#endif

void showUsage()
{
	showInfo(_L("Command Line Arguments"), HELPTEXT);
}

void showVersion()
{
	showInfo(_L("Version Information"), getVersionString());
#ifdef __GNUC__
	printf(" * built with gcc %d.%d.%d\n", __GNUC_MINOR__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif //__GNUC__
}

int main(int argc, char *argv[])
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	//start working dir is highly unpredictable in MacOSX (typically you start in "/"!)
	//oh, thats quite hacked - thomas
	char str[256];
	strcpy(str, argv[0]);
	char *pt=str+strlen(str);
	while (*pt!='/') pt--;
	*pt=0;
	chdir(str);
	chdir("../../..");
	getwd(str);
	printf("GETWD=%s\n", str);
#endif

	// Create application object
	RigsOfRods app;
	app.buildmode=false;

//MacOSX adds an extra argument in the for of -psn_0_XXXXXX when the app is double clicked
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE
	CSimpleOpt args(argc, argv, cmdline_options);
	while (args.Next()) {
		if (args.LastError() == SO_SUCCESS) {
			if (args.OptionId() == OPT_HELP) {
				showUsage();
				return 0;
			} else if (args.OptionId() == OPT_TRUCK) {
				SETTINGS.setSetting("Preselected Truck", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_TRUCKCONFIG) {
				SETTINGS.setSetting("Preselected TruckConfig", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_MAP) {
				SETTINGS.setSetting("Preselected Map", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_CMD) {
				SETTINGS.setSetting("cmdline CMD", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCH) {
				SETTINGS.setSetting("Benchmark", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHNUM) {
				SETTINGS.setSetting("BenchmarkTrucks", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHPOS) {
				SETTINGS.setSetting("BenchmarkFinalPosition", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_BENCHPOSERR) {
				SETTINGS.setSetting("BenchmarkFinalPositionError", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_NOCRASHCRPT) {
				SETTINGS.setSetting("NoCrashRpt", "Yes");
			} else if (args.OptionId() == OPT_USERPATH) {
				SETTINGS.setSetting("userpath", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_CONFIG) {
				SETTINGS.setSetting("configfile", String(args.OptionArg()));
			} else if (args.OptionId() == OPT_WDIR) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				SetCurrentDirectory(args.OptionArg());
#endif
			} else if (args.OptionId() == OPT_STREAMCACHEGEN) {
				SETTINGS.setSetting("streamCacheGenerationOnly", "Yes");
			} else if (args.OptionId() == OPT_CHECKCACHE) {
				// just regen cache and exit
				SETTINGS.setSetting("regen-cache-only", "True");
			} else if (args.OptionId() == OPT_ENTERTRUCK) {
				SETTINGS.setSetting("Enter Preselected Truck", "Yes");
			} else if (args.OptionId() == OPT_SETUP) {
				app.useogreconfig = true;
			} else if (args.OptionId() == OPT_BUILD) {
				app.buildmode = true;
			} else if (args.OptionId() == OPT_VER) {
				showVersion();
				return 0;
			}
		} else {
			showUsage();
			return 1;
		}
	}
#endif

#ifdef USE_CRASHRPT
	if(SETTINGS.getSetting("NoCrashRpt").empty())
		install_crashrpt();

	//test_crashrpt();
#endif //USE_CRASHRPT

	try {
		app.go();
	} catch(Ogre::Exception& e)
	{
		// try to shutdown input system upon an error
		if(InputEngine::instanceExists()) // this prevents the creating of it, if not existing
			INPUTENGINE.prepareShutdown();

		String url = "http://wiki.rigsofrods.com/index.php?title=Error_" + StringConverter::toString(e.getNumber())+"#"+e.getSource();
		showWebError("An exception has occured!", e.getFullDescription(), url);
		return 1;
	}

#ifdef USE_CRASHRPT
	if(SETTINGS.getSetting("NoCrashRpt").empty())
		uninstall_crashrpt();
#endif //USE_CRASHRPT

	return 0;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
	return main(__argc, __argv);
}
#endif


#ifdef __cplusplus
}
#endif
