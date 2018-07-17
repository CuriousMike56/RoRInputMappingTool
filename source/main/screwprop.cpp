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
#include "screwprop.h"

Screwprop::Screwprop(node_t *nd, int nr, int nb, int nu, float power, Water* w, int trucknum)
{
	this->trucknum=trucknum;
	nodes=nd;
	noderef=nr;
	nodeback=nb;
	nodeup=nu;
	fullpower=power;
	water=w;
	splashp = DustManager::getSingleton().getDustPool("splash");
	ripplep = DustManager::getSingleton().getDustPool("ripple");
	reset();
}


void Screwprop::updateForces(int update)
{
	if (!water) return;
	float depth=water->getHeightWaves(nodes[noderef].AbsPosition)-nodes[noderef].AbsPosition.y;
	if (depth<0) return; //out of water!
	Vector3 dir=nodes[nodeback].RelPosition-nodes[noderef].RelPosition;
	Vector3 rudaxis=nodes[noderef].RelPosition-nodes[nodeup].RelPosition;
	dir.normalise();
	if (reverse) dir=-dir;
	rudaxis.normalise();
	dir=(throtle*fullpower)*(Quaternion(Degree(rudder),rudaxis)*dir);
	nodes[noderef].Forces+=dir;
	if (update && splashp && throtle>0.1)
	{
		if (depth<0.2) splashp->allocSplash(nodes[noderef].AbsPosition, 10.0*dir/fullpower);
		ripplep->allocRipple(nodes[noderef].AbsPosition, 10.0*dir/fullpower);
	}
}

void Screwprop::setThrotle(float val)
{
	if (val>1.0) val=1.0;
	if (val<-1.0) val=-1.0;
	throtle=fabs(val);
	reverse=(val<0);
	//pseudo-rpm
	float prpm=(0.5+fabs(val)/2.0)*100.0;
#ifdef USE_OPENAL
	SoundScriptManager *ssm = SoundScriptManager::getSingleton();
	if(ssm) ssm->modulate(trucknum, SS_MOD_ENGINE, prpm);
#endif //OPENAL
}

void Screwprop::setRudder(float val)
{
	if (val>1.0) val=1.0;
	if (val<-1.0) val=-1.0;
	rudder=val*45.0;
}

float Screwprop::getThrotle()
{
	if (reverse) return -throtle; else return throtle;
}

float Screwprop::getRudder()
{
	return rudder/45.0;
}

void Screwprop::reset()
{
	setThrotle(0);
	//throtle=0;
	rudder=0;
	reverse=false;
}

void Screwprop::toggleReverse()
{
	throtle=0;
	reverse=!reverse;
}

