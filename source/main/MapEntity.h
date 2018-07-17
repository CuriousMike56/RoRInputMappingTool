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

#ifndef __MAP_ENTITY_H__
#define __MAP_ENTITY_H__

#include <Ogre.h>
#include <BaseLayout.h>

class MapControl;

ATTRIBUTE_CLASS_LAYOUT(MapEntity, "MapEntity.layout");
class MapEntity :
	public wraps::BaseLayout,
	public MemoryAllocatedObject
{
public:
	MapEntity(MapControl *ctrl, Ogre::String type, MyGUI::StaticImagePtr parent);
	~MapEntity();
	void setPosition(Ogre::Vector3 pos);
	void setPosition(float x, float z);
	void setRotation(Ogre::Quaternion q);
	void setRotation(Ogre::Radian r);
	bool getVisibility();
	void setVisibility(bool value);
	void onTop();

	void setState(int state);
	int getState();

	enum EntityStates {Activated = 0, Deactivated, Sleeping, Networked, MaxEntityStates};
	static Ogre::String entityStates[MaxEntityStates];
	void update();
	void setDescription(Ogre::String s);
	Ogre::String getDescription();

protected:
	void updateIcon();
private:
	MyGUI::StaticImagePtr mParent;

	ATTRIBUTE_FIELD_WIDGET_NAME(MapEntity, mCaption, "mCaption");
	MyGUI::StaticText* mCaption;

	ATTRIBUTE_FIELD_WIDGET_NAME(MapEntity, mIcon, "mIcon");
	MyGUI::StaticImage* mIcon;

	MyGUI::RotatingSkin* mIconRotating;

	Ogre::String mType;
	Ogre::String mDescription;
	Ogre::Real mX, mZ;
	MyGUI::IntSize mIconSize;
	Ogre::Real mRotation;
	MapControl *mMapControl;
	EntityStates mState;
	bool mIsStatic;
	void init();
};

#endif // __MAP_ENTITY_H__


#endif //MYGUI

