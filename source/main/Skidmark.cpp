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

#include "Skidmark.h"
#include "heightfinder.h"
#include <Ogre.h>
#include <cassert>
#include <cmath>

//using namespace Ogre;
//using namespace std;
int Skidmark::instancecounter = 0;

template<> SkidmarkManager *Ogre::Singleton<SkidmarkManager>::ms_Singleton=0;

SkidmarkManager::SkidmarkManager()
{
	Ogre::LogManager::getSingleton().logMessage("SkidmarkManager created");
	loadDefaultModels();
}

SkidmarkManager::~SkidmarkManager()
{
	Ogre::LogManager::getSingleton().logMessage("SkidmarkManager destroyed");
}

int SkidmarkManager::loadDefaultModels()
{
	Ogre::LogManager::getSingleton().logMessage("SkidmarkManager loading default models");
	// check if we have a config file
	Ogre::String group = "";
	try
	{
		group = Ogre::ResourceGroupManager::getSingleton().findGroupContainingResource("skidmarks.cfg");
	} catch(...)
	{
	}
	// emit a warning if we did not found the file
	if (group.empty())
	{
		Ogre::LogManager::getSingleton().logMessage("skidmarks| skidmarks.cfg not found");
		return 1;
	}

	// open the file for reading
	Ogre::DataStreamPtr ds = Ogre::ResourceGroupManager::getSingleton().openResource("skidmarks.cfg", group);
	Ogre::String line = "";
	Ogre::String currentModel = "";

	while (!ds->eof())
	{	
		line=ds->getLine();
		Ogre::StringUtil::trim(line);
		
		if (line.empty() || line[0]==';')
			continue;

		Ogre::vector< Ogre::String >::type args = Ogre::StringUtil::split(line, ",");
		
		if (args.size() == 1)
		{
			currentModel = line;
			continue;
		}
		
		// process the line if we got a model
		if(!currentModel.empty())
			processLine(args, currentModel);
	}
	return 0;
}

int SkidmarkManager::processLine(Ogre::vector< Ogre::String >::type args,  Ogre::String modelName)
{
	// we only accept 4 arguments
	if (args.size() != 4)
		return 1;
	
	// parse the data
	skidmark_config_t cfg;
	cfg.ground = args[0];
	Ogre::StringUtil::trim(cfg.ground);
	cfg.texture = args[1];
	Ogre::StringUtil::trim(cfg.texture);

	cfg.slipFrom = Ogre::StringConverter::parseReal(args[2]);
	cfg.slipTo = Ogre::StringConverter::parseReal(args[3]);

	if(!models.size() || models.find(modelName) == models.end())
		models[modelName] = std::vector<skidmark_config_t>();

	models[modelName].push_back(cfg);
	return 0;
}

int SkidmarkManager::getTexture(Ogre::String model, Ogre::String ground, float slip, Ogre::String &texture)
{
	if(models.find(model) == models.end()) return 1;
	for(std::vector<skidmark_config_t>::iterator it=models[model].begin(); it!=models[model].end(); it++)
	{
		if(it->ground == ground && it->slipFrom <= slip && it->slipTo > slip)
		{
			texture = it->texture;
			return 0;
		}
	}
	return 2;
}

/////////////// Skidmark below

// this is a hardcoded array which we use to map ground types to a certain texture with UV/ coords
Ogre::Vector2 Skidmark::tex_coords[4] = {Vector2(0,0), Vector2(0,1), Vector2(1,0), Vector2(1,1)};

Skidmark::Skidmark(SceneManager *scm
, wheel_t *wheel
, HeightFinder *hfinder
, Ogre::SceneNode *snode
, int _lenght
, int bucketCount)
: scm(scm)
, mNode(snode)
, hfinder(hfinder)
, mDirty(true)
, lenght(_lenght%2 ? _lenght - (_lenght%2) : _lenght)
, bucketCount(bucketCount)
, wheel(wheel)
, minDistance(0.1f)
, maxDistance(std::max(0.5f, wheel->width*1.1f))
, minDistanceSquared(minDistance * minDistance)
, maxDistanceSquared(maxDistance * maxDistance)
{
	if(lenght%2) lenght -= lenght%2; // round it!
}

Skidmark::~Skidmark()
{
}

void Skidmark::addObject(Ogre::Vector3 start, Ogre::String texture)
{
	//LogManager::getSingleton().logMessage("new skidmark section");
	skidmark_t skid;
	skid.pos=0;
	skid.lastPointAv=start;
	skid.facecounter=0;
	for(int i=0;i<3;i++) skid.face[i] = Ogre::Vector3::ZERO;
	skid.colour = ColourValue(Ogre::Math::RangeRandom(0, 100)/100.0f, Ogre::Math::RangeRandom(0, 100)/100.0f, Ogre::Math::RangeRandom(0, 100)/100.0f, 0.8f);


	// new material
	char bname[255]="";
	sprintf(bname, "mat-skidmark-%d", instancecounter);
	Ogre::MaterialPtr mat=(Ogre::MaterialPtr)(Ogre::MaterialManager::getSingleton().create(bname, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
	Ogre::Pass *p = mat->getTechnique(0)->getPass(0);
	
	p->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
	p->setLightingEnabled(false);
	p->setDepthWriteEnabled(false);
	p->setDepthBias(3, 3);
	p->setCullingMode(Ogre::CULL_NONE);

	skid.points.resize(lenght);
	skid.facesizes.resize(lenght);
	skid.ground_texture.resize(lenght);
	skid.obj = scm->createManualObject("skidmark" + StringConverter::toString(instancecounter++));
	skid.obj->setDynamic(true);
	skid.obj->setRenderingDistance(2000); //2km sight range
	skid.obj->begin(bname, RenderOperation::OT_TRIANGLE_STRIP);
	for(int i = 0; i < lenght; i++)
	{
		skid.points[i] = start;
		skid.facesizes[i] = 0;
		skid.ground_texture[i] = "0.png";
		skid.obj->position(start);
		skid.obj->textureCoord(0,0);
	}
    skid.obj->end();
	mNode->attachObject(skid.obj);


	objects.push(skid);

	limitObjects();
}

void Skidmark::limitObjects()
{
	if((int)objects.size() > bucketCount)
	{
		//LogManager::getSingleton().logMessage("deleting first skidmarks section to keep the limits");
		objects.front().points.clear();
		objects.front().facesizes.clear();
		scm->destroyManualObject(objects.front().obj);
		objects.pop();
	}
}

void Skidmark::setPointInt(unsigned short index, const Vector3 &value, Real fsize, String texture)
{
	objects.back().points[index] = value;
	objects.back().facesizes[index] = fsize;
	objects.back().ground_texture[index] = texture;

	mDirty = true;
}

void Skidmark::updatePoint()
{
	Ogre::Vector3 thisPoint = wheel->lastContactType?wheel->lastContactOuter:wheel->lastContactInner;
	Ogre::Vector3 axis = wheel->lastContactType?(wheel->refnode1->RelPosition - wheel->refnode0->RelPosition):(wheel->refnode0->RelPosition - wheel->refnode1->RelPosition);
	Ogre::Vector3 thisPointAV = thisPoint + axis * 0.5f;
	Ogre::Real distance = 0;
	Ogre::Real maxDist = maxDistance;
	Ogre::String texture = "none";
	SkidmarkManager::getSingleton().getTexture("default", wheel->lastGroundModel->name, wheel->lastSlip, texture);
	
	// dont add points with no texture
	if(texture == "none") return;

	if(wheel->speed > 1) maxDist *= wheel->speed;

	if(!objects.size())
	{
		// add first bucket
		addObject(thisPoint, texture);
	} else
	{
		// check existing buckets
		skidmark_t skid = objects.back();

		distance = skid.lastPointAv.distance(thisPointAV);
		// too near to update?
		if(distance < minDistance)
		{
			//LogManager::getSingleton().logMessage("E: too near for update");
			return;
		}
		
		// change ground texture if required
		if(skid.pos > 0 && skid.ground_texture[0] != texture)
		{
			// new object with new texture
			if(distance > maxDist)
			{
				// to far away for connection
				addObject(thisPoint, texture);
			} else
			{
				// add new bucket with connection to last bucket
				Ogre::Vector3 lp1 = objects.back().points[objects.back().pos-1];
				Ogre::Vector3 lp2 = objects.back().points[objects.back().pos-2];
				addObject(lp1, texture);
				addPoint(lp2, distance, texture);
				addPoint(lp1, distance, texture);
			}
		} else
		{
			// no texture change required :D

			// far enough for new bucket?
			if(skid.pos >= (int)skid.points.size())
			{
				if(distance > maxDist)
				{
					// to far away for connection
					addObject(thisPoint, texture);
				} else
				{
					// add new bucket with connection to last bucket
					Ogre::Vector3 lp1 = objects.back().points[objects.back().pos-1];
					Ogre::Vector3 lp2 = objects.back().points[objects.back().pos-2];
					addObject(lp1, texture);
					addPoint(lp2, distance, texture);
					addPoint(lp1, distance, texture);
				}
			}
			else if(distance > maxDistance)
			{
				// just new bucket, no connection to last bucket
				addObject(thisPoint, texture);
			}
		}
	}

	skidmark_t skid = objects.back();

	float overaxis = 0.2f;
	// tactics: we always choose the latest oint and then create two points
	
	// XXX: TO BE IMPROVED
	//Ogre::Vector3 groundNormal = Vector3::ZERO;
	//hfinder->getNormalAt(thisPoint.x, thisPoint.y, thisPoint.z, &groundNormal);

	//Ogre::LogManager::getSingleton().logMessage("ground normal: "+StringConverter::toString(wheel->refnode1->RelPosition.dotProduct(groundNormal)));

	// choose node wheel by the latest added point
	if(!wheel->lastContactType)
	{
		// choose inner
		//LogManager::getSingleton().logMessage("inner");
		addPoint(wheel->lastContactInner - (axis * overaxis), distance, texture);
		addPoint(wheel->lastContactInner + axis + (axis * overaxis), distance, texture);
	} else
	{
		// choose outer
		//LogManager::getSingleton().logMessage("outer");
		addPoint(wheel->lastContactOuter + axis + (axis * overaxis), distance, texture);
		addPoint(wheel->lastContactOuter - (axis * overaxis), distance, texture);
	}

	// save as last point (in the middle of the wheel)
	objects.back().lastPointAv = thisPointAV;

	/*
	// debug code: adds boxes to the average point
	SceneNode *sn = mNode->getParentSceneNode()->createChildSceneNode();
	sn->attachObject(scm->createEntity("addPointTRACK"+StringConverter::toString(objects.back().lastPointAv) +StringConverter::toString(axis), "beam.mesh"));
	sn->setPosition(thisPointAV);
	sn->setScale(0.1f, 0.01f, 0.1f);
	*/

}

void Skidmark::addPoint(const Vector3 &value, Real fsize, String texture)
{
	if(objects.back().pos >= lenght)
	{
		//LogManager::getSingleton().logMessage("E: boundary protection hit");
		return;
	}
	setPointInt(objects.back().pos, value, fsize, texture);
	objects.back().pos++;
}

void Skidmark::update()
{
	if(!objects.size()) return;
	skidmark_t skid = objects.back();
	Ogre::Vector3 vaabMin = skid.points[0];
	Ogre::Vector3 vaabMax = skid.points[0];
	skid.obj->beginUpdate(0);
	bool behindEnd = false;
	Ogre::Vector3 lastValid = Ogre::Vector3::ZERO;
	int to_counter=0;
	float tcox_counter=0;

	for(int i = 0; i < lenght; i++, to_counter++)
	{
		if(i>=skid.pos) behindEnd=true;

		if(to_counter>3)
			to_counter=0;

		if(!behindEnd)
			tcox_counter += skid.facesizes[i] / minDistance;

		while(tcox_counter>1)
			tcox_counter--;

		if(behindEnd)
		{
			skid.obj->position(lastValid);
			skid.obj->textureCoord(0,0);
		} else
		{
			skid.obj->position(skid.points[i]);

			Vector2 tco = tex_coords[to_counter];
			tco.x *= skid.facesizes[i] / minDistance; // scale texture according face size
			skid.obj->textureCoord(tco);

			lastValid = skid.points[i];
		}
		
		if(skid.points[i].x < vaabMin.x) vaabMin.x = skid.points[i].x;
		if(skid.points[i].y < vaabMin.y) vaabMin.y = skid.points[i].y;
		if(skid.points[i].z < vaabMin.z) vaabMin.z = skid.points[i].z;
		if(skid.points[i].x > vaabMax.x) vaabMax.x = skid.points[i].x;
		if(skid.points[i].y > vaabMax.y) vaabMax.y = skid.points[i].y;
		if(skid.points[i].z > vaabMax.z) vaabMax.z = skid.points[i].z;
	}
    skid.obj->end();

	skid.obj->setBoundingBox(AxisAlignedBox(vaabMin, vaabMax));
	
	// Use infinite AAB to always stay visible
	/*
	// for debugging only
	AxisAlignedBox aabInf;
	aabInf.setInfinite();
	skid.obj->setBoundingBox(aabInf);
	*/

	mDirty = false;
}

