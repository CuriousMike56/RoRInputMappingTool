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
#include "Road.h"
void Road::preparePending()
	{
		//setup rotation points
		lastpturn=pturn;
		protl=ppos+Quaternion(Degree(pturn), Vector3::UNIT_Y)*Vector3(0, 0, 4.5);
		protr=ppos+Quaternion(Degree(pturn), Vector3::UNIT_Y)*Vector3(0, 0, -4.5);
		tenode->setPosition(ppos);
		tenode->setOrientation(Quaternion(Degree(pturn), Vector3::UNIT_Y)*Quaternion(Degree(ppitch), Vector3::UNIT_Z));
		tenode->pitch(Degree(-90));
	}

	void Road::updatePending()
	{
		if (pturn-lastpturn>0) tenode->setPosition(Quaternion(Degree(pturn-lastpturn), Vector3::UNIT_Y)*(ppos-protl)+protl);
			else
			if (pturn-lastpturn<0) tenode->setPosition(Quaternion(Degree(pturn-lastpturn), Vector3::UNIT_Y)*(ppos-protr)+protr);
				else tenode->setPosition(ppos);
		tenode->setOrientation(Quaternion(Degree(pturn), Vector3::UNIT_Y)*Quaternion(Degree(ppitch), Vector3::UNIT_Z));
		tenode->pitch(Degree(-90));
	}

	Road::Road(SceneManager *scm, Vector3 start)
	{
		mSceneMgr=scm;
		free_rtype=0;
//		roadlink=0;
		ppitch=0;
		pturn=0;
		lastpturn=0;
		ppos=start;
		addRoadType("road");
		addRoadType("roadborderleft");
		addRoadType("roadborderright");
		addRoadType("roadborderboth");
		addRoadType("roadbridge");
		cur_rtype=0;
		tenode=rtypes[cur_rtype].node;
		strcpy(curtype, rtypes[cur_rtype].name);
		tenode->setVisible(true);
		preparePending();
	};

	void Road::reset(Vector3 start)
	{
		ppitch=0;
		pturn=0;
		lastpturn=0;
		ppos=start;
		cur_rtype=0;
		tenode=rtypes[cur_rtype].node;
		strcpy(curtype, rtypes[cur_rtype].name);
		tenode->setVisible(true);
		preparePending();
	}

	void Road::addRoadType(const char* name)
	{
		//create visuals
		char oname[256];
		char mname[256];
		sprintf(oname,"roadpreview-%s", name);
		sprintf(mname,"%s.mesh", name);
		Entity *te = mSceneMgr->createEntity(oname, mname);
		te->setCastShadows(false);
		rtypes[free_rtype].node=mSceneMgr->getRootSceneNode()->createChildSceneNode();
		(rtypes[free_rtype].node)->attachObject(te);
		(rtypes[free_rtype].node)->setVisible(false);
		strcpy(rtypes[free_rtype].name, name);
		free_rtype++;
	}

	void Road::toggleType()
	{
		Quaternion rot=tenode->getOrientation();
		Vector3 pos=tenode->getPosition();
		cur_rtype++;
		if (cur_rtype==free_rtype) cur_rtype=0;
		tenode->setVisible(false);
		tenode=rtypes[cur_rtype].node;
		strcpy(curtype, rtypes[cur_rtype].name);
		tenode->setOrientation(rot);
		tenode->setPosition(pos);
		tenode->setVisible(true);
	}

	void Road::dpitch(float v)
	{
		ppitch+=v;
		updatePending();
	}

	void Road::dturn(float v)
	{
		pturn+=v;
		updatePending();
	}

	void Road::append()
	{
		//register pending and set collision boxes
		//first, calculate the real position
		if (pturn-lastpturn>0) rpos=Quaternion(Degree(pturn-lastpturn), Vector3::UNIT_Y)*(ppos-protl)+protl;
			else
			if (pturn-lastpturn<0) rpos=Quaternion(Degree(pturn-lastpturn), Vector3::UNIT_Y)*(ppos-protr)+protr;
				else rpos=ppos;
		//set real rot
		rrot=Vector3(0, pturn, ppitch);
		//set new pending coordinates (we keep angles)
		ppos=rpos+(Quaternion(Degree(pturn), Vector3::UNIT_Y)*Quaternion(Degree(ppitch), Vector3::UNIT_Z))*Vector3(10,0,0);
		//prepare pending
		preparePending();
	}


	Road::~Road()
	{
	}


