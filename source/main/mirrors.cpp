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
#include "mirrors.h"
#include "ResourceBuffer.h"
#include <Ogre.h>

Mirrors::Mirrors(Ogre::SceneManager *mSceneMgr, Ogre::RenderWindow *mWindow, Ogre::Camera *camera)
: 
mCamera(camera)
, mMirrorCam(mSceneMgr->createCamera("MirrorCam"))
, rttTex(0)
, mat(Ogre::MaterialManager::getSingleton().getByName("mirror"))
{

		Ogre::TexturePtr rttTexPtr 
			= Ogre::TextureManager::getSingleton().createManual("mirrortexture"
				, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
				, Ogre::TEX_TYPE_2D
				, 128
				, 256
				, 0
				, Ogre::PF_R8G8B8
				, Ogre::TU_RENDERTARGET
				, new ResourceBuffer());
		rttTex = rttTexPtr->getBuffer()->getRenderTarget();
		
		mMirrorCam->setNearClipDistance(0.2);
		mMirrorCam->setFarClipDistance(mCamera->getFarClipDistance());
		mMirrorCam->setFOVy(Ogre::Degree(50));
		mMirrorCam->setAspectRatio(
			((Ogre::Real)mWindow->getViewport(0)->getActualWidth() / 
			(Ogre::Real)mWindow->getViewport(0)->getActualHeight())/2.0);

		Ogre::Viewport *v = rttTex->addViewport( mMirrorCam );
		v->setClearEveryFrame( true );
		v->setBackgroundColour( camera->getViewport()->getBackgroundColour() );

		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirrortexture");
		mat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
		v->setOverlaysEnabled(false);
	}

void Mirrors::setActive(bool state)
{
	rttTex->setActive(state);
	if (state)
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirrortexture");
	else
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirror.dds");

}

void Mirrors::update(Ogre::Vector3 normal, Ogre::Vector3 center, Ogre::Radian rollangle)
{
	Ogre::Plane plane=Ogre::Plane(normal, center);
	mMirrorCam->setPosition(center); 
	Ogre::Vector3 project=plane.projectVector(mCamera->getPosition()-center);
	mMirrorCam->lookAt(mCamera->getPosition()-2*project); 
	mMirrorCam->roll(rollangle);
}

