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

#include "MapControl.h"
#include "Beam.h"


#include "RoRFrameListener.h"

#include "MapEntity.h"

MapControl::MapControl(int _mapsizex, int _mapsizez) : x(0),y(0),w(100),h(100)
{
	initialiseByAttributes(this);

	mapsizex=_mapsizex;
	mapsizez=_mapsizez;
	rWinWidth=1;
	rWinHeight=1;
	mAlpha = 1.0;
}

MapControl::~MapControl()
{
}

void MapControl::setMapTexture(Ogre::String _name)
{
	mMapTexture->setImageTexture(_name);
}

void MapControl::setWorldSize(int x, int z)
{
	this->mapsizex = x;
	this->mapsizez = z;
}

MapEntity *MapControl::createMapEntity(String type)
{
	MapEntity *m = new MapEntity(this, type, mMapTexture);
	mMapEntities.push_back(m);
	return m;
}

MapEntity *MapControl::createNamedMapEntity(Ogre::String name, Ogre::String type)
{
	MapEntity *e = createMapEntity(type);
	mNamedEntities[name] = e;
	return e;
}

MapEntity *MapControl::getEntityByName(Ogre::String name)
{
	if (mNamedEntities.find(name) != mNamedEntities.end())
		return mNamedEntities[name];

	return nullptr;
}

Ogre::String MapControl::getTypeByDriveable(int driveable)
{
	if(driveable == NOT_DRIVEABLE)
		return "load";
	else if(driveable == TRUCK)
		return "truck";
	else if(driveable == AIRPLANE)
		return "airplane";
	else if(driveable == BOAT)
		return "boat";
	else if(driveable == MACHINE)
		return "machine";
	return "unkown";
}

void MapControl::deleteMapEntity(MapEntity *ent)
{
	std::vector<MapEntity *>::iterator it;
	for(it=mMapEntities.begin(); it!= mMapEntities.end(); it++)
	{
		if((*it) == ent)
		{
			// found it, erase!
			delete *it;
			mMapEntities.erase(it);
			return;
		}
	}
}

bool MapControl::getVisibility()
{
	return mMainWidget->getVisible();
}

void MapControl::setVisibility(bool value)
{
	mMainWidget->setVisible(value);
}

void MapControl::setAlpha(float value)
{
	mAlpha = value;
	mMainWidget->setAlpha(value);
}

void MapControl::setPosition(float _x, float _y, float _w, float _h, Ogre::RenderWindow* rw)
{
	mMainWidget->setCoord(_x*rWinWidth, _y*rWinHeight, _w*rWinWidth, _h*rWinHeight);
	myScale = _w;
	updateEntityPositions();
}


void MapControl::updateEntityPositions()
{
	std::vector<MapEntity *>::iterator it;
	for(it=mMapEntities.begin(); it!=mMapEntities.end(); it++)
	{
		(*it)->update();
	}
}

void MapControl::setEntitiesVisibility(bool value)
{
	std::vector<MapEntity *>::iterator it;
	for(it=mMapEntities.begin(); it!=mMapEntities.end(); it++)
	{
		(*it)->setVisibility(value);
	}
}

void MapControl::resizeToScreenRatio(Ogre::RenderWindow* rw)
{
	//win->setRealPosition(
	// TODO
}

void MapControl::updateRenderMetrics(Ogre::RenderWindow* win)
{
	win->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);
}

#endif //MYGUI

