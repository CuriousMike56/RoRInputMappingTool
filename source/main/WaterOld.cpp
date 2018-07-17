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
#include "WaterOld.h"
#include "Settings.h"
#include "rormemory.h"
#include "RoRFrameListener.h"
#include "ResourceBuffer.h"

//Camera* theCam;
Entity* pPlaneEnt;
//std::vector<Entity*> aboveWaterEnts;
//std::vector<Entity*> belowWaterEnts;

Plane waterPlane;
Plane bottomPlane;
Plane reflectionPlane;
Plane refractionPlane;
SceneManager *waterSceneMgr;

class RefractionTextureListener : public RenderTargetListener
{
public:
	void preRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		waterSceneMgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(false);
		// Hide plane
		pPlaneEnt->setVisible(false);
		//hide WaterOld spray
		if (eflsingleton) eflsingleton->showspray(false);
	}
	void postRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		// Show plane
		pPlaneEnt->setVisible(true);
		waterSceneMgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(true);
		//restore WaterOld spray
		if (eflsingleton) eflsingleton->showspray(true);
	}

};
class ReflectionTextureListener : public RenderTargetListener
{
public:
	void preRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		waterSceneMgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(false);
		// Hide plane
		pPlaneEnt->setVisible(false);

	}
	void postRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		// Show plane
		pPlaneEnt->setVisible(true);
		waterSceneMgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(true);
	}
};

RefractionTextureListener mRefractionListener;
ReflectionTextureListener mReflectionListener;

WaterOld::WaterOld()
{
	// empty constructor for derivated objects
}

WaterOld::WaterOld(int type, Camera *camera, SceneManager *mSceneMgr, RenderWindow *mWindow, float wheight, float *_mapsizex, float *_mapsizez, bool usewaves)
{
	vRtt1 = vRtt2 = 0;
	mapsizex = _mapsizex;
	mapsizez = _mapsizez;
	mScale = 1.0f;
	if(*mapsizex < 1500)
		mScale = 1.5f;
	//reading wavefield
	visible=true;
	haswaves=usewaves;
	free_wavetrain=0;
	FILE *fd;
	char line[1024];
	fd=fopen((SETTINGS.getSetting("Config Root")+"wavefield.cfg").c_str(), "r");
	if (fd)
	{
		while (!feof(fd))
		{
			int res = fscanf(fd," %[^\n\r]",line);
			if (line[0]==';')
			{
				continue;
			};
			float wl,amp,mx,dir;
			res = sscanf(line,"%f, %f, %f, %f",&wl,&amp,&mx,&dir);
			if(res < 4) continue;
			wavetrains[free_wavetrain].wavelength=wl;
			wavetrains[free_wavetrain].amplitude=amp;
			wavetrains[free_wavetrain].maxheight=mx;
			wavetrains[free_wavetrain].direction=dir/57.0;
			free_wavetrain++;
		}
		fclose(fd);
	}
	//        theCam=camera;
	int i;
	maxampl=0;
	for (i=0; i<free_wavetrain; i++)
	{
		wavetrains[i].wavespeed=1.25*sqrt(wavetrains[i].wavelength);
		maxampl+=wavetrains[i].maxheight;
	}
	pTestNode=0;
	waterSceneMgr=mSceneMgr;
	framecounter=0;
	mCamera=camera;
	height=wheight;
	orgheight=wheight;
	mType=type;
	rttTex1=0;
	rttTex2=0;
	MeshPtr mprt;
	mReflectCam=0;
	mRefractCam=0;
	//		wbuf=0;
	//		ColourValue fade=camera->getViewport()->getBackgroundColour();
	ColourValue fade=mSceneMgr->getFogColour();

	if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED || mType==WATER_REFLECT)
	{
		// Check prerequisites first
		const RenderSystemCapabilities* caps = Root::getSingleton().getRenderSystem()->getCapabilities();
		if (!caps->hasCapability(RSC_VERTEX_PROGRAM) || !(caps->hasCapability(RSC_FRAGMENT_PROGRAM)))
		{
			OGRE_EXCEPT(1, "Your card does not support vertex and fragment programs, so cannot "
				"run WaterOld effects. Sorry!",
				"WaterOld effects");
		}
		else
		{
			if (!GpuProgramManager::getSingleton().isSyntaxSupported("arbfp1") &&
				!GpuProgramManager::getSingleton().isSyntaxSupported("ps_2_0") &&
				!GpuProgramManager::getSingleton().isSyntaxSupported("ps_1_4")
				)
			{
				OGRE_EXCEPT(1, "Your card does not support advanced fragment programs, "
					"so cannot run WaterOld effects. Sorry!",
					"WaterOld effects");
			}
		}
		//okay
		// Define a floor plane mesh
		reflectionPlane.normal = Vector3::UNIT_Y;
		reflectionPlane.d = -wheight+0.15;
		refractionPlane.normal = -Vector3::UNIT_Y;
		refractionPlane.d = wheight+0.15;
		waterPlane.normal = Vector3::UNIT_Y;
		waterPlane.d = -wheight;

		if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED)
		{
			TexturePtr rttTex1Ptr = TextureManager::getSingleton().createManual("Refraction", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 512, 512, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
			rttTex1 = rttTex1Ptr->getBuffer()->getRenderTarget();
			{
				mRefractCam = mSceneMgr->createCamera("RefractCam");
				mRefractCam->setNearClipDistance(mCamera->getNearClipDistance());
				mRefractCam->setFarClipDistance(mCamera->getFarClipDistance());
				mRefractCam->setAspectRatio(
					(Real)mWindow->getViewport(0)->getActualWidth() /
					(Real)mWindow->getViewport(0)->getActualHeight());

				vRtt1 = rttTex1->addViewport( mRefractCam );
				vRtt1->setClearEveryFrame( true );
				vRtt1->setBackgroundColour( fade );
				//            v->setBackgroundColour( ColourValue::Black );


				MaterialPtr mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefraction");
				mat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName("Refraction");

				vRtt1->setOverlaysEnabled(false);

				rttTex1->addListener(&mRefractionListener);

				//optimisation
				rttTex1->setAutoUpdated(false);

				// Also clip
				mRefractCam->enableCustomNearClipPlane(refractionPlane);
			}
		}

		TexturePtr rttTex2Ptr = TextureManager::getSingleton().createManual("Reflection", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 512, 512, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
		rttTex2 = rttTex2Ptr->getBuffer()->getRenderTarget();
		{
			mReflectCam = mSceneMgr->createCamera("ReflectCam");
			mReflectCam->setNearClipDistance(mCamera->getNearClipDistance());
			mReflectCam->setFarClipDistance(mCamera->getFarClipDistance());
			mReflectCam->setAspectRatio(
				(Real)mWindow->getViewport(0)->getActualWidth() /
				(Real)mWindow->getViewport(0)->getActualHeight());

			vRtt2 = rttTex2->addViewport( mReflectCam );
			vRtt2->setClearEveryFrame( true );
			vRtt2->setBackgroundColour( fade );
			//            v->setBackgroundColour( ColourValue::Black );


			MaterialPtr mat ;
			if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED) mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefraction");
			else mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflection");
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName("Reflection");

			vRtt2->setOverlaysEnabled(false);

			rttTex2->addListener(&mReflectionListener);

			//optimisation
			rttTex2->setAutoUpdated(false);

			// set up linked reflection
			mReflectCam->enableReflection(waterPlane);
			// Also clip
			mReflectCam->enableCustomNearClipPlane(reflectionPlane);
		}

		mprt=MeshManager::getSingleton().createPlane("ReflectPlane",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			waterPlane,
			*mapsizex * mScale,*mapsizez * mScale,WAVEREZ,WAVEREZ,true,1,50,50,Vector3::UNIT_Z, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
		pPlaneEnt = mSceneMgr->createEntity( "plane", "ReflectPlane" );
		if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED) pPlaneEnt->setMaterialName("Examples/FresnelReflectionRefraction");
		else pPlaneEnt->setMaterialName("Examples/FresnelReflection");
		//        mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(pPlaneEnt);
		//position
		pTestNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("WaterPlane");
		pTestNode->attachObject(pPlaneEnt);
		pTestNode->setPosition( Vector3((*mapsizex * mScale)/2,0,(*mapsizez * mScale)/2) );
	}
	else
	{
		//basic WaterOld
		waterPlane.normal = Vector3::UNIT_Y;
		waterPlane.d = -wheight;
		mprt=MeshManager::getSingleton().createPlane("WaterPlane",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			waterPlane,
			*mapsizex * mScale,*mapsizez * mScale,WAVEREZ,WAVEREZ,true,1,50,50,Vector3::UNIT_Z, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
		pPlaneEnt = mSceneMgr->createEntity( "plane", "WaterPlane" );
		pPlaneEnt->setMaterialName("tracks/basicwater");
		//position
		pTestNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("WaterPlane");
		pTestNode->attachObject(pPlaneEnt);
		pTestNode->setPosition( Vector3((*mapsizex * mScale)/2,0,(*mapsizez * mScale)/2) );
	}
	//bottom
	bottomPlane.normal = Vector3::UNIT_Y;
	bottomPlane.d = -wheight+30.0; //30m below waterline
	MeshManager::getSingleton().createPlane("BottomPlane",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		bottomPlane,
		*mapsizex * mScale,*mapsizez * mScale,1,1,true,1,1,1,Vector3::UNIT_Z);
	Entity *pE = mSceneMgr->createEntity( "bplane", "BottomPlane" );
	pE->setMaterialName("tracks/seabottom");
	//position
	pBottomNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("BottomWaterPlane");
	pBottomNode->attachObject(pE);
	pBottomNode->setPosition( Vector3((*mapsizex * mScale)/2,0,(*mapsizez * mScale)/2) );
	//setup for waves
	wbuf=mprt->sharedVertexData->vertexBufferBinding->getBuffer(0);
	if (wbuf->getSizeInBytes()==(WAVEREZ+1)*(WAVEREZ+1)*32)
	{
		wbuffer=(float*)ror_malloc(wbuf->getSizeInBytes());
		wbuf->readData(0, wbuf->getSizeInBytes(), wbuffer);
	} else wbuffer=0;
}

bool WaterOld::allowUnderWater()
{
	return false;
}

void WaterOld::setVisible(bool value)
{
	visible = value;
	if(pPlaneEnt)
		pPlaneEnt->setVisible(value);
	if(pTestNode)
		pTestNode->setVisible(value);
	if(pBottomNode)
		pBottomNode->setVisible(value);

}

void WaterOld::setFadeColour(ColourValue ambient)
{
	// update the viewports background colour!
	if(vRtt1) vRtt1->setBackgroundColour(ambient);
	if(vRtt2) vRtt2->setBackgroundColour(ambient);
}


void WaterOld::moveTo(Camera *cam, float centerheight)
{
	if (pTestNode)
	{
		Vector3 pos=cam->getPosition();
		Vector3 offset=cam->getDirection();
		offset.y=0;
		offset.normalise();
		pos = pos + offset * *mapsizex * mScale * 0.46666;
		pos.y=orgheight - height;
		pos.x=((int)pos.x/60)*60;
		pos.z=((int)pos.z/60)*60;
		pTestNode->setPosition(pos);
		pBottomNode->setPosition(pos);
		if (haswaves) showWave(pos);
		if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED || mType==WATER_REFLECT) updateReflectionPlane(centerheight);
	}
}

void WaterOld::showWave(Vector3 refpos)
{
	int px,pz;
	if (!wbuffer) return;
	for (px=0; px<WAVEREZ+1; px++)
	{
		for (pz=0; pz<WAVEREZ+1; pz++)
		{
			wbuffer[(pz*(WAVEREZ+1)+px)*8+1]=getHeightWaves(refpos+Vector3((*mapsizex * mScale)/2-(float)px*(*mapsizex * mScale)/WAVEREZ, 0, (float)pz*(*mapsizez * mScale)/WAVEREZ-(*mapsizez * mScale)/2));
		}
	}
	//normals
	for (px=0; px<WAVEREZ+1; px++)
	{
		for (pz=0; pz<WAVEREZ+1; pz++)
		{
			int left=px-1; if (left<0) left=0;
			int right=px+1; if (right>WAVEREZ) right=WAVEREZ;
			int up=pz-1; if (up<0) up=0;
			int down=pz+1; if (down>WAVEREZ) down=WAVEREZ;
			Vector3 normal=(Vector3(wbuffer+((pz*(WAVEREZ+1)+left)*8))-Vector3(wbuffer+((pz*(WAVEREZ+1)+right)*8))).crossProduct(Vector3(wbuffer+((up*(WAVEREZ+1)+px)*8))-Vector3(wbuffer+((down*(WAVEREZ+1)+px)*8)));
			normal.normalise();
			wbuffer[(pz*(WAVEREZ+1)+px)*8+3]=normal.x;
			wbuffer[(pz*(WAVEREZ+1)+px)*8+4]=normal.y;
			wbuffer[(pz*(WAVEREZ+1)+px)*8+5]=normal.z;
		}
	}
//	wbuf->lock(HardwareBuffer::HBL_DISCARD);
	wbuf->writeData(0, (WAVEREZ+1)*(WAVEREZ+1)*32, wbuffer, true);
//	if(wbuf->isLocked())
//		wbuf->unlock();
}

void WaterOld::update()
{
	if(!visible)
		return;
	framecounter++;
	if (mType==WATER_FULL_SPEED)
	{
		if (framecounter%2)
		{
			mReflectCam->setOrientation(mCamera->getOrientation());
			mReflectCam->setPosition(mCamera->getPosition());
			mReflectCam->setFOVy(mCamera->getFOVy());
			rttTex2->update();
		}
		else
		{
			mRefractCam->setOrientation(mCamera->getOrientation());
			mRefractCam->setPosition(mCamera->getPosition());
			mRefractCam->setFOVy(mCamera->getFOVy());
			rttTex1->update();
		}
	} else if (mType==WATER_FULL_QUALITY)
	{
		mReflectCam->setOrientation(mCamera->getOrientation());
		mReflectCam->setPosition(mCamera->getPosition());
		mReflectCam->setFOVy(mCamera->getFOVy());
		rttTex2->update();
		mRefractCam->setOrientation(mCamera->getOrientation());
		mRefractCam->setPosition(mCamera->getPosition());
		mRefractCam->setFOVy(mCamera->getFOVy());
		rttTex1->update();
	}
	else if (mType==WATER_REFLECT)
	{
		mReflectCam->setOrientation(mCamera->getOrientation());
		mReflectCam->setPosition(mCamera->getPosition());
		mReflectCam->setFOVy(mCamera->getFOVy());
		rttTex2->update();
	}
}

void WaterOld::prepareShutdown()
{
	if (rttTex1) rttTex1->removeListener(&mRefractionListener);
	if (rttTex2) rttTex2->removeListener(&mReflectionListener);
}

float WaterOld::getHeight() {return height;};

void WaterOld::setHeight(float value)
{
	height = value;
	update();
}

float WaterOld::getHeightWaves(Vector3 pos)
{
	if(!haswaves)
		return height;
	if (pos.y>height+maxampl) return height;
	int i;
	float waveheight=(pos-Vector3((*mapsizex * mScale)/2, height, (*mapsizez * mScale)/2)).squaredLength()/3000000.0;
	float result=height;
	for (i=0; i<free_wavetrain; i++)
	{
		float amp=wavetrains[i].amplitude*waveheight;
		if (amp>wavetrains[i].maxheight) amp=wavetrains[i].maxheight;
		result+=amp*sin(6.28318*((mrtime*wavetrains[i].wavespeed+sin(wavetrains[i].direction)*pos.x+cos(wavetrains[i].direction)*pos.z)/wavetrains[i].wavelength));
	}
	return result;
}

Vector3 WaterOld::getVelocity(Vector3 pos)
{
	if (pos.y>height+maxampl) return Vector3::ZERO;
	int i;
	float waveheight=(pos-Vector3((*mapsizex * mScale)/2, height, (*mapsizez * mScale)/2)).squaredLength()/3000000.0;
	Vector3 result=Vector3::ZERO;
	for (i=0; i<free_wavetrain; i++)
	{
		float amp=wavetrains[i].amplitude*waveheight;
		if (amp>wavetrains[i].maxheight) amp=wavetrains[i].maxheight;
		float speed=6.28318*amp/(wavetrains[i].wavelength/wavetrains[i].wavespeed);
		result.y+=speed*cos(6.28318*((mrtime*wavetrains[i].wavespeed+sin(wavetrains[i].direction)*pos.x+cos(wavetrains[i].direction)*pos.z)/wavetrains[i].wavelength));
		result+=Vector3(sin(wavetrains[i].direction), 0, cos(wavetrains[i].direction))*speed*sin(6.28318*((mrtime*wavetrains[i].wavespeed+sin(wavetrains[i].direction)*pos.x+cos(wavetrains[i].direction)*pos.z)/wavetrains[i].wavelength));
	}
	return result;
}



void WaterOld::updateReflectionPlane(float h)
{
	//Ray ra=mCamera->getCameraToViewportRay(0.5,0.5);
	//std::pair<bool, Real> mpair=ra.intersects(Plane(Vector3::UNIT_Y, -height));
	//if (mpair.first) h=ra.getPoint(mpair.second).y;
	reflectionPlane.d = -h+0.15;
	refractionPlane.d = h+0.15;
	waterPlane.d = -h;
	if (mRefractCam) mRefractCam->enableCustomNearClipPlane(refractionPlane);
	if (mReflectCam)
	{
		mReflectCam->enableReflection(waterPlane);
		mReflectCam->enableCustomNearClipPlane(reflectionPlane);
	};

}


void WaterOld::setSunPosition(Ogre::Vector3)
{
	// not used here!
}

void WaterOld::framestep(float dt)
{
	update();
}
