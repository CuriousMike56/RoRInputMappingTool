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
#ifndef __AeroEngine_H__
#define __AeroEngine_H__

#include "Ogre.h"

using namespace Ogre;
#include "Beam.h"

#define AEROENGINE_TYPE_TURBOPROP 0
#define AEROENGINE_TYPE_TURBOJET 1

class AeroEngine
{
public:

//	AeroEngine(SceneManager *manager, char* propname, node_t *nd, int nr, int nb, int np1, int np2, int np3, int np4, int tqn, float power, char* propfoilname, int mnumber, BeamAudio *maudio, bool disable_smoke, bool ispiston, float fpitch);
	virtual ~AeroEngine() {};

	virtual void updateVisuals()=0;
	virtual void updateForces(float dt, int doUpdate)=0;

	virtual void setThrotle(float val)=0;

	virtual float getThrotle()=0;

	virtual void reset()=0;

	virtual void toggleReverse()=0;

	virtual void flipStart()=0;

	virtual float getRPMpc()=0;
	virtual float getRPM()=0;
	virtual void setRPM(float _rpm)=0;
	virtual float getpropwash()=0;
	virtual Vector3 getAxis()=0;
	virtual bool isFailed()=0;
	virtual int getType()=0;
	virtual bool getIgnition()=0;
	virtual int getNoderef()=0;
	virtual bool getWarmup()=0;
	virtual float getRadius()=0;
};

#endif
