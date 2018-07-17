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
//created by thomas fischer 3rd of October 2009

#include "errorutils.h"
#include "Ogre.h"
#include "language.h"


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#include <shlobj.h> 
#include <shellapi.h> // for ShellExecuteW
#endif

using namespace Ogre;

int showError(Ogre::String title, Ogre::String err)
{
	return showMsgBox(title, err, 0);
}

int showInfo(Ogre::String title, Ogre::String err)
{
	return showMsgBox(title, err, 1);
}

int showMsgBox(Ogre::String title, Ogre::String err, int type)
{
	// we might call the showMsgBox without having ogre created yet!
	//Ogre::LogManager::getSingleton().logMessage("message box: " + title + ": " + err);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	int mtype = MB_ICONERROR;
	if(type == 1) mtype = MB_ICONINFORMATION;
	MessageBoxA( NULL, err.c_str(), title.c_str(), MB_OK | mtype | MB_TOPMOST);
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	printf("\n\n%s: %s\n\n", title.c_str(), err.c_str());
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	printf("\n\n%s: %s\n\n", title.c_str(), err.c_str());
	//CFOptionFlags flgs;
	//CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, T("A network error occured"), T("Bad server port."), NULL, NULL, NULL, &flgs);
#endif
	return 0;
}

int showWebError(Ogre::String title, Ogre::String err, Ogre::String url)
{
	// NO logmanager use, because it could be that its not initialized yet!
	//Ogre::LogManager::getSingleton().logMessage("web message box: " + title + ": " + err + " / url: " + url);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	Ogre::String additional = _L("\n\nYou can eventually get help here:\n\n") + url + _L("\n\nDo you want to open that address in your default browser now?");
	err += additional;
	int Response = MessageBoxA( NULL, err.c_str(), title.c_str(), MB_YESNO | MB_ICONERROR | MB_TOPMOST);
	// 6 = yes, 7 = no
	if(Response == 6)
	{
		// Microsoft conversion hell follows :|
		char tmp[255], tmp2[255];
		wchar_t ws1[255], ws2[255];
		strncpy(tmp, "open", 255);
		mbstowcs(ws1, tmp, 255);
		strncpy(tmp2, url.c_str(), 255);
		mbstowcs(ws2, tmp2, 255);
		ShellExecuteW(NULL, ws1, ws2, NULL, NULL, SW_SHOWNORMAL);
	}
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	printf("\n\n%s: %s / url: %s\n\n", title.c_str(), err.c_str(), url.c_str());
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	printf("\n\n%s: %s / url: %s\n\n", title.c_str(), err.c_str(), url.c_str());
	//CFOptionFlags flgs;
	//CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, "An exception has occured!", err.c_str(), NULL, NULL, NULL, &flgs);
#endif
	return 0;
}