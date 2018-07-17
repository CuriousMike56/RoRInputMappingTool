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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 12th of October 2009

#ifndef GUI_DUSTMANAGER_H__
#define GUI_DUSTMANAGER_H__

#include "OgreSingleton.h"
#include "OgrePrerequisites.h"
#include "RoRPrerequisites.h"
#include "rormemory.h"


class DustManager : public Ogre::Singleton< DustManager >, public MemoryAllocatedObject
{
public:
	DustManager(Ogre::SceneManager *mSceneMgr);
	~DustManager();
	static DustManager& getSingleton(void);
	static DustManager* getSingletonPtr(void);

	DustPool *getGroundModelDustPool(ground_model_t *g);
	
	void update(float wspeed);
	void setWater(Water *w);
	void setVisible(bool visible);

	DustPool *getDustPool(Ogre::String name);
	
protected:
	Ogre::SceneManager *mSceneMgr;
	Water *w;
	bool mEnabled;
	std::map < Ogre::String , DustPool * > dustpools;
	//void addNewDustPool(ground_model_t *g);
};

#endif //GUI_FRICTION_H__
