/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009,2010 Pierre-Michel Ricordel
Copyright 2007,2008,2009,2010 Thomas Fischer

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

#ifdef USE_CAELUM

#ifndef SKYMANAGER_H__
#define SKYMANAGER_H__

#include "OgrePrerequisites.h"
#include "CaelumPrerequisites.h"
#include "OgreSingleton.h"

class SkyManager : public Ogre::Singleton< SkyManager >
{
public:
	SkyManager();
	~SkyManager();

	static SkyManager& getSingleton(void);
	static SkyManager* getSingletonPtr(void);

	void init(Ogre::SceneManager *mScene, Ogre::RenderWindow *mWindow, Ogre::Camera *mCamera);

	void loadScript(Ogre::String script);
	
	/// change the time scale
	void setTimeFactor(double f);
	Ogre::Light *getMainLight();
	/// gets the current time scale
	double getTimeFactor();
	
	/// prints the current time of the simulation inthe format of HH:MM:SS
	Ogre::String getPrettyTime();
	

protected:
    Caelum::CaelumSystem *mCaelumSystem;
	Caelum::CaelumSystem *getCaelumSystem() { return mCaelumSystem; };
};

#endif //SKYMANAGER_H__

#endif //USE_CAELUM
