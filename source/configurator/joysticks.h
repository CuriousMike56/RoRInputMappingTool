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

#ifndef __Joysticks_H__
#define __Joysticks_H__

#include "Ogre.h"
#include "OISInputManager.h"
#include "OISJoyStick.h"
#include "OISInterface.h"
#include "OISForceFeedback.h"

typedef struct
{
	char name[256];
	int joy;
	int axis;
	int value;
} axis_t;

class Joysticks
{
private:
char defaultX[256];
char defaultY[256];
int numjoy;
OIS::InputManager *mInputManager;
OIS::JoyStick* joys[64];
OIS::ForceFeedback* ffs[64];
int free_axis;
axis_t axis[256];

public:
	Joysticks(void* mhwnd)
	{
		strcpy(defaultX, "Keyboard");
		strcpy(defaultY, "Keyboard");
		free_axis=0;

//		OIS::ParamList pl;
//		std::ostringstream windowHndStr;
//		windowHndStr << mhwnd;
//		pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
//		mInputManager = OIS::InputManager::createInputSystem( pl );
//		char shndl[64];
//		sprintf(shndl, "%d", (long)mhwnd);
//		mInputManager = OIS::InputManager::createInputSystem(std::make_pair( std::string("WINDOW"), shndl));
		mInputManager = OIS::InputManager::createInputSystem( (size_t)mhwnd );
		//there is an inconsistency in OIS between numJoySticks and numJoysticks (note the S vs s)
#if __WIN32__
		numjoy=mInputManager->numJoySticks();
#endif
#if __MACOSX__
		numjoy=mInputManager->numJoySticks();
#endif
#if __LINUX__
		numjoy=mInputManager->numJoysticks();
#endif
		int j,a;
		for (j=0; j<numjoy; j++)
		{
			joys[j] = static_cast<OIS::JoyStick*>(mInputManager->createInputObject(OIS::OISJoyStick, false));
			ffs[j]=0;
			//ffs[j] = static_cast<OIS::ForceFeedback*>(joys[j]->queryInterface(OIS::Interface::ForceFeedback));
			for (a=0; a<joys[j]->axes(); a++)
			{
				//we have a new axis
				char name[256];
				sprintf(name, "joy%i/axis%i", j,a);
				if (j==0 && a==0) strcpy(defaultX, name);
				if (j==0 && a==1) strcpy(defaultY, name);
				axis[free_axis].joy=j;
				axis[free_axis].axis=a;
				strcpy(axis[free_axis].name, name);
				axis[free_axis].value=0;
				free_axis++;
			}
		}
	}

	int getNumaxis() {return free_axis;}

	char *getAxisDesc(int pos) {return axis[pos].name;}

	int findJoy(const char* name)
	{
		int i;
		for (i=0; i<free_axis; i++) if (!strcmp(name, axis[i].name)) return i;
		return -1;
	}

	char *getDefaultX() {return defaultX;}
	char *getDefaultY() {return defaultY;}
	bool isAxisStickAble(int axisnum) {return ffs[axis[axisnum].joy]!=0 && ffs[axis[axisnum].joy]->getFFAxesNumber()>=2 && (axis[axisnum].axis==0 || axis[axisnum].axis==1);}
	int getStickPair(int axisnum)
	{
		if (!isAxisStickAble(axisnum)) return -1;
		int i;
		for (i=0; i<free_axis; i++)
		{
			if (axis[axisnum].joy==axis[i].joy)
			{
				if (axis[axisnum].joy==0 && axis[i].joy==1) return i;
				if (axis[axisnum].joy==1 && axis[i].joy==0) return i;
			}
		}
		return -1;
	}

void UpdateInputState( )
{
	int j,k;
//	OIS::JoyStickState jstate[64];
	for (j=0; j<numjoy; j++)
	{
		joys[j]->capture();
//		jstate[j]=joys[j]->getJoyStickState();
	}
	for (k=0; k<free_axis; k++)
	{
//			axis[k].value=jstate[axis[k].joy].mAxes[axis[k].axis].abs;
		axis[k].value=joys[axis[k].joy]->getJoyStickState().mAxes[axis[k].axis].abs;
	}
}

	int getValue(int axisnum)
	{
		return axis[axisnum].value;
	}

	~Joysticks()
	{
	}

};

#endif
