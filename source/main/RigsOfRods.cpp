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
#include "RigsOfRods.h"
#include "OgreLineStreamOverlayElement.h"
#include "language.h"
#include "errorutils.h"

#include <OgreHeaderPrefix.h>
#include <OgreRTShaderSystem.h>

/** This class simply demonstrates basic usage of the CRTShader system.
It sub class the material manager listener class and when a target scheme callback
is invoked with the shader generator scheme it tries to create an equvialent shader
based technique based on the default technique of the given material.
*/
class ShaderGeneratorTechniqueResolverListener : public MaterialManager::Listener
{
public:

	ShaderGeneratorTechniqueResolverListener(RTShader::ShaderGenerator* pShaderGenerator)
	{
		mShaderGenerator = pShaderGenerator;
	}

	virtual Technique* handleSchemeNotFound(unsigned short schemeIndex, 
		const String& schemeName, Material* originalMaterial, unsigned short lodIndex, 
		const Renderable* rend)
	{		
		// Case this is the default shader generator scheme.
		if (schemeName == RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME)
		{
			MaterialRegisterIterator itFind = mRegisteredMaterials.find(originalMaterial);
			bool techniqueCreated = false;

			// This material was not registered before.
			if (itFind == mRegisteredMaterials.end())
			{
				techniqueCreated = mShaderGenerator->createShaderBasedTechnique(
					originalMaterial->getName(), 
					MaterialManager::DEFAULT_SCHEME_NAME, 
					schemeName);				
			}
			mRegisteredMaterials[originalMaterial] = techniqueCreated;
		}

		return NULL;
	}

protected:
	typedef std::map<Material*, bool>		MaterialRegisterMap;
	typedef MaterialRegisterMap::iterator	MaterialRegisterIterator;


protected:
	MaterialRegisterMap				mRegisteredMaterials;		// Registered material map.
	RTShader::ShaderGenerator*		mShaderGenerator;			// The shader generator instance.
};


RigsOfRods::RigsOfRods(Ogre::String name, unsigned int hwnd) :
	mRoot(0),
	mCamera(0),
	mSceneMgr(0),
	mFrameListener(0),
	mWindow(0),
	ssm(0),
	hwnd(hwnd),
	name(name)
{
	useogreconfig=false;
}

RigsOfRods::~RigsOfRods()
{
	// this is the end of all and everything
	if (mFrameListener)
		delete mFrameListener;
	if (mRoot)
	{
		if (mWindow)
			mRoot->detachRenderTarget(mWindow);
		try
		{
			mRoot->shutdown();
			// XXX : commented out code below crashes
			// 
			//if(mRoot)
			//	delete mRoot;
		} catch(Ogre::Exception& e)
		{
			LogManager::getSingleton().logMessage("error while closing RoR: " + e.getFullDescription());
		}
	}
}

void RigsOfRods::go(void)
{
	if (!setup())
		return;

	mRoot->startRendering();

	// clean up
	//destroyScene(); we don't!
}

void RigsOfRods::loadMainResource(String name, String group)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Resources Path")+name+".zip", "Zip", group, true);
}

bool RigsOfRods::setup(void)
{
	//try to get correct paths
	//note: we don't have LogManager available yet!
	//FIRST: Get the "program path" and the user space path

	// note: this is now done in the settigns class, so set it up
	// note: you need to set the buildmode correcty before you build the paths!
	SETTINGS.setSetting("BuildMode", buildmode?"Yes":"No");
	if(!SETTINGS.setupPaths())
		return false;

	// load RoR.cfg directly after setting up paths
	SETTINGS.loadSettings(SETTINGS.getSetting("Config Root")+"RoR.cfg");

	//CREATE OGRE ROOT
	String logFilename   = SETTINGS.getSetting("Log Path") + name + Ogre::String(".log");
	String pluginsConfig = SETTINGS.getSetting("plugins.cfg");
	String ogreConfig    = SETTINGS.getSetting("ogre.cfg");
	mRoot = new Root(pluginsConfig, ogreConfig, logFilename);

	// log verbosity change
	if(SETTINGS.getSetting("Logging Level") == "verbose")
		LogManager::getSingleton().setLogDetail(LL_BOREME);
	else if(SETTINGS.getSetting("Logging Level") == "normal")
		LogManager::getSingleton().setLogDetail(LL_NORMAL);
	else if(SETTINGS.getSetting("Logging Level") == "low")
		LogManager::getSingleton().setLogDetail(LL_LOW);
	//FROM NOW ON WE HAVE LOGMANAGER!

	CACHE.setLocation(SETTINGS.getSetting("Cache Path"), SETTINGS.getSetting("Config Root"));

	ColoredTextAreaOverlayElementFactory *cef = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(cef);

	// load factory to be able to create stream lines
	OverlayManager& overlayManager = OverlayManager::getSingleton();
	overlayManager.addOverlayElementFactory(new LineStreamOverlayElementFactory());

#ifdef HAS_EDITOR
	spinfact=new SpinControlOverlayElementFactory();
	Ogre::OverlayManager::getSingleton().addOverlayElementFactory(spinfact);
#endif

	//load bootstrap and main resources
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	//bootstrap
	LogManager::getSingleton().logMessage("Loading Bootstrap");
	loadMainResource("OgreCore", "Bootstrap");
	//main game resources
	LogManager::getSingleton().logMessage("Loading main resources");
	loadMainResource("airfoils");
	loadMainResource("materials");
	loadMainResource("meshes");
	loadMainResource("overlays");
	loadMainResource("particles");
	loadMainResource("mygui");
	loadMainResource("layouts");
	loadMainResource("scripts");
	loadMainResource("textures");
	loadMainResource("flags");
	loadMainResource("icons");

	// optional ones
	if (SETTINGS.getSetting("3D Sound renderer") != "No sound")
		loadMainResource("sounds");
	
	if (SETTINGS.getSetting("Sky effects") == "Caelum (best looking, slower)")
		loadMainResource("caelum");

	if(SETTINGS.getSetting("Hydrax") == "Yes")
		loadMainResource("hydrax", "Hydrax"); // special resourcegroup required!

	if(SETTINGS.getSetting("Vegetation") != "None (fastest)")
		loadMainResource("paged");

	if(SETTINGS.getSetting("HDR") == "Yes")
		loadMainResource("hdr");

	if(SETTINGS.getSetting("DOF") == "Yes")
		loadMainResource("dof");

	if(SETTINGS.getSetting("Glow") == "Yes")
		loadMainResource("glow");

	if(SETTINGS.getSetting("Motion blur") == "Yes")
		loadMainResource("blur");
	
	if(SETTINGS.getSetting("HeatHaze") == "Yes")
		loadMainResource("heathaze");

	if (SETTINGS.getSetting("Sunburn")!="Yes")
		loadMainResource("sunburn");

	if (SETTINGS.getSetting("Shadow technique")=="Parallel-split Shadow Maps")
		loadMainResource("pssm");
	
	//streams path, to be processed later by the cache system
	LogManager::getSingleton().logMessage("Loading filesystems");

	//main synced streams
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Streams Path"), "FileSystem", "Streams");
	exploreStreams(); //this will explore subdirs and register them as Packs dirs
	//cache, flat
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"cache", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	//config, flat
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"config", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	//packs, to be processed later by the cache system
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"packs", "FileSystem", "Packs", true);
	//user vehicles, to be processed later by the cache system
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"vehicles", "FileSystem", "VehicleFolders");
	exploreVehicles(); //this will add subdirs contents into General
	//user terrains, to be processed later by the cache system
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"terrains", "FileSystem", "TerrainFolders");
	exploreTerrains(); //this will add subdirs contents into General

	// add scripts folder
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("User Path")+"scripts", "FileSystem", "Scripts", true);


	// init skin manager, important to happen before trucks resource loading!
	LogManager::getSingleton().logMessage("registering Skin Manager");
	new SkinManager();

	LogManager::getSingleton().logMessage("registering colored text overlay factory");
	ColoredTextAreaOverlayElementFactory *pCT = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(pCT);

	LogManager::getSingleton().logMessage("Configuring");
	bool carryOn = configure();
	if (!carryOn) return false;

	LogManager::getSingleton().logMessage("Creating Sound Manager");
	//get instance of SoundScriptManager
	//this will create the singleton and register the parser!
#ifdef USE_OPENAL
	ssm=SoundScriptManager::getSingleton();
#endif //OPENAL

	//CREATE SCENE MANAGER
	LogManager::getSingleton().logMessage("Creating Scene Manager");
	if(SETTINGS.getSetting("new Terrain Mode") == "Yes")
	{
		mSceneMgr = mRoot->createSceneManager(ST_GENERIC);
	} else
	{
		mSceneMgr = mRoot->createSceneManager(ST_EXTERIOR_CLOSE);
	}

	// create RTShader System
	initRTShaderSystem();

	//CREATE CAMERA
	LogManager::getSingleton().logMessage("Creating camera");
	// Create the camera
	mCamera = mSceneMgr->createCamera("PlayerCam");
	// Position it at 500 in Z direction
	mCamera->setPosition(Vector3(128,25,128));
	// Look back along -Z
	mCamera->lookAt(Vector3(0,0,-300));
	mCamera->setNearClipDistance( 0.5 );
	mCamera->setFarClipDistance( 1000.0*1.733 );
	mCamera->setFOVy(Degree(60));

	//CREATE VIEWPORT
	LogManager::getSingleton().logMessage("Creating Viewport");
	// Create one viewport, entire window
	vp = mWindow->addViewport(mCamera);
	vp->setBackgroundColour(ColourValue(0,0,0));


	// Set default mipmap level (NB some APIs ignore this)
	TextureManager::getSingleton().setDefaultNumMipmaps(5);
	//mRoot->setFrameSmoothingPeriod(2.0);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifndef _UNICODE
	if(hwnd == 0)
	{
		// only in non-embedded mode
		size_t hWnd = 0;
		mWindow->getCustomAttribute("WINDOW", &hWnd);

		char buf[MAX_PATH];
		::GetModuleFileNameA(0, (LPCH)&buf, MAX_PATH);

		HINSTANCE instance = ::GetModuleHandleA(buf);
		HICON hIcon = ::LoadIconA(instance, MAKEINTRESOURCE(1001));
		if (hIcon)
		{
			::SendMessageA((HWND)hWnd, WM_SETICON, 1, (LPARAM)hIcon);
			::SendMessageA((HWND)hWnd, WM_SETICON, 0, (LPARAM)hIcon);
		}
	}
#endif //_UNICODE
#endif //OGRE_PLATFORM_WIN32

	// load all resources now, so the zip files are also initiated
	LogManager::getSingleton().logMessage("initialiseAllResourceGroups()");
	try
	{
		ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	} catch(Ogre::Exception& e)
	{
		LogManager::getSingleton().logMessage("catched error while initializing Resource groups: " + e.getFullDescription());
	}

	//rgm.initialiseResourceGroup(ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

#ifndef NOLANG
	// load language, must happen after initializing Settings class and Ogre Root!
	// also it must happen after loading all basic resources!
	LanguageEngine::Instance().setup();
#endif //NOLANG

	// Create the scene

	LogManager::getSingleton().logMessage("createScene()");
	createScene();

	LogManager::getSingleton().logMessage("Adding Frame Listener");
	bool isEmbedded = (hwnd != 0);
	mFrameListener = new RoRFrameListener(mWindow, mCamera, mSceneMgr, mRoot, isEmbedded);
	mRoot->addFrameListener(mFrameListener);

	LogManager::getSingleton().logMessage("Setup finished successfully");
	return true;
}

void RigsOfRods::initRTShaderSystem()
{
	if(!Ogre::RTShader::ShaderGenerator::initialize())
		return;

	// Grab the shader generator pointer.
	Ogre::RTShader::ShaderGenerator *mShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();

	// Set the scene manager.
	mShaderGenerator->addSceneManager(mSceneMgr);


	// Add the shader libs resource location.
	loadMainResource("rtshader");

	// Set shader cache path.
	mShaderGenerator->setShaderCachePath(SETTINGS.getSetting("User Path")+"cache");

	ShaderGeneratorTechniqueResolverListener *mMaterialMgrListener = new ShaderGeneratorTechniqueResolverListener(mShaderGenerator);				
	MaterialManager::getSingleton().addListener(mMaterialMgrListener);

	// add per pixel lighting
	/*
	RTShader::ShaderGenerator* shaderGenerator = RTShader::ShaderGenerator::getSingletonPtr();
	RTShader::RenderState* renderState = shaderGenerator->getRenderState(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
	renderState->reset();
	RTShader::SubRenderState* perPixelLightModel = shaderGenerator->createSubRenderState(RTShader::PerPixelLighting::Type);
	renderState->addTemplateSubRenderState(perPixelLightModel);
	// Invalidate the scheme in order to re-generate all shaders based technique related to this scheme.
	shaderGenerator->invalidateScheme(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
	*/
}

void RigsOfRods::createScene(void)
{
	String tft=SETTINGS.getSetting("Texture Filtering");
	TextureFilterOptions tfo=TFO_NONE;
	if (tft=="Bilinear") tfo=TFO_BILINEAR;
	if (tft=="Trilinear") tfo=TFO_TRILINEAR;
	if (tft=="Anisotropic (best looking)") tfo=TFO_ANISOTROPIC;

	// Set ambient light
	//        mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));
	mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));
	//lighten shadows
	//		mSceneMgr->setShadowColour(ColourValue(0.5, 0.5, 0.5));

	// Accept default settings: point light, white diffuse, just set position
	// NB I could attach the light to a SceneNode if I wanted it to move automatically with
	//  other objects, but I don't
	//        l->setPosition(-20000,80000,-50000);

	MaterialManager::getSingleton().setDefaultAnisotropy(8);
	MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);

}

bool RigsOfRods::configure(void)
{
	// Show the configuration dialog and initialise the system
	// You can skip this and use root.restoreConfig() to load configuration
	// settings if you were sure there are valid ones saved in ogre.cfg
	if(hwnd == 0)
	{
		//default mode
		bool ok = false;
		if(useogreconfig)
			ok = mRoot->showConfigDialog();
		else
			ok = mRoot->restoreConfig();
		if(ok)
		{
			// If returned true, user clicked OK so initialise
			// Here we choose to let the system create a default rendering window by passing 'true'
			mWindow = mRoot->initialise(true);
			return true;
		}
		else
		{
			showError(_L("Configuration error"), _L("Run the RoRconfig program first."));
			return false;
		}
	} else
	{
		// embedded mode
		if(!mRoot->restoreConfig())
		{
			showError(_L("Configuration error"), _L("Run the RoRconfig program first."));
			return false;
		}

		mRoot->initialise(false);

		Ogre::NameValuePairList param;
		param["externalWindowHandle"] = Ogre::StringConverter::toString(hwnd);
		mWindow = mRoot->createRenderWindow(name, 320, 240, false, &param);
		return true;
	}
	return false;
}

void RigsOfRods::exploreStreams()
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	//explore stream folders, add their content as packs
	FileInfoListPtr files= rgm.findResourceFileInfo("Streams", "*", true); //searching for dirs
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if (iterFiles->filename==String(".svn")) continue;
		String filename = SETTINGS.getSetting("Streams Path") + (*iterFiles).filename;
		rgm.addResourceLocation(filename, "FileSystem", "Packs");

		// HACK: add subfolders
		rgm.addResourceLocation(filename+dirsep+"vehicles", "FileSystem", "VehicleFolders");
		rgm.addResourceLocation(filename+dirsep+"terrains", "FileSystem", "TerrainFolders");
	}
}

void RigsOfRods::exploreVehicles()
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	//explore vehicles folders, add their content as flat
	FileInfoListPtr files= rgm.findResourceFileInfo("VehicleFolders", "*", true); //searching for dirs
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if ((*iterFiles).filename==String(".svn")) continue;
		//trying to get the full path
		String fullpath=(*iterFiles).archive->getName()+dirsep;
		rgm.addResourceLocation(fullpath+(*iterFiles).filename, "FileSystem", "VehicleFolders");
		//add textures/temp for unpacked ddx support
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"textures"+dirsep+"temp", "FileSystem", "VehicleFolders");
	}
}

void RigsOfRods::exploreTerrains()
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	//explore vehicles folders, add their content as flat
	FileInfoListPtr files= rgm.findResourceFileInfo("TerrainFolders", "*", true); //searching for dirs
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if ((*iterFiles).filename==String(".svn")) continue;
		//trying to get the full path
		String fullpath=(*iterFiles).archive->getName()+dirsep;
		rgm.addResourceLocation(fullpath+(*iterFiles).filename, "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"textures"+dirsep+"temp", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"objects", "FileSystem", "TerrainFolders");
		// ogitor support below
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Caelum", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Hydrax", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Materials", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Models", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Scripts", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"SkyX", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Temp", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Templates", "FileSystem", "TerrainFolders");
		rgm.addResourceLocation(fullpath+(*iterFiles).filename+dirsep+"Terrain", "FileSystem", "TerrainFolders");
		
	}
}
