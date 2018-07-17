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
#ifndef __Turbojet_H__
#define __Turbojet_H__

#include "Ogre.h"
#include "SoundScriptManager.h"
#include "materialFunctionMapper.h"

using namespace Ogre;
#include "Beam.h"
#include "Airfoil.h"
#include "aeroengine.h"
#include "rormemory.h"

class Turbojet: public AeroEngine, public MemoryAllocatedObject
{
private:
	node_t *nodes;
	int number;
	bool reverse;
	bool ignition;
	float radius;
	bool failed;
	float rpm; //in percent!
	float throtle;
	float warmupstart;
	float warmuptime;
	bool warmup;
	int nodefront;
	int nodeback;
	int noderef;
	float propwash;
	Vector3 axis;
	float reflen;
	float timer;
	float lastflip;
	bool reversable;
	bool afterburner;
	float maxdrythrust; //in kN
	float afterburnthrust; //in kN
	float exhaust_velocity; //in m/s
	float area;
	ParticleSystem* smokePS;
	ParticleSystem* heathazePS;
    SceneNode *smokeNode;

	SceneNode *nzsnode;
	SceneNode *absnode;
	bool heathaze;
	SoundScriptManager *ssm;
	int trucknum;
	int mod_id;
	int src_id;
	int thr_id;
	int ab_id;

public:
	bool afterburnable;

	Turbojet(SceneManager *manager, char* propname, int tnumber, int trucknum, node_t *nd, int tnodefront, int tnodeback, int tnoderef, float tmaxdrythrust, bool treversable, bool tafterburnable, float tafterburnthrust, float diskdiam, float nozdiam, float nozlength, bool disable_smoke, bool heathaze, MaterialFunctionMapper *mfm, Skin *usedSkin);

	void updateVisuals();
	void updateForces(float dt, int doUpdate);

	void setThrotle(float val);

	float getThrotle();

	void reset();

	void toggleReverse();

	void flipStart();

	float getRPM() { return rpm; };
	float getRPMpc() { return rpm; };
	void setRPM(float _rpm);
	float getpropwash() {return propwash;};
	Vector3 getAxis() {return axis;};
	bool isFailed() {return failed;};
	int getType() {return AEROENGINE_TYPE_TURBOJET;};
	bool getIgnition() {return ignition;};
	int getNoderef() {return nodeback;};
	bool getWarmup() {return warmup;};
	float getRadius() {return radius;};
	float getAfterburner() {return (float)afterburner;};
};

#endif
