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
#include "editor.h"
#include "Settings.h"
#include "OverlayWrapper.h"

Editor::Editor(SceneManager *scm, RoRFrameListener *efl)
{
	char line[1024];
	mSceneMgr=scm;
	free_rtype=0;
	ppitch=0;
	pturn=0;
	ppos=Vector3(0,0,0);
	FILE *fd;
	String editorcfg = SETTINGS.getSetting("Config Root")+"editor.cfg";
	fd=fopen(editorcfg.c_str(), "r");
	if (!fd)
	{
		LogManager::getSingleton().logMessage("Can not open editr.cfg file: "+editorcfg);
		if(efl->getOverlayWrapper())
			efl->getOverlayWrapper()->flashMessage((char *)"Can not find editor.cfg", 3);
		return;
	};
	while (!feof(fd))
	{
		int res = fscanf(fd," %[^\n]",line);
		if (line[0]==';')
		{
			continue;
		};
		if (strlen(line)<=2) continue;
		addObjectType(line);
	}
	fclose(fd);
//		addObjectType("sign-leftturn");
//		addObjectType("sign-rightturn");
//		addObjectType("sign-bump");
//		addObjectType("sign-crossing");
//		addObjectType("sign-descent");
//		addObjectType("sign-fall");
//		addObjectType("sign-rocks");
//		addObjectType("sign-warning");
//		addObjectType("sign-dir-elk-left");
//		addObjectType("sign-dir-john-cast");
//		addObjectType("sign-dir-stud-cast");
//		addObjectType("sign-dir-cargo");
//		addObjectType("sign-dir-fish");
//		addObjectType("sign-dir-cold-cast");
//		addObjectType("sign-dir-cold-left");
//		addObjectType("sign-pos-castle");
//		addObjectType("sign-pos-coldwater");
//		addObjectType("road-slab");
	cur_rtype=0;
	tenode=rtypes[cur_rtype].node;
	classic=rtypes[cur_rtype].classic;
	strcpy(curtype, rtypes[cur_rtype].name);
	tenode->setVisible(true);
	updatePending();
};

void Editor::addObjectType(char* name)
{
	//create visuals
	char oname[256];
	char mname[256];
	char fname[256];
	char line[256];
	sprintf(oname,"objectpreview-%s", name);

	bool classic_ref=true;
	sprintf(fname,"%s.odef", name);
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	String group="";
	try
	{
		group = rgm.findGroupContainingResource(fname);
	}catch(...)
	{
	}
	if(group == "")
		return;

	DataStreamPtr ds=rgm.openResource(fname, group);
	//mesh
	ds->readLine(mname, 1023);

	//scale
	ds->readLine(line, 1023);
	while (!ds->eof())
	{
		size_t ll=ds->readLine(line, 1023);
		char* ptline=line;
		if (ll==0 || line[0]=='/') continue;
		//trim line
		while (*ptline==' ' || *ptline=='\t') ptline++;
		if (!strcmp("end",ptline)) break;
		if (!strcmp("standard", ptline)) {classic_ref=false;break;};
	}

	Entity *te = mSceneMgr->createEntity(oname, mname);
	te->setCastShadows(false);
	rtypes[free_rtype].node=mSceneMgr->getRootSceneNode()->createChildSceneNode();
	(rtypes[free_rtype].node)->attachObject(te);
	(rtypes[free_rtype].node)->setVisible(false);
	strcpy(rtypes[free_rtype].name, name);
	rtypes[free_rtype].classic=classic_ref;
	free_rtype++;
}

void Editor::updatePending()
{
	if(free_rtype == 0) return;
	tenode->setPosition(ppos);
	tenode->setOrientation(Quaternion(Degree(pturn), Vector3::UNIT_Y)*Quaternion(Degree(ppitch), Vector3::UNIT_Z));
	if (classic) tenode->pitch(Degree(-90));
}

void Editor::setPos(Vector3 pos)
{
	ppos=pos;
	updatePending();
}

void Editor::toggleType()
{
	if(free_rtype == 0) return;
	Quaternion rot=tenode->getOrientation();
	Vector3 pos=tenode->getPosition();
	cur_rtype++;
	if (cur_rtype==free_rtype) cur_rtype=0;
	tenode->setVisible(false);
	tenode=rtypes[cur_rtype].node;
	classic=rtypes[cur_rtype].classic;
	strcpy(curtype, rtypes[cur_rtype].name);
	tenode->setOrientation(rot);
	tenode->setPosition(pos);
	tenode->setVisible(true);
}

void Editor::dpitch(float v)
{
	ppitch+=v;
	updatePending();
}

void Editor::dturn(float v)
{
	pturn+=v;
	updatePending();
}

Editor::~Editor()
{
}
