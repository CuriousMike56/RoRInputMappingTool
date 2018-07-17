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
#ifndef __RoRFrameListener_H__
#define __RoRFrameListener_H__

#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif
//#include "OgreKeyEvent.h"
//#include "OgreEventListeners.h"
//#include "OISEvents.h"
//#include "OISInputManager.h"
//#include "OISMouse.h"
//#include "OISKeyboard.h"
//#include "OISJoyStick.h"

#ifdef USE_MPLATFORM
#include "mplatform_base.h"
#endif

#include "OgreStringConverter.h"
#include "OgreException.h"
//#include "OgreTerrainSceneManager.h"
#include "OgrePixelFormat.h"
#include "CollisionTools.h"
#include <string.h>
#include <stdio.h>
#include "SkyManager.h"
#include "Beam.h"
#include "InputEngine.h"
#include "turbojet.h"
#include "Character.h"
#include "CacheSystem.h"
#include "ChatSystem.h"
#include "envmap.h"
#include "ForceFeedback.h"

#ifdef USE_PAGED
# include "PagedGeometry.h"
# include "GrassLoader.h"
# include "TreeLoader2D.h"
#endif

#include "OgreTerrainGroup.h"

#ifdef USE_LUA
class LuaSystem;
#endif

class Road;
class ProceduralManager;
class Editor;
class Water;
class HeatHaze;
class Dashboard;
class DotSceneLoader;
#ifdef HAS_EDITOR
class TruckEditor;
#endif
class Network;
//class BeamJoystick;
class Collisions;
class Mirrors;
class Beam;
class MapControl;
class HDRListener;

//#include "dirt.h"


#define CAMERA_EXT 0
#define CAMERA_FIX 1
#define CAMERA_INT 2
#define CAMERA_END 3
// the free modes are on purpose behind end, so they require a special key to be triggered
#define CAMERA_FREE 4
#define CAMERA_FREE_FIXED 5

#define CAMERA_EXTERNALCONTROL 9999


#define NONE_LOADED 0
#define TERRAIN_LOADED 1
#define ALL_LOADED 2
#define EXITING 3
#define EDITING 4
#define RELOADING 5
#define EDITOR_PAUSE 6

#define EVENT_NONE 0
#define EVENT_ALL 1
#define EVENT_AVATAR 2
#define EVENT_TRUCK 3
#define EVENT_AIRPLANE 4
#define EVENT_DELETE 5

#define DEFAULT_INTERNAL_CAM_PITCH Degree(-15)

#define MAX_PLAYLIST_ENTRIES 20

// this defines the version that is used for the scripts and console stuff.
// $LastChangedDate: 2010-05-01 09:16:13 +0200 (Sat, 01 May 2010) $
// $LastChangedRevision: 1355 $
// $LastChangedBy: rorthomas $
// $Id: RoRFrameListener.h 1355 2010-05-01 07:16:13Z rorthomas $
// $Rev: 1355 $
#define SVN_REVISION "$Rev: 1355 $"
#define SVN_ID "$Id: RoRFrameListener.h 1355 2010-05-01 07:16:13Z rorthomas $"
#define ROR_VERSION_STRING "0.37"

//using namespace Ogre;

#ifdef USE_PAGED
using namespace Forests;
#endif

#define LOCALIZER_VERTICAL 0
#define LOCALIZER_HORIZONTAL 1
#define LOCALIZER_NDB 2
#define LOCALIZER_VOR 3

enum QueryFlags
{
   OBJECTS_MASK = 1<<7,
   TRUCKS_MASK  = 1<<8,
};

typedef struct
{
	int type;
	Vector3 position;
	Quaternion rotation;
} localizer_t;

typedef struct
{
	float px;
	float py;
	float pz;
//	float ry;
	Quaternion rotation;
	char name[256];
	bool ismachine;
	bool freePosition;
} truck_prepare_t;

typedef struct
{
	Vector3 pos;
	Quaternion rot;
} spawn_location_t;

#ifdef USE_PAGED
typedef struct
{
	PagedGeometry *geom;
	void *loader;
} paged_geometry_t;
#endif //PAGED

typedef struct
{
	Ogre::Entity *ent;
	Ogre::SceneNode *node;
	Ogre::AnimationState *anim;
	float speedfactor;
} animated_object_t;

class RoRFrameListener;
class DOFManager;

class SelectorWindow;

extern RoRFrameListener *eflsingleton;


class RoRFrameListener: public FrameListener, public Ogre::WindowEventListener
{
	friend class BeamFactory;
protected:
	void initializeCompontents();
	OverlayWrapper *ow;
	int setupBenchmark();
	void benchStep(float dt);
	truck_prepare_t truck_preload[100];
	int truck_preload_num;
	localizer_t localizers[64];
	int free_localizer;
	int mSceneDetailIndex;
	Real mMoveSpeed;
	bool benchmarking;
	//    Real dirSpeed;
	Degree mRotateSpeed;

	MapControl *bigMap;
	StaticGeometry *bakesg;
	int interactivemap;
    bool xfire_enabled;
	int externalCameraMode;
	int netPointToUID;

	//    Beam *beam;
	int loading_state;


	std::map< std::string, Ogre::SceneNode *> loadedObjects;
	DOFManager *mDOF;
	bool mDOFDebug;
	int mouseX;
	int mouseY;
	int shaderSchemeMode;
	int inputGrabMode;
	int mouseGrabState;
	bool switchMouseButtons;
	int screenWidth;
	int screenHeight;
	bool isnodegrabbed;
	int truckgrabbed;
	int nodegrabbed;
	Real distgrabbed;
	bool enablePosStor;

	//GUI_Progress *gui_progress;
	TerrainGroup* mTerrainGroup;

	std::map<String, spawn_location_t> netSpawnPos;
	float truckx, trucky, truckz;
	//	SceneNode *speed_node;
	//	SceneNode *tach_node;
	//	SceneNode *roll_node;
	//	SceneNode *pitch_node;
	//	SceneNode *rollcorr_node;


	SceneManager *mSceneMgr;
	Root *mRoot;
	//	bool usejoy;
	//bool useforce;
	//BeamJoystick *joy;
	ForceFeedback *forcefeedback;
#ifdef USE_OIS_G27
	OIS::Win32LogitechLEDs* leds;
#endif //OIS_G27
	bool flipflop;
	HDRListener *hdrListener;

	bool dirvisible;
	SceneNode *dirArrowNode;
	Vector3 dirArrowPointed;
	int objectCounter;
	float mouseGrabForce;

	SceneNode *pointerDestination;

	//SceneNode *personode;
	//AnimationState *persoanim;
	//float persoangle;
	//float persospeed;
	//float persovspeed;
	//bool perso_canjump;
	//Vector3 lastpersopos;

	void updateStats(void);
	//update engine panel
	void updateGUI(float dt);
	void updateIO(float dt);

#ifdef USE_PAGED
	std::vector<paged_geometry_t> pagedGeometry;
#endif
	String grassdensityTextureFilename;
	void setGrassDensity(float x, float y, int density, bool relative=false);
	void saveGrassDensity();
	int changeGrassBuffer(unsigned char *data, int relchange);
	void updateGrass(Vector3 pos);
	void initSoftShadows();
	void initSSAO();
	void initHDR();

	// WindowEventListener
	void windowMoved(Ogre::RenderWindow* rw);
	void windowResized(Ogre::RenderWindow* rw);
	void windowClosed(Ogre::RenderWindow* rw);
	void windowFocusChange(Ogre::RenderWindow* rw);
	int flaresMode;
	MapTextureCreator *mtc;
	int fogmode;
	float fogdensity;
	ManualObject *pickLine;
	SceneNode *pickLineNode;
	bool updateTruckMirrors(float dt);
	void gridScreenshots(Ogre::RenderWindow* pRenderWindow, Ogre::Camera* pCamera, const int& pGridSize, const Ogre::String& path, const Ogre::String& pFileName, const Ogre::String& pFileExtention, const bool& pStitchGridImages);
	void initDust();
public:
	// Constructor takes a RenderWindow because it uses that to determine input context
	RoRFrameListener(RenderWindow* win, Camera* cam, SceneManager* scm, Root* root, bool isEmbedded=false);
	virtual ~RoRFrameListener();

	void removeBeam(Beam *);
	void removeTruck(int truck);
	void loadObject(const char* name, float px, float py, float pz, float rx, float ry, float rz, SceneNode * bakeNode, const char* instancename, bool enable_collisions=true, int luahandler=-1, const char *type=0, bool uniquifyMaterial=false);
	void unloadObject(const char* name);
	void repairTruck(char* inst, char* box);
	void removeTruck(char* inst, char* box);
	bool updateEvents(float dt);
	bool benchmarkStep(float dt);
	void initTrucks(bool loadmanual, Ogre::String selected, Ogre::String selectedExtension = Ogre::String(), std::vector<Ogre::String> *truckconfig=0, bool enterTruck=false);
	void setCurrentTruck(int v);
	//bool processUnbufferedMouseInput(const FrameEvent& evt);
	void moveCamera(float dt);
	void hideGUI(bool visible);
	Ogre::String saveTerrainMesh();
	//void showBigMap(bool show);
	// Override frameStarted event to process that (don't care about frameEnded)
	bool frameStarted(const FrameEvent& evt);
	void recursiveActivation(int j);
	bool setCameraPositionWithCollision(Ogre::Vector3 newPos);
	bool checkForActive(int j, bool *sleepyList);
	int getFogMode() { return fogmode; };
	float getFogDensity() { return fogdensity; };
	bool frameEnded(const FrameEvent& evt);
	void showLoad(int type, char* instance, char* box);
	void getMeshInformation(Mesh* mesh,size_t &vertex_count,Vector3* &vertices,
			size_t &index_count, unsigned* &indices,
			const Vector3 &position = Vector3::ZERO,
			const Quaternion &orient = Quaternion::IDENTITY,const Vector3 &scale = Vector3::UNIT_SCALE);
	void showspray(bool s);
	int getNetPointToUID() { return netPointToUID; };
	void setNetPointToUID(int uid);
	void checkRemoteStreamResultsChanged();
	float netcheckGUITimer;

	Character *person;
	ChatSystem *netChat;
	bool getSwitchButtons() { return switchMouseButtons; };
	static HeightFinder *hfinder;
	double getTime() {return rtime;};
	SceneManager *getSceneMgr() { return mSceneMgr; };
	RenderWindow *getRenderWindow() { return mWindow; };
	Collisions *getCollisions() { return collisions; };
	int addTruck(char* fname, Vector3 pos);
	Beam *getTruck(int number) { return trucks[number]; };
	int getCurrentTruckNumber() { return current_truck; };
	int getTruckCount() { return free_truck; };
	Beam *getCurrentTruck() { return (current_truck<0)?0:trucks[current_truck]; };
	Water *getWater() { return w; };
	Camera *getCamera() { return mCamera; };
	//SceneNode *getPersonNode() { return personode; };
	//OIS::Mouse *getMouse() { return  mMouse; };
	int getLoadingState() { return loading_state; };
	void setLoadingState(int value);
	void pauseSim(bool value);
	void loadNetTerrain(char *preselected_map);
	void exploreScripts();
	float mapsizex, mapsizez;

	float stopTimer();
	void startTimer();
	void updateRacingGUI();

	int cameramode, lastcameramode;
	void setCameraRotation(Ogre::Radian x, Ogre::Radian y, Ogre::Real distance) { camRotX=x; camRotY=y; camDist=distance;};
	void hideMap();
	void setDirectionArrow(char *text, Vector3 position);
	Ogre::Radian getCameraRotationX() { return camRotX; };
	void netDisconnectTruck(int number);

	static float getGravity() { return gravity; };
	static void setGravity(float value);

	Ogre::Ray getMouseRay();
	void shutdown_pre();
	void shutdown_final();

	void loadTerrain(Ogre::String terrainfile);
	void loadClassicTerrain(Ogre::String terrainfile);
	void loadOgitorTerrain(Ogre::String terrainfile);

	OverlayWrapper *getOverlayWrapper() { return ow; };

	Water *w;

public:
	// mutex'ed data
	void setNetQuality(int q);
	int getNetQuality(bool ack=false);
	bool getNetQualityChanged();
	pthread_mutex_t mutex_data;
private:
	int net_quality; 
	bool net_quality_changed; 


protected:

	HeatHaze *heathaze;
	bool chatting;
	bool chatlock;
	double rtime;
	float terrainxsize;
	float terrainzsize;

	DotSceneLoader* mLoader;



	//OIS::InputManager* mInputManager;
	//OIS::Mouse* mMouse;
	Camera* mCamera;
	Vector3 cdoppler;
	Vector3 reload_pos;
	collision_box_t *reload_box;
	Quaternion reload_dir;

	RenderWindow* mWindow;
	int mStatsOn;
	bool mTruckInfoOn;
	int mapMode;
	unsigned int mNumScreenShots;
	// just to stop toggles flipping too fast
	Real mTimeUntilNextToggle ;
	bool pressure_pressed;
	SoundScriptManager* ssm;

	//camera
	Vector3 camIdealPosition;
	Radian camRotX, camRotY;
	bool camCollided;
	Vector3 camPosColl;
	Radian pushcamRotX, pushcamRotY;
	Real camDist;
	float mMoveScale;
	Degree mRotScale;
	Vector3 lastPosition;
	//	float lastangle;
	float clutch;
	Editor *editor;
	int showcredits;
	float creditsviewtime;
	static float gravity;

	bool netmode;
	bool initialized;
	bool isEmbedded;

	bool hidegui;
	bool debugCollisions;

#ifdef USE_LUA
	LuaSystem *lua;
#endif

	Collisions *collisions;
	MOC::CollisionTools* mCollisionTools;

	int raceStartTime;

	int objcounter;
	char terrainmap[256];
	Cache_Entry loaded_terrain;

	Ogre::String terrainUID;
	Road *road;
	Mirrors *mirror;
	Dashboard *dashboard;
	FILE *editorfd;
	//	Dirt *dirt;
#ifdef HAS_EDITOR
	TruckEditor *trucked;
#endif
	Beam *trucks[MAX_TRUCKS];
	int thread_mode;

	/** adds a truck to the main array
	  */
	int addTruck(Beam *b);
	int getFreeTruckSlot();
	int free_truck;
	int current_truck;

	Vector3 persostart;
	int joyshiftlock;

	Envmap *envmap;

	Network *net;
	ProceduralManager *proceduralManager;


	void updateXFire();
	int gameStartTime;

#ifdef USE_PAGED
	Forests::TreeLoader2D *treeLoader;
#endif

//	char *preselected_map;
//	char *preselected_truck;

#ifdef USE_MPLATFORM
	MPlatform_Base *mplatform;
#endif

	static bool fileExists(const char* filename);
	void processConsoleInput();

	RenderWindow* renderwin;

	char screenshotformat[255];
	float globalFogDensity;
	bool useCaelumSky;
	float farclip;
	ColourValue fadeColour;

//	GUI_TruckTool *truckToolGUI;
//	GUI_MainMenu *mainmenu;

	Ogre::String loadedTerrain;

	std::vector<animated_object_t> animatedObjects;
	bool updateAnimatedObjects(float dt);
};


#endif
