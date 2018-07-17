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

#ifndef RORVIEWER_H__
#define RORVIEWER_H__
#include <string>
#include <Ogre.h>

#include <CCSCameraControlSystem.h>
#include <CCSOrbitalCameraMode.h>

using namespace std;
using namespace Ogre;

class RoRViewer
{
public:
	RoRViewer(string meshPath);
	~RoRViewer(void);

	bool Initialize(std::string hwndStr);
	void Deinitialize(void);
	void Update(void);
	
	RenderWindow* GetOgreWindow(void){return window;}
	SceneManager* GetSceneManager(void){return scene_mgr;}
	Viewport* GetViewport(void){return viewport;}
	Camera* GetCamera(void){return camera;}
	MeshPtr GetMesh(void){return active_mesh;}
	Entity* GetEntity(void){return active_entity;}
	SceneNode* GetSceneNode(void){return active_node;}

	void TurnCamera(Vector3 speed);
	void MoveCamera(Vector3 speed);

private:
	bool			initialized;
	Root*			ogre_root;
	SceneManager*	scene_mgr;
	RenderWindow*	window;
	Camera*			camera;
	SceneNode*		camera_node;
	Viewport*		viewport;

	MeshPtr			active_mesh;
	Entity*			active_entity;
	SceneNode*		active_node;

	SceneNode*		light_node;
	Light*			light;

	Ogre::Timer*	timer;

	CCS::CameraControlSystem *mCameraCS;
	CCS::OrbitalCameraMode *camModeOrbital;
	
	string          meshPath;
};

#endif //RORVIEWER_H__