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
#ifndef __Turboprop_H__
#define __Turboprop_H__

#include "Ogre.h"
#include "SoundScriptManager.h"

using namespace Ogre;
#include "Beam.h"
#include "Airfoil.h"
#include "aeroengine.h"

class Turboprop: public AeroEngine, public MemoryAllocatedObject
{
private:
	node_t *nodes;
	int nodeback;
	int nodep[4];
	int torquenode;
	float torquedist;
	Airfoil *airfoil;
	float fullpower; //in kW
	float proparea;
	float airdensity;
	float timer;
	float lastflip;
	float warmupstart;
	float warmuptime;
	int number;
	int numblades;
	float bladewidth;
	float pitchspeed;
	float maxrevpitch;
	float regspeed;
	SceneNode* vspinner;
	SceneNode* vpales[4];
	int free_vpale;
	ParticleSystem* smokePS;
	ParticleSystem* heathazePS;
    SceneNode *smokeNode;
	float twistmap[5];
	double rotenergy;
	float fixed_pitch;

	bool reverse;
	bool warmup;
	bool ignition;
	float radius;
	bool failed;
	bool failedold;
	float rpm;
	float throtle;
	int noderef;
	char debug[256];
	float propwash;
	Vector3 axis;
	bool heathaze;
	SoundScriptManager *ssm;
	int trucknum;
	int mod_id;
	int src_id;
	int thr_id;
public:
	bool is_piston;
	float pitch;
	float indicated_torque;
	float max_torque;

	Turboprop(SceneManager *manager, char* propname, node_t *nd, int nr, int nb, int np1, int np2, int np3, int np4, int tqn, float power, char* propfoilname, int mnumber, int trucknum, bool disable_smoke, bool ispiston, float fpitch, bool heathaze);

	void updateVisuals();
	void updateForces(float dt, int doUpdate);

	void setThrotle(float val);

	float getThrotle();

	void reset();

	void toggleReverse();

	void flipStart();

	void addPale(SceneNode* sn);

	void addSpinner(SceneNode* sn);

	float getRPM() { return rpm; };
	float getRPMpc() { return rpm/10.0; };
	void setRPM(float _rpm);
	float getpropwash() {return propwash;};
	Vector3 getAxis() {return axis;};
	bool isFailed() {return failed;};
	int getType() {return AEROENGINE_TYPE_TURBOPROP;};
	bool getIgnition() {return ignition;};
	int getNoderef() {return noderef;};
	bool getWarmup() {return warmup;};
	float getRadius() {return radius;};
};

#endif
