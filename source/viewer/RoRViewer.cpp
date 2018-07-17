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

#include "RoRViewer.h"

#include "Settings.h"
#include "errorutils.h"

RoRViewer::RoRViewer(string _meshPath)
{
	active_node = NULL;
	active_mesh.setNull();
	active_entity = NULL;
	ogre_root = NULL;
	scene_mgr = NULL;
	window = NULL;
	meshPath = _meshPath;
}

RoRViewer::~RoRViewer(void)
{
}

bool RoRViewer::Initialize(std::string hwndStr)
{
	if(!SETTINGS.setupPaths())
		return false;

	// load RoR.cfg directly after setting up paths
	SETTINGS.loadSettings(SETTINGS.getSetting("Config Root")+"RoR.cfg");

	String logFilename   = SETTINGS.getSetting("Log Path") + Ogre::String("RoRViewer.log");
	String programPath   = SETTINGS.getSetting("Program Path");
	String pluginsConfig = SETTINGS.getSetting("plugins.cfg");
	
	String ogreConfig    = SETTINGS.getSetting("ogre.cfg");
	ogre_root = new Ogre::Root(pluginsConfig, ogreConfig, logFilename);

	ogre_root->restoreConfig();
	ogre_root->initialise(false);


	Ogre::NameValuePairList param;
	param["externalWindowHandle"] = hwndStr;

	window    = ogre_root->createRenderWindow("viewer", 320, 240, false, &param);
	scene_mgr = ogre_root->createSceneManager(ST_GENERIC);
	camera    = scene_mgr->createCamera("ViewerCam");
	viewport  = window->addViewport(camera);

	camera->setNearClipDistance(0.1);
	camera->setFarClipDistance(1000);
	camera->setFOVy(Ogre::Radian(Ogre::Degree(60)));
	camera->setAutoAspectRatio(true);
	camera->lookAt(0, 0, 0);
	camera->setPosition(Ogre::Vector3(0,12,0));

	mCameraCS = new CCS::CameraControlSystem(scene_mgr, "CameraControlSystem", camera);
	camModeOrbital = new CCS::OrbitalCameraMode(mCameraCS, 1);
	mCameraCS->registerCameraMode("Orbital",camModeOrbital);

	viewport->setBackgroundColour(ColourValue(0.35, 0.35, 0.35));

	timer = new Ogre::Timer();
	timer->reset();


	light = scene_mgr->createLight("Lamp01");
	light->setType(Light::LT_DIRECTIONAL);
	light->setPosition(0, 50, 0);
	light_node = scene_mgr->createSceneNode("RoRViewerLightNode");
	light_node->attachObject(light);
	scene_mgr->getRootSceneNode()->addChild(light_node);
	light_node->lookAt(Vector3::NEGATIVE_UNIT_Y, Node::TS_WORLD, Vector3::UNIT_Z);

	scene_mgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));

	// figure out the files path
	Ogre::String filename, filepath;
	Ogre::StringUtil::splitFilename(meshPath, filename, filepath);

	// add the file path to the resource lookup path
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(filepath, "FileSystem");

	// add default RoR resources, not too bad if this fails
	try
	{
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation(programPath + "resources/OgreCore.zip", "Zip");
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation(programPath + "resources/materials.zip", "Zip");
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation(programPath + "resources/textures.zip", "Zip");
	} catch(...)
	{
	}
	// init all resource locations
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();


	// load the mesh
	String group = Ogre::ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
	active_mesh = Ogre::MeshManager::getSingleton().load(filename, group);
	active_entity = scene_mgr->createEntity(filepath, active_mesh->getName());

	active_node = scene_mgr->getRootSceneNode()->createChildSceneNode();
	active_node->attachObject(active_entity);

	active_node->setPosition(0,0,0);

	mCameraCS->setFixedYawAxis(true);

	mCameraCS->setCameraTarget(active_node);
	mCameraCS->setCurrentCameraMode(camModeOrbital);
	camModeOrbital->setZoom(active_mesh->getBoundingSphereRadius());
	return true;
}

void RoRViewer::Deinitialize(void)
{
	if (!initialized) return;
	ResourceGroupManager* rg_mgr = ResourceGroupManager::getSingletonPtr();

	if(timer) delete timer;
	window->removeViewport(0);
	//scene_mgr->getRootSceneNode()->removeChild(camera_node);
	mCameraCS->deleteAllCameraModes();
	scene_mgr->destroyCamera(camera);
	ogre_root->destroySceneManager(scene_mgr);
	ogre_root->detachRenderTarget("RoRViewer");
	ogre_root->shutdown();

	initialized = false;
}

void RoRViewer::Update()
{
	double tm = timer->getMilliseconds();
	timer->reset();
	ogre_root->renderOneFrame(tm);
	mCameraCS->update(tm);

}

void RoRViewer::TurnCamera(Vector3 speed)
{
	camModeOrbital->yaw(-speed.x);
	camModeOrbital->pitch(-speed.y);
	camModeOrbital->zoom(-speed.z);
}



void RoRViewer::MoveCamera(Vector3 speed)
{
	// TODO
}