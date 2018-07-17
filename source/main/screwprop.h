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
#ifndef __Screwprop_H__
#define __Screwprop_H__

#include "Ogre.h"
#include "water.h"

using namespace Ogre;
#include "Beam.h"
#include "SoundScriptManager.h"
#include "DustPool.h"
#include "DustManager.h"

class Screwprop : public MemoryAllocatedObject
{
private:
	node_t *nodes;
	int nodeback;
	int nodeup;
	float fullpower; //in HP
	Water* water;
	int trucknum;
	DustPool *splashp, *ripplep;

public:
	int noderef;

	bool reverse;
	float throtle;
	float rudder;

	Screwprop(node_t *nd, int nr, int nb, int nu, float power, Water* w, int trucknum);
	void updateForces(int update);
	void setThrotle(float val);
	void setRudder(float val);
	float getThrotle();
	float getRudder();
	void reset();
	void toggleReverse();
};

#endif
