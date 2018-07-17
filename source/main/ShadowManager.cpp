/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009,2010 Pierre-Michel Ricordel
Copyright 2007,2008,2009,2010 Thomas Fischer

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
#include "ShadowManager.h"

#include "Ogre.h"
#include "OgreShadowCameraSetup.h"

#include "OgreTerrain.h"
#include "OgreTerrainMaterialGeneratorA.h"

#include "Settings.h"

//using namespace std;
//using namespace Ogre;

//---------------------------------------------------------------------
template<> ShadowManager * Ogre::Singleton< ShadowManager >::ms_Singleton = 0;
ShadowManager* ShadowManager::getSingletonPtr(void)
{
	return ms_Singleton;
}
ShadowManager& ShadowManager::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

ShadowManager::ShadowManager(Ogre::SceneManager *scene, Ogre::RenderWindow *window, Ogre::Camera *camera) :
	mSceneMgr(scene), mWindow(window), mCamera(camera), mPSSMSetup()
{
}

ShadowManager::~ShadowManager()
{
}

void ShadowManager::loadConfiguration()
{
	Ogre::String s = SETTINGS.getSetting("Shadow technique");
	if (s == "Stencil shadows")            changeShadowTechnique(Ogre::SHADOWTYPE_STENCIL_MODULATIVE);
	if (s == "Texture shadows")            changeShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
	if (s == "Parallel-split Shadow Maps") changeShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
}

int ShadowManager::changeShadowTechnique(Ogre::ShadowTechnique tech)
{
	float shadowFarDistance = Ogre::StringConverter::parseInt(SETTINGS.getSetting("Shadow distance"));
	float scoef=0.2;
	mSceneMgr->setShadowColour(Ogre::ColourValue(0.563+scoef, 0.578+scoef, 0.625+scoef));

	mSceneMgr->setShadowTechnique(tech);
	mSceneMgr->setShadowFarDistance(shadowFarDistance);
	mSceneMgr->setShowDebugShadows(false);

	if(tech == Ogre::SHADOWTYPE_STENCIL_MODULATIVE)
	{
		//		mSceneMgr->setShadowIndexBufferSize(2000000);
		mSceneMgr->setShadowDirectionalLightExtrusionDistance(100);

		//important optimization
		mSceneMgr->getRenderQueue()->getQueueGroup(Ogre::RENDER_QUEUE_WORLD_GEOMETRY_1)->setShadowsEnabled(false);

		//		mSceneMgr->setUseCullCamera(false);
		//		mSceneMgr->setShowBoxes(true);
		//		mSceneMgr->showBoundingBoxes(true);
	} else if(tech == Ogre::SHADOWTYPE_TEXTURE_MODULATIVE)
	{
		mSceneMgr->setShadowTextureSettings(2048,2);
	} else if(tech == Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED)
	{
#if OGRE_VERSION>0x010602

		Ogre::TerrainMaterialGeneratorA::SM2Profile *matProfile  = 0;
		if(Ogre::TerrainGlobalOptions::getSingletonPtr())
		{
			matProfile = static_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile*>(Ogre::TerrainGlobalOptions::getSingleton().getDefaultMaterialGenerator()->getActiveProfile());
			matProfile->setReceiveDynamicShadowsEnabled(true);
			matProfile->setReceiveDynamicShadowsLowLod(false);
		}


		// General scene setup

		// 3 textures per directional light (PSSM)
		mSceneMgr->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, 3);

		if (mPSSMSetup.isNull())
		{
			// shadow camera setup
			Ogre::PSSMShadowCameraSetup* pssmSetup = new Ogre::PSSMShadowCameraSetup();
			pssmSetup->setSplitPadding(mCamera->getNearClipDistance());
			pssmSetup->calculateSplitPoints(3, mCamera->getNearClipDistance(), mSceneMgr->getShadowFarDistance());
			pssmSetup->setOptimalAdjustFactor(0, 2);
			pssmSetup->setOptimalAdjustFactor(1, 1);
			pssmSetup->setOptimalAdjustFactor(2, 0.5);

			mPSSMSetup.bind(pssmSetup);

		}
		mSceneMgr->setShadowCameraSetup(mPSSMSetup);
		
		bool depthShadows = true;
		if (depthShadows)
		{
			mSceneMgr->setShadowTextureCount(3);
			mSceneMgr->setShadowTextureConfig(0, 2048, 2048, Ogre::PF_FLOAT32_R);
			mSceneMgr->setShadowTextureConfig(1, 1024, 1024, Ogre::PF_FLOAT32_R);
			mSceneMgr->setShadowTextureConfig(2, 1024, 1024, Ogre::PF_FLOAT32_R);
			mSceneMgr->setShadowTextureSelfShadow(true);
			mSceneMgr->setShadowCasterRenderBackFaces(true);
			mSceneMgr->setShadowTextureCasterMaterial("PSSM/shadow_caster");

			/*
			MaterialPtr houseMat = buildDepthShadowMaterial("fw12b.jpg");
			for (EntityList::iterator i = mHouseList.begin(); i != mHouseList.end(); ++i)
			{
				(*i)->setMaterial(houseMat);
			}
			*/
		}
		else
		{
			mSceneMgr->setShadowTextureCount(3);
			mSceneMgr->setShadowTextureConfig(0, 2048, 2048, Ogre::PF_X8B8G8R8);
			mSceneMgr->setShadowTextureConfig(1, 1024, 1024, Ogre::PF_X8B8G8R8);
			mSceneMgr->setShadowTextureConfig(2, 1024, 1024, Ogre::PF_X8B8G8R8);
			mSceneMgr->setShadowTextureSelfShadow(false);
			mSceneMgr->setShadowCasterRenderBackFaces(false);
			mSceneMgr->setShadowTextureCasterMaterial(Ogre::StringUtil::BLANK);
		}

		if(matProfile)
		{
			matProfile->setReceiveDynamicShadowsDepth(depthShadows);
			matProfile->setReceiveDynamicShadowsPSSM(static_cast<Ogre::PSSMShadowCameraSetup*>(mPSSMSetup.get()));
		}

		/*
		Ogre::ResourceManager::ResourceMapIterator RI = Ogre::MaterialManager::getSingleton().getResourceIterator();
		while (RI.hasMoreElements())
		{
			Ogre::MaterialPtr mat = RI.getNext();
			if(mat.isNull()) continue;
			if(!mat->getNumTechniques()) continue;
			if (mat->getTechnique(0)->getPass("SkyLight") != NULL)
				mat->getTechnique(0)->getPass("SkyLight")->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);
		}
		*/

#else
		showError("Parallel-split Shadow Maps as shadow technique is only available when you build with Ogre 1.6 support.", "PSSM error");
		exit(1);
#endif //OGRE_VERSION
	}
	return 0;
}
