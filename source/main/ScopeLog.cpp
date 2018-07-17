#include "ScopeLog.h"
#include "Ogre.h"
#include "Settings.h"

#include <stdio.h> // for remove

// created by thomas fischer thomas{AT}thomasfischer{DOT}biz, 18th of Juli 2009

using namespace Ogre;

ScopeLog::ScopeLog(String name) : orgLog(0), logFileName()
{
	if(SETTINGS.getSetting("Custom Logs") != "Yes") return;
	orgLog = LogManager::getSingleton().getDefaultLog();

	logFileName = SETTINGS.getSetting("Log Path") + SETTINGS.getSetting("dirsep") + name + ".log";
	LogManager::getSingleton().createLog(logFileName, true);
}

ScopeLog::~ScopeLog()
{
	if(orgLog)
	{
		LogManager::getSingleton().setDefaultLog(orgLog);
		if(getFileSize(logFileName) == 0)
		{
			// if the file is empty, remove it.
			remove(logFileName.c_str());
		}
	}
}

int ScopeLog::getFileSize(String filename)
{
	FILE *f = fopen(filename.c_str(), "rb");
	if(f != NULL)
	{
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fclose(f);
		return size;
	}
	return 0;
}