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
// created by thomas{AT}thomasfischer{DOT}biz, 5th of July 2010
#ifndef SHADOWMANAGER_H__
#define SHADOWMANAGER_H__

#include "OgrePrerequisites.h"
#ifdef USE_CAELUM
#include "CaelumPrerequisites.h"
#endif // USE_CAELUM
#include "OgreCommon.h"
#include "OgreSingleton.h"
#include "OgreShadowCameraSetup.h"

class ShadowManager : public Ogre::Singleton< ShadowManager >
{
public:
	ShadowManager(Ogre::SceneManager *mScene, Ogre::RenderWindow *mWindow, Ogre::Camera *mCamera);
	~ShadowManager();

	static ShadowManager& getSingleton(void);
	static ShadowManager* getSingletonPtr(void);

	int changeShadowTechnique(Ogre::ShadowTechnique tech);
	void loadConfiguration();

protected:
	Ogre::SceneManager *mSceneMgr;
	Ogre::RenderWindow *mWindow;
	Ogre::Camera *mCamera;
	Ogre::ShadowCameraSetupPtr mPSSMSetup;
};

#endif //SHADOWMANAGER_H__
