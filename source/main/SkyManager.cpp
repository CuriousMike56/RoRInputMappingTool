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

#ifdef USE_CAELUM

#include "SkyManager.h"

#include "Ogre.h"
#include "Caelum.h"

using namespace std;
using namespace Ogre;
using namespace Caelum;

//---------------------------------------------------------------------
template<> SkyManager * Singleton< SkyManager >::ms_Singleton = 0;
SkyManager* SkyManager::getSingletonPtr(void)
{
	return ms_Singleton;
}
SkyManager& SkyManager::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

SkyManager::SkyManager()
{
}

SkyManager::~SkyManager()
{
}

void SkyManager::init(Ogre::SceneManager *mScene, Ogre::RenderWindow *mWindow, Ogre::Camera *mCamera)
{
	// Initi5alise CaelumSystem.
	mCaelumSystem = new Caelum::CaelumSystem (Root::getSingletonPtr(), mScene, Caelum::CaelumSystem::CAELUM_COMPONENTS_NONE);
	mCaelumSystem->attachViewport(mCamera->getViewport());

	/*
	// TODO: set real time, and let the user select his true location
	mCaelumSystem->getUniversalClock()->setGregorianDateTime(2008, 4, 9, 6, 33, 0);
	mCaelumSystem->setObserverLongitude(Ogre::Degree(0));
	mCaelumSystem->setObserverLatitude(Ogre::Degree(0));
	mCaelumSystem->getUniversalClock()->setTimeScale(100);
	*/

	// Register caelum as a listener.
	mWindow->addListener (mCaelumSystem);
	Root::getSingletonPtr()->addFrameListener(mCaelumSystem);
}

void SkyManager::loadScript(Ogre::String script)
{
	// load the caelum config
	try
	{
		CaelumPlugin::getSingleton().loadCaelumSystemFromScript (mCaelumSystem, script);


		// overwrite some settings
		if(mCaelumSystem->getMoon())
		{
			mCaelumSystem->getMoon()->setAutoDisable(true);
			mCaelumSystem->getMoon()->setAutoDisableThreshold(1);
			mCaelumSystem->getMoon()->setForceDisable(true);
			mCaelumSystem->getMoon()->getMainLight()->setCastShadows(false);
		}

		mCaelumSystem->setEnsureSingleShadowSource(true);
		mCaelumSystem->setEnsureSingleLightSource(true);

	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("exception upon loading sky script: " + e.getFullDescription());
	}
}

void SkyManager::setTimeFactor(LongReal factor)
{
    mCaelumSystem->getUniversalClock()->setTimeScale (factor);
}

Ogre::Light *SkyManager::getMainLight()
{
	if(mCaelumSystem && mCaelumSystem->getSun())
		return mCaelumSystem->getSun()->getMainLight();
	return 0;
}
LongReal SkyManager::getTimeFactor()
{
    return mCaelumSystem->getUniversalClock()->getTimeScale();
}

Ogre::String SkyManager::getPrettyTime()
{
	int ignore;
	int hour;
	int minute;
	Caelum::LongReal second;
	Caelum::Astronomy::getGregorianDateTimeFromJulianDay(mCaelumSystem->getJulianDay()
	, ignore, ignore, ignore, hour, minute, second);
	
	return Ogre::StringConverter::toString( hour, 2, '0' )
	+ ":" + Ogre::StringConverter::toString( minute, 2, '0' )
	+ ":" + Ogre::StringConverter::toString( (int)second, 2, '0' );
}

#endif //USE_CAELUM
