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
#include "heightfinder.h"
//#include <OgreTerrainSceneManager.h>
#include "rormemory.h"
#include <Ogre.h>
#include <OgreConfigFile.h>
#include <OgreLogManager.h>
#include <OgreStringConverter.h>

using namespace Ogre;

// this is the Height-Finder for the standart ogre Terrain Manager

TSMHeightFinder::TSMHeightFinder(char *_cfgfilename, char *fname, float defaultheight)
{
	flipped=false;
	//cfgfilename = _cfgfilename; this is a bug
	strcpy(cfgfilename, _cfgfilename);
	defaulth=defaultheight;
//	scale=tsm->getScale();
	//this is not correct, but it works
//	size=tsm->getPageSize();
    ConfigFile config;
    String val;
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	DataStreamPtr stream=rgm.openResource(cfgfilename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    config.load( stream );
    val = config.getSetting( "PageSize" );
	if ( !val.empty() )
	{
		size=atoi(val.c_str());
		size1 = size - 1;
	}
    scale = Vector3::UNIT_SCALE;

    val = config.getSetting( "PageWorldX" );
    if ( !val.empty() )
        scale.x = atof( val.c_str() );

    val = config.getSetting( "MaxHeight" );
    if ( !val.empty() )
	{
        scale.y = atof( val.c_str() );
	}

    val = config.getSetting( "PageWorldZ" );
    if ( !val.empty() )
        scale.z = atof( val.c_str() );
    // Scale x/z relative to pagesize
	scale.x /= size1;
	scale.z /= size1;

	inverse_scale.x=1.0/scale.x;
	inverse_scale.y=scale.y/65535.0;
	inverse_scale.z=1.0/scale.z;

	data=(unsigned short*)ror_malloc(size*size*2);
	DataStreamPtr ds=rgm.openResource(fname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	ds->read(data, size*size*2);
	// ds closes automatically, so do not close it explicitly here
	loadSettings();
}

void TSMHeightFinder::loadSettings()
{
	ConfigFile cfg;
	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(String(cfgfilename), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	cfg.load(ds, "\t:=", false);
	flipped=(cfg.getSetting("Heightmap.flip")=="true");
	LogManager::getSingleton().logMessage("loading HeightFinder configuration from " + String(cfgfilename) + " flipped: " + StringConverter::toString(flipped));
}

TSMHeightFinder::~TSMHeightFinder()
{
	if (data) ror_free(data);
}

float TSMHeightFinder::getHeightAt(float x, float z)
{

	if (x < 0 || z < 0) return defaulth;

	float rx=x*inverse_scale.x;
	float rz=z*inverse_scale.z;

	if (rx >= size1 || rz >= size1) return defaulth;

	int x_index=(int)rx;
	int z_index=(int)rz;

	//dx=irx; dz=irz;
	float t1=0, t2=0, b1=0, b2=0;
	if(!flipped)
	{
	  	int z_i = z_index * size;
		int z_iPlus1 = z_i + size;
		t1 = data[x_index+z_i];
		t2 = data[x_index+1+z_i];
		b1 = data[x_index+z_iPlus1];
		b2 = data[x_index+1+z_iPlus1];
	} 
	 else
	{
		int rez_i = ( size1 - z_index ) * size;
		int rez_i1 = rez_i - size;
		t1 = data[x_index+rez_i];
		t2 = data[x_index+1+rez_i];
		b1 = data[x_index+rez_i1];
		b2 = data[x_index+1+rez_i1];
	}

	//		dx=irx; dz=x_pct;
	float x_pct=rx-x_index;
	float z_pct=rz-z_index;

	if (x_pct + z_pct <= 1) {
		b2 = b1 + t2 - t1;
	} else {
		t1 = b1 + t2 - b2;
	}

	float t = t1 + x_pct * ( t2 - t1 );
	float b = b1 + x_pct * ( b2 - b1 );

	float h = (t + z_pct * ( b - t )) * inverse_scale.y;

	if (h<defaulth) h=defaulth;
	return h;
}
