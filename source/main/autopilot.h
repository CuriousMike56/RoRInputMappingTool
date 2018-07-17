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
#ifndef __Autopilot_H__
#define __Autopilot_H__

#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif
#include <math.h>
#include "heightfinder.h"
#include "water.h"
#include "SoundScriptManager.h"
#include "Beam.h"
#include "RoRFrameListener.h"
#include <stdio.h>
using namespace Ogre;

#define HEADING_NONE 0
#define HEADING_FIXED 1
#define HEADING_WLV 2
#define HEADING_NAV 3

#define ALT_NONE 0
#define ALT_FIXED 1
#define ALT_VS 2

class Autopilot : public MemoryAllocatedObject
{
private:
	int mode_heading;
	int mode_alt;
	bool mode_ias;
	bool mode_gpws;
	int alt;
	int vs;
	int ias;
	node_t *ref_l;
	node_t *ref_r;
	node_t *ref_b;
	float ref_span;
	float last_elevator;
	float last_aileron;
	float last_rudder;
	HeightFinder *hf;
	float last_gpws_height;
	float last_pullup_height;
	Water *water;
	float lastradiov;
	float lastradioh;
	float lastradiorwh;
	float lastradiorwd;
	float last_closest_hdist;
	
	int trucknum;
	SoundScriptManager *ssm;

public:
	int heading;
	bool wantsdisconnect;

	Autopilot(HeightFinder *hfd, Water *w, int trucknum);
	void reset();
	void disconnect();
	void setInertialReferences(node_t *refl, node_t *refr, node_t *refb);
	int toggleHeading(int mode);
	int toggleAlt(int mode);
	bool toggleIAS();
	bool toggleGPWS();
	int adjHDG(int d);
	int adjALT(int d);
	int adjVS(int d);
	int adjIAS(int d);

	float getAilerons();
	float getElevator();
	float getRudder();
	float getThrotle(float thrtl, float dt);

	void gpws_update();

	void getRadioFix(localizer_t *localizers, int free_localizer, float *vdev, float *hdev);
};


#endif
