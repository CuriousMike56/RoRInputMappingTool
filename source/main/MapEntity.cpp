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

#include "Beam.h"
#include "MapEntity.h"
#include "MapControl.h"

Ogre::String MapEntity::entityStates[MaxEntityStates] = {"activated", "deactivated", "sleeping", "networked"};

MapEntity::MapEntity(MapControl *ctrl, Ogre::String type, MyGUI::StaticImagePtr _parent)
{
	initialiseByAttributes(this, _parent);

	if (mIcon) mIconRotating = mIcon->getSubWidgetMain()->castType<MyGUI::RotatingSkin>(false);
	else mIconRotating = nullptr;

	mMapControl = ctrl;
	mParent = _parent;
	mType = type;
	mX = 0;
	mZ = 0;
	mRotation = 0;
	mState = Sleeping;
	init();
}

MapEntity::~MapEntity()
{
}

void MapEntity::init()
{
	// check if static only icon
	Ogre::String imageFile = "icon_" + mType + ".dds";
	Ogre::String group = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

	if(Ogre::ResourceGroupManager::getSingleton().resourceExists(group, imageFile))
	{
		Ogre::LogManager::getSingleton().logMessage("static map icon found: " + imageFile);
		mIsStatic=true;
	}
	else
	{
		Ogre::LogManager::getSingleton().logMessage("static map icon not found: " + imageFile);
		mIsStatic=false;
	}

	setVisibility(false);

	updateIcon();
	update();
}

void MapEntity::setPosition(Ogre::Vector3 pos)
{
	setPosition(pos.x, pos.z);
}

void MapEntity::setPosition(float _x, float _z)
{
	bool needUpdate=false;
	if(fabs(_x - mX) > 0.00001f || fabs(_z - mZ) > 0.00001f)
		needUpdate=true;
	mX = _x;
	mZ = _z;
	if(needUpdate)
		update();
}

void MapEntity::setRotation(Ogre::Quaternion q)
{
	mRotation = q.getYaw().valueRadians() - Ogre::Math::PI/2;
	if (mIconRotating) mIconRotating->setAngle(-mRotation);
}

void MapEntity::setRotation(Ogre::Radian _r)
{
	mRotation = _r.valueRadians();
	if (mIconRotating) mIconRotating->setAngle(-mRotation);
}

void MapEntity::onTop()
{
//	container->_notifyZOrder(container->getZOrder()+10);
}

bool MapEntity::getVisibility()
{
	return mMainWidget->getVisible();
}

void MapEntity::setVisibility(bool value)
{
	mMainWidget->setVisible(value);
}

void MapEntity::setState(int truckstate)
{
	if (mIsStatic)
		return;

	EntityStates mapstate;
	switch (truckstate)
	{
	case ACTIVATED: mapstate = Activated; break;
	case DESACTIVATED:
	case MAYSLEEP:
	case GOSLEEP: mapstate = Deactivated; break;
	case SLEEPING: mapstate = Sleeping; break;
	case NETWORKED: mapstate = Networked; break;
	default: mapstate = Sleeping;
	}

	if (mState != mapstate)
	{
		mState = mapstate;
		update();
	}
}

int MapEntity::getState()
{
	return mState;
}

void MapEntity::update()
{
	float wscale = mMapControl->getWindowScale();

	mCaption->setVisible(wscale > 0.5f);

	Ogre::Vector2 s = mMapControl->getMapSize();
	mMainWidget->setPosition(
		mX / s.x * mParent->getWidth() - mMainWidget->getWidth() / 2,
		mZ / s.y * mParent->getHeight() - mMainWidget->getHeight() / 2
	);
	mIcon->setCoord(
		mMainWidget->getWidth() / 2 - mIconSize.width * wscale / 2,
		mMainWidget->getHeight() / 2 - mIconSize.height * wscale / 2,
		mIconSize.width * wscale,
		mIconSize.height * wscale
	);
	mIcon->setVisible(true);
}

void MapEntity::setDescription(Ogre::String s)
{
	mDescription = s;
	mCaption->setCaption(mDescription);
}

Ogre::String MapEntity::getDescription()
{
	return mDescription;
}

void MapEntity::updateIcon()
{
	// check if static only icon
	Ogre::String imageFile;
	if(mIsStatic)	imageFile = "icon_" + mType + ".dds";
	else			imageFile = "icon_" + mType + "_" + entityStates[mState] + ".dds";

	// set image texture to load it into memory, so TextureManager::getByName will have it loaded if files exist
	mIcon->setImageTexture(imageFile);

	Ogre::TexturePtr texture = (Ogre::TexturePtr)(Ogre::TextureManager::getSingleton().getByName(imageFile));
	if(texture.isNull())
	{
		imageFile = "icon_missing.dds";
		texture = (Ogre::TexturePtr)(Ogre::TextureManager::getSingleton().getByName(imageFile));
	}

	if(!texture.isNull())
	{
		mIconSize.width = texture->getWidth();
		mIconSize.height = texture->getHeight();
		mIcon->setSize(mIconSize);
	}
	
	if (mIconRotating)
	{
		mIconRotating->setCenter(MyGUI::IntPoint(mIcon->getWidth()/2, mIcon->getHeight()/2));
		mIconRotating->setAngle(mRotation);
	}
}

#endif // MYGUI
