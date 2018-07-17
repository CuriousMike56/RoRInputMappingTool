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

#ifdef USE_MYGUI 

#ifndef __MAP_CONTROL_H__
#define __MAP_CONTROL_H__

#include <Ogre.h>
#include "OgreTextAreaOverlayElement.h"
#include <vector>
#include <MyGUI.h>
#include <BaseLayout.h>
#include "rormemory.h"

class MapEntity;

ATTRIBUTE_CLASS_LAYOUT(MapControl, "MapControl.layout");
class MapControl :
	public wraps::BaseLayout,
	public MemoryAllocatedObject
{
public:
	MapControl(int mapsizex, int mapsizez);
	~MapControl();

	void setMapTexture(Ogre::String _name);

	MapEntity *createNamedMapEntity(Ogre::String name, Ogre::String type);
	MapEntity *createMapEntity(Ogre::String type);
	MapEntity *getEntityByName(Ogre::String name);
	void deleteMapEntity(MapEntity *ent);
	bool getVisibility();
	void setVisibility(bool value);

	void setPosition(float x, float y, float w, float h, Ogre::RenderWindow* rw=0);
	void setPosition(float x, float y, float w, Ogre::RenderWindow* rw=0);
	void resizeToScreenRatio(Ogre::RenderWindow* win);

	void updateRenderMetrics(Ogre::RenderWindow* win);

	static Ogre::String getTypeByDriveable(int driveable);
	float getAlpha() { return mAlpha; }
	void setAlpha(float value);
	void setEntitiesVisibility(bool value);
	void setWorldSize(int x, int z);
	Ogre::Vector2 getMapSize(){ return Ogre::Vector2(mapsizex, mapsizez); }
	float getWindowScale() { return myScale; }

protected:
	ATTRIBUTE_FIELD_WIDGET_NAME(MapControl, mMapTexture, "mMapTexture");
	MyGUI::StaticImage* mMapTexture;

	std::vector<MapEntity *> mMapEntities;
	std::map<Ogre::String, MapEntity *> mNamedEntities;
	int mapsizex, mapsizez;
	float x,y,w,h, myScale;
	float mAlpha;
	unsigned int rWinWidth, rWinHeight, rWinDepth;
	int rWinLeft, rWinTop;

	void updateEntityPositions();

};

#endif // __MAP_CONTROL_H__


#endif // MYGUI
