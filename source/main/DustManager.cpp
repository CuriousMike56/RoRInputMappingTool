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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 12th of October 2009

#include "DustManager.h"
#include "Ogre.h"
#include "DustPool.h"
#include "Settings.h"
#include "collisions.h"
#include "BeamData.h"

#include "language.h"

using namespace Ogre;

template<> DustManager * Singleton< DustManager >::ms_Singleton = 0;
DustManager* DustManager::getSingletonPtr(void)
{
	return ms_Singleton;
}
DustManager& DustManager::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

DustManager::DustManager(Ogre::SceneManager *mSceneMgr) : mSceneMgr(mSceneMgr), mEnabled(false)
{
	mEnabled = (SETTINGS.getSetting("Dust") == "Yes");

	
	if (SETTINGS.getSetting("Dust")=="Yes")
	{
		dustpools["dust"]   = new DustPool((char *)"tracks/Dust", 20, mSceneMgr->getRootSceneNode(), mSceneMgr, w);
		dustpools["clump"]  = new DustPool((char *)"tracks/Clump", 20, mSceneMgr->getRootSceneNode(), mSceneMgr, w);
		dustpools["sparks"] = new DustPool((char *)"tracks/Sparks", 10, mSceneMgr->getRootSceneNode(), mSceneMgr, w);
	}
	if (SETTINGS.getSetting("Spray")=="Yes")
	{
		dustpools["drip"]   = new DustPool((char *)"tracks/Drip", 50, mSceneMgr->getRootSceneNode(), mSceneMgr, w);
		dustpools["splash"] = new DustPool((char *)"tracks/Splash", 20, mSceneMgr->getRootSceneNode(), mSceneMgr, w);
		dustpools["ripple"] = new DustPool((char *)"tracks/Ripple", 20, mSceneMgr->getRootSceneNode(), mSceneMgr, w);
	}

}

DustManager::~DustManager()
{
	// delete all created dustpools and remove them
	std::map < Ogre::String , DustPool * >::iterator it;
	for(it=dustpools.begin(); it!=dustpools.end();it++)
	{
		// delete the DustPool instance
		delete(it->second);
		it->second = 0;
	}
	// then clear the vector
	dustpools.clear();
}

DustPool *DustManager::getGroundModelDustPool(ground_model_t *g)
{
	return 0;

	/*
	// disabled for now...
	if(!mEnabled) return 0;

	// if we have a non particle type, then return 0
	if(g->fx_type != FX_PARTICLE) return 0;

	String pname = String(g->particle_name);
	if(!dustpools.size() || dustpools.find(pname) == dustpools.end())
	{
		addNewDustPool(g);
	}
	// return the entry we have
	return dustpools[pname];
	*/
}

/*
void DustManager::addNewDustPool(ground_model_t *g)
{
	if(!mEnabled) return;

	// simple and clean addition of a new dustpool class instance
	String pname = String(g->particle_name);
	DustPool *dp = new DustPool(pname,
								g->fx_particle_amount,
								g->fx_particle_min_velo,
								g->fx_particle_max_velo,
								g->fx_particle_fade,
								g->fx_particle_timedelta,
								g->fx_particle_velo_factor,
								g->fx_particle_ttl,
								mSceneMgr->getRootSceneNode(),
								mSceneMgr);
	dustpools[pname] = dp;
}
*/

void DustManager::update(float wspeed)
{
	if(!mEnabled) return;
	std::map < Ogre::String , DustPool * >::iterator it;
	for(it=dustpools.begin(); it!=dustpools.end();it++)
	{
		it->second->update(wspeed);
	}
}

void DustManager::setWater(Water *w)
{
	if(!mEnabled) return;
	this->w = w;
	std::map < Ogre::String , DustPool * >::iterator it;
	for(it=dustpools.begin(); it!=dustpools.end();it++)
	{
		it->second->setWater(w);
	}
}

void DustManager::setVisible(bool visible)
{
	if(!mEnabled) return;
	std::map < Ogre::String , DustPool * >::iterator it;
	for(it=dustpools.begin(); it!=dustpools.end();it++)
	{
		it->second->setVisible(visible);
	}
}

DustPool *DustManager::getDustPool(Ogre::String name)
{
	if(dustpools.find(name) == dustpools.end())
		return 0;
	return dustpools[name];
}
