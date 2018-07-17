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

#ifndef __CmdKeyInertia_H__
#define __CmdKeyInertia_H__
#include "OgrePrerequisites.h"
#include "rormemory.h"

class CmdKeyInertia : public MemoryAllocatedObject
{
	
public:
	CmdKeyInertia(int maxCmdKeys);
	~CmdKeyInertia();

	Ogre::Real calcCmdKeyDelay(Ogre::Real cmdInput,int cmdKey, Ogre::Real dt);
	int setCmdKeyDelay(int number,Ogre::Real startDelay,Ogre::Real stopDelay,Ogre::String startFunction, Ogre::String stopFunction);
	void resetCmdKeyDelay(int maxCmdKeys);
protected: 

	struct cmdKeyInertia_s
	{
		Ogre::Real startDelay;
		Ogre::Real stopDelay;
		Ogre::Real lastOutput;
		Ogre::SimpleSpline *startSpline;
		Ogre::SimpleSpline *stopSpline;
		Ogre::Real time;
	};

	Ogre::Real calculateCmdOutput(Ogre::Real time,Ogre::SimpleSpline *spline);
	int processLine(Ogre::vector< Ogre::String >::type args,  Ogre::String model);

	cmdKeyInertia_s *cmdKeyInertia;
	std::map < Ogre::String, Ogre::SimpleSpline > splines;
	int loadDefaultInertiaModels();

};
#endif
