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

#include "MapTextureCreator.h"

#include "ResourceBuffer.h"
#include "RoRFrameListener.h"
#include "water.h"

int MapTextureCreator::mCounter=0;
MapTextureCreator::MapTextureCreator(Ogre::SceneManager *mgr, Ogre::Camera *maincam, RoRFrameListener *efl)
{
	mEfl=efl;
	mMainCam = maincam;
	mCounter++;
	mSceneManager=mgr;
	mStatics=0;
	mCamdir=Ogre::Quaternion::ZERO;
	init();
}

void MapTextureCreator::init()
{
	Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual("MapRttTex"+Ogre::StringConverter::toString(mCounter), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, 1024, 1024, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET, new ResourceBuffer());
	mRttTex = texture->getBuffer()->getRenderTarget();
	mRttTex->setAutoUpdated(true);
	mCamera = mSceneManager->createCamera("MapRenderCam");

	mViewport = mRttTex->addViewport(mCamera);
	mViewport->setBackgroundColour(Ogre::ColourValue::Black);
	mViewport->setOverlaysEnabled(false);

	mMaterial = Ogre::MaterialManager::getSingleton().create("MapRttMat"+Ogre::StringConverter::toString(mCounter), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	mTextureUnitState = mMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("MapRttTex"+Ogre::StringConverter::toString(mCounter));

	mRttTex->addListener(this);
	//mCamera->setProjectionType(PT_ORTHOGRAPHIC);

	mZoom = 3;

	mCamera->setPosition(0, 4500, 0.0001);
	mCamera->lookAt(0,0,0);

	mCamera->setFarClipDistance(0);
	mCamera->setAspectRatio(1.0);
	mCamera->setFixedYawAxis(false);
	mCamera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
	mCamera->setFOVy(Ogre::Radian(Ogre::Math::HALF_PI));
	mCamera->setNearClipDistance(mZoom);
}

void MapTextureCreator::setTranlucency(float amount)
{
	//mTextureUnitState->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_MANUAL, 1.0, amount);
}

void MapTextureCreator::setCameraMode(Ogre::PolygonMode pm)
{
	mCamera->setPolygonMode(pm);
}

void MapTextureCreator::setStaticGeometry(Ogre::StaticGeometry *geo)
{
	mStatics = geo;
}

void MapTextureCreator::setCamZoomRel(float zoomdelta)
{
	mZoom += zoomdelta * mZoom/100;
}

void MapTextureCreator::setCamZoom(float newzoom)
{
	mZoom = newzoom;
}

void MapTextureCreator::setCamPosition(Ogre::Vector3 pos, Ogre::Quaternion direction)
{
	mCampos = pos;
	mCamdir = direction;
}


void MapTextureCreator::update()
{
	// 1 = max out = total overview
	if(mZoom<=0.3)
		mZoom=0.3;

	float width = mEfl->mapsizex;
	float height = mEfl->mapsizez;
	float zoomfactor = mZoom * ((width+height)/2) * 0.002;

	//LogManager::getSingleton().logMessage(StringConverter::toString(mZoom));
	mCamera->setNearClipDistance(mZoom);
	mCamera->setPosition(mCampos + Ogre::Vector3(0, zoomfactor, 0));
	if(mCamdir != Ogre::Quaternion::ZERO) mCamera->setOrientation(mCamdir);
	mCamera->lookAt(mCampos - Ogre::Vector3(0, zoomfactor, 0));

	// output the zoom factor for debug purposes
	//LogManager::getSingleton().logMessage(StringConverter::toString(mZoom));

	/*
	// this is bugged, so deactivated for now
	float f = 50-mZoom;
	if(f<20)
		f=20;
	for (int i=0; i<mEfl->getTruckCount(); i++)
		mEfl->getTruck(i)->preMapLabelRenderUpdate(true, f);
	*/

	if(mStatics)
		mStatics->setRenderingDistance(0);
	// thats a huge workaround to be able to not use the normal LOD

	setFogVisible(false);

	mRttTex->update();

	setFogVisible(true);
	if(mEfl->getWater())
		mEfl->getWater()->setVisible(false);
	if(mStatics)
		mStatics->setRenderingDistance(1000);
	if(mEfl->getWater())
		mEfl->getWater()->setVisible(true);

	/*
	// deactivated for the moment
	for (int i=0; i<mEfl->getTruckCount(); i++)
		mEfl->getTruck(i)->preMapLabelRenderUpdate(false);
	*/

}

void MapTextureCreator::setFogVisible(bool value)
{
	int fogmode = mEfl->getFogMode();
	//LogManager::getSingleton().logMessage("fogswitch: "+StringConverter::toString(fogmode)+" / "+StringConverter::toString(value));
	if(!fogmode || fogmode == 2)
		return;

	// this refuses to work, somehow:
	if(fogmode == 1)
	{
#if 0
		// TODO: tofix: caelum
		// USE_CAELUM
		if(value)
			static_cast<Caelum::StoredImageSkyColourModel *>(mEfl->getCaelumModel())->setFogDensity(mEfl->getFogDensity());
		else
			static_cast<Caelum::StoredImageSkyColourModel *>(mEfl->getCaelumModel())->setFogDensity(0);

		// force Caelum to update
		if(value)
			mEfl->getCaelumSystem()->setLocalTime(mEfl->getCaelumSystem()->getLocalTime()+1);
		else
			mEfl->getCaelumSystem()->setLocalTime(mEfl->getCaelumSystem()->getLocalTime()-1);
#endif //0
	}
	else if(fogmode == 3)
	{
		if(value)
			mSceneManager->setFog(Ogre::FOG_LINEAR, mSceneManager->getFogColour(), mEfl->getFogDensity(), mSceneManager->getFogStart(), mSceneManager->getFogEnd());
		else
			mSceneManager->setFog(Ogre::FOG_NONE, mSceneManager->getFogColour(), mEfl->getFogDensity(), mSceneManager->getFogStart(), mSceneManager->getFogEnd());
	}

}

void MapTextureCreator::setAutoUpdated(bool value)
{
	mRttTex->setAutoUpdated(value);
}

Ogre::String MapTextureCreator::getMaterialName()
{
	return "MapRttMat"+Ogre::StringConverter::toString(mCounter);
}

Ogre::String MapTextureCreator::getRTName()
{
	return "MapRttTex"+Ogre::StringConverter::toString(mCounter);
}

void MapTextureCreator::preRenderTargetUpdate(const RenderTargetEvent& evt)
{
	Water *w = mEfl->getWater();
	if(w)
		w->setVisible(false);
}
void MapTextureCreator::postRenderTargetUpdate(const RenderTargetEvent& evt)
{
	Water *w = mEfl->getWater();
	if(w)
		w->setVisible(true);
}
