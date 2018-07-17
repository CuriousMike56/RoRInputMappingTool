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
#include "envmap.h"
#include "Beam.h"
/*
Camera *mloCamera;

class MirrorsListener : public RenderTargetListener
{
public:
void preRenderTargetUpdate(const RenderTargetEvent& evt)
{
//mloCamera->setFOVy(mloCamera->getFOVy()/2.0);
}
void postRenderTargetUpdate(const RenderTargetEvent& evt)
{
//mloCamera->setFOVy(mloCamera->getFOVy()*2.0);
}

};

MirrorsListener mMirrorsListener;
*/
Envmap::Envmap(SceneManager *mSceneMgr, RenderWindow *mWindow, Camera *incam, bool dynamic)
{
	isDynamic=dynamic;
	inited=false;
	round=0;
	texture = TextureManager::getSingleton().createManual("EnvironmentTexture",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_CUBE_MAP, 256, 256, 0,
		PF_R8G8B8, TU_RENDERTARGET);

	for (int face = 0; face < 6; face++)
	{
		rt[face] = texture->getBuffer(face)->getRenderTarget();
		// rt->addListener(this);

		char name[256];
		sprintf(name, "EnvironmentCamera-%i\n", face);
		camera[face] = mSceneMgr->createCamera(name);
		camera[face]->setAspectRatio(1.0);
		camera[face]->setProjectionType(PT_PERSPECTIVE);
		camera[face]->setFOVy(Degree(90));
		camera[face]->setNearClipDistance(0.1); //why this does not work, I wonder.
		camera[face]->setFarClipDistance(incam->getFarClipDistance());
		//ent->getParentSceneNode()->attachObject(camera[face]);

		Viewport *v = rt[face]->addViewport(camera[face]);
		v->setOverlaysEnabled(false);
		v->setClearEveryFrame(true);
		v->setBackgroundColour(incam->getViewport()->getBackgroundColour());
		rt[face]->setAutoUpdated(false);

		switch (face)
		{
		case 0:
			camera[face]->setDirection(-1, 0, 0);   // <-- should be +X
			break;
		case 1:
			camera[face]->setDirection(1, 0, 0);   // <-- should be -X
			break;
		case 2:
			camera[face]->setDirection(0, 0, 1);
			camera[face]->pitch(Degree(90));
			break;
		case 3:
			camera[face]->setDirection(0, 0, 1);
			camera[face]->pitch(Degree(-90));
			break;
		case 4:
			camera[face]->setDirection(0, 0, 1);
			break;
		case 5:
			camera[face]->setDirection(0, 0, -1);
			break;
		}
	} 



}

void Envmap::update(Vector3 center, Beam *beam)
{
	if (!isDynamic)
	{
		if (inited) return;
		else
		{
			//capture all images at once
			for (int i=0; i<6; i++) 
			{
				camera[i]->setPosition(center); 
				rt[i]->update();
			}
			inited=true;
		}
	}
	for (int i=0; i<6; i++) camera[i]->setPosition(center); 
	
	// try to hide all flexbodies and cabs prior render, and then show them again after done
	// but only if they are visible ...
	bool togglemeshes = (beam)?(beam->meshesVisible):false;
	if(beam && togglemeshes)
		beam->setMeshVisibility(false);
	rt[round]->update();
	if(beam && togglemeshes)
		beam->setMeshVisibility(true);
	round++;
	if (round>=6) round=0;
}

void Envmap::forceUpdate(Vector3 center)
{
	if (isDynamic) return;
	//capture all images at once
	for (int i=0; i<6; i++) 
	{
		camera[i]->setPosition(center); 
		rt[i]->update();
	}
	inited=true;
}

void Envmap::prepareShutdown()
{
	//if (rttTex) rttTex->removeListener(&mMirrorsListener);
}

