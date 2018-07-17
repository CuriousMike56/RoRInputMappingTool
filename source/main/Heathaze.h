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
#ifndef __HeatHaze_H__
#define __HeatHaze_H__

#include "Ogre.h"
#include "rormemory.h"

class HeatHazeListener : public Ogre::RenderTargetListener, public MemoryAllocatedObject
{
public:
	HeatHazeListener(Ogre::SceneManager *mSceneMgr);
    void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
private:
	Ogre::SceneManager *mSceneMgr;
};

class HeatHaze : public MemoryAllocatedObject
{
public:
	HeatHaze(Ogre::SceneManager *mSceneMgr, Ogre::RenderWindow *mWindow, Ogre::Camera *cam);
	void setEnable(bool en);
	void prepareShutdown();
	void update();
private:
	HeatHazeListener *listener;
	Ogre::Camera *mHazeCam;
	Ogre::TextureUnitState *tex;
	Ogre::RenderTexture* rttTex;
	Ogre::SceneManager *mSceneMgr;	
};

#endif
