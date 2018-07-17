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
#include "dashboard.h"
#include "TruckHUD.h"
#include "IngameConsole.h"
#include "ResourceBuffer.h"

SceneManager* mScene;
Overlay* dashOverlay;
Overlay* needlesOverlay;
Overlay* blendOverlay;
Overlay* truckHUDOverlay;

class DashboardListener : public RenderTargetListener
{
private:
	bool fpsDisplayed;
	bool truckHUD;
	bool consolevisible;
	bool truckhHUDvisible;
	Ogre::Overlay *fpsOverlay;
public:
	DashboardListener() : RenderTargetListener(), fpsOverlay(0)
	{
	}

	void preRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		// Hide everything
		mScene->setFindVisibleObjects(false);

		// hide fps stats
		if(fpsOverlay)
		{
			fpsDisplayed = fpsOverlay->isVisible();
			if(fpsDisplayed)
				fpsOverlay->hide();
		} else
			// this must be here and not in the constructor, as upon construction time the overlaymanager is not working, somehow
			fpsOverlay = OverlayManager::getSingleton().getByName("Core/DebugOverlay");

		// hide truck HUD
		truckhHUDvisible = truckHUDOverlay->isVisible();
		if(truckhHUDvisible)
			truckHUDOverlay->hide();

		//show overlay
		dashOverlay->show();
		needlesOverlay->show();
		blendOverlay->show();

		//hide unwanted overlays
		consolevisible = NETCHAT.getVisible();
		if(consolevisible)
			NETCHAT.setVisible(false);

	}
	void postRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		// Show everything 
		mScene->setFindVisibleObjects(true);

		//show everything again, if it was displayed before hiding it...
		if(fpsOverlay && fpsDisplayed)
			fpsOverlay->show();

		if(truckHUDOverlay && truckhHUDvisible)
			truckHUDOverlay->show();

		//hide overlay
		dashOverlay->hide();
		needlesOverlay->hide();
		blendOverlay->hide();

		if(consolevisible)
			NETCHAT.setVisible(true);
	}

};

DashboardListener mDashboardListener;

Dashboard::Dashboard(SceneManager *mSceneMgr, RenderWindow *mWindow)
	{
		mScene=mSceneMgr;
		rttTex=0;

		TexturePtr rttTexPtr = TextureManager::getSingleton().createManual("dashtexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 1024, 512, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
		rttTex = rttTexPtr->getBuffer()->getRenderTarget();
		{
			mDashCam = mSceneMgr->createCamera("DashCam");
			mDashCam->setNearClipDistance(1.0);
			mDashCam->setFarClipDistance(10.0);
			mDashCam->setPosition(Vector3(0.0, -10000.0, 0.0));

			mDashCam->setAspectRatio(2.0);

			Viewport *v = rttTex->addViewport( mDashCam );
			v->setClearEveryFrame( true );
			v->setBackgroundColour( ColourValue::Black );

			MaterialPtr mat = MaterialManager::getSingleton().getByName("renderdash");
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("dashtexture");

			//v->setOverlaysEnabled(false);

			rttTex->addListener(&mDashboardListener);

		}
		dashOverlay = OverlayManager::getSingleton().getByName("tracks/3D_DashboardOverlay");
		needlesOverlay = OverlayManager::getSingleton().getByName("tracks/3D_NeedlesOverlay");
		blendOverlay = OverlayManager::getSingleton().getByName("tracks/3D_BlendOverlay");
		truckHUDOverlay = OverlayManager::getSingleton().getByName("tracks/TruckInfoBox");

//		dbdebugOverlay = OverlayManager::getSingleton().getByName("Core/DebugOverlay");
//		dbeditorOverlay = OverlayManager::getSingleton().getByName("tracks/EditorOverlay");

	}


void Dashboard::setEnable(bool en)
{
	rttTex->setActive(en);
}

void Dashboard::prepareShutdown()
{
	if (rttTex) rttTex->removeListener(&mDashboardListener);
}
