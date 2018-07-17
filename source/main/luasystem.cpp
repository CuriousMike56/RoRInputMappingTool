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
#ifdef USE_LUA

#include "luasystem.h"
#include "Settings.h"
#include "OverlayWrapper.h"

#ifdef USE_MYGUI
#include "SelectorWindow.h"
#endif //USE_MYGUI

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

LuaSystem *luaInstance=0;


static int SLUApanic (lua_State *lua) {return luaInstance->panic(lua);}
static int SLUAflashMessage (lua_State *lua) {return luaInstance->flashMessage(lua);}
static int SLUAlog (lua_State *lua) {return luaInstance->log(lua);}
static int SLUAshowChooser (lua_State *lua) {return luaInstance->showChooser(lua);}
static int SLUArepairVehicle (lua_State *lua) {return luaInstance->repairVehicle(lua);}
static int SLUAremoveVehicle (lua_State *lua) {return luaInstance->removeVehicle(lua);}
static int SLUAgetTime (lua_State *lua) {return luaInstance->getTime(lua);}
static int SLUAregisterCallBack(lua_State *lua) {return luaInstance->registerCallBack(lua);}
static int SLUAunregisterCallBack(lua_State *lua) {return luaInstance->unregisterCallBack(lua);}
static int SLUAspawnObject(lua_State *lua) {return luaInstance->spawnObject(lua);}
static int SLUAspawnBeam(lua_State *lua) {return luaInstance->spawnBeam(lua);}
static int SLUApointArrow(lua_State *lua) {return luaInstance->pointArrow(lua);}
static int SLUAstopTimer(lua_State *lua) {return luaInstance->stopTimer(lua);}
static int SLUAstartTimer(lua_State *lua) {return luaInstance->startTimer(lua);}
static int SLUAgetInterfaceVersion(lua_State *lua) {return luaInstance->getVersion(lua);}
static int SLUAgetSetting(lua_State *lua) {return luaInstance->getSetting(lua);}

// this enables LUA to load files through Ogre's Resource Manager in chunks
const char *luaFileReader(lua_State * L, void * data, size_t * size)
{
	static char buffer[255] = "";
	DataStreamPtr ds = *(DataStreamPtr *)data;
	*size = ds->read(buffer, 255);
	//Ogre::LogManager::getSingleton().logMessage("reading LUA chunk" + StringConverter::toString(*size));
	buffer[*size] = '\0';
	return (const char *) buffer;
}

int lua_run(lua_State *L, String filename)
{
	int s = lua_pcall (L, 0, LUA_MULTRET, 0);
	switch (s)
	{
	case 0:
		// 0 = ok
		return s;
		break;
	case LUA_ERRRUN:
		{
			Ogre::LogManager::getSingleton().logMessage("LUA runtime error while executing file "+filename);
			if(lua_isstring(L, -1))
				Ogre::LogManager::getSingleton().logMessage("LUA original error: " + String(lua_tostring (L, -1)));
			return s;
		}
		break;
	case LUA_ERRSYNTAX:
		{
			Ogre::LogManager::getSingleton().logMessage("LUA syntax error in file "+filename);
			if(lua_isstring(L, -1))
				Ogre::LogManager::getSingleton().logMessage("LUA original error: " + String(lua_tostring (L, -1)));
			break;
		}
	case LUA_ERRMEM:
		{
			Ogre::LogManager::getSingleton().logMessage("LUA memory allocation error while executing file "+filename);
			break;
		}
	case LUA_ERRERR:
		{
			Ogre::LogManager::getSingleton().logMessage("LUA error while running the error handler function while executing file "+filename);
			break;
		}
	default:
		{
			Ogre::LogManager::getSingleton().logMessage("LUA unkown error " + StringConverter::toString(s) + " in file "+filename);
			if(lua_isstring(L, -1))
				Ogre::LogManager::getSingleton().logMessage("LUA original error: " + String(lua_tostring (L, -1)));
			return s;
		}
		//error("terrain lua", s);
	}
	return s;
}

LuaSystem::LuaSystem(RoRFrameListener *efl)
{
	if (luaInstance) return; else luaInstance=this;
	L = lua_open();
	mefl=efl;
	lua_atpanic (L, SLUApanic);
	luaL_openlibs(L);
	for (int i=0; i<MAX_EVENTSOURCE; i++) {lastevents[i]=-1; curevents[i]=-1;};
	for (int i=0; i<10; i++) event_pool_id[i]=-1;

	registerFunctions();

	// load through ogre system
	DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource("base.lua", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	int s = lua_load(L, luaFileReader, ((void *)&ds), "base.lua");
	if(!s)
		lua_run(L, "base.lua");
}

bool LuaSystem::fileExists(const char *filename)
{
	return ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(filename);
}

void LuaSystem::loadTerrain(Ogre::String terrainname)
{
	char trrn[255]="";
	strncpy(trrn,terrainname.c_str(), 240);
	strcat(trrn,".lua");
	if(!fileExists(trrn))
	{
		Ogre::LogManager::getSingleton().logMessage(String(trrn)+" not found.");
		return;
	}
	String group="";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(trrn);
	}catch(...)
	{
	}
	if(group == "")
		return;
	DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(trrn, group);
	int s = lua_load(L, luaFileReader, ((void *)&ds), trrn);
	if (!s)
		lua_run(L, trrn);
}


LuaSystem::~LuaSystem()
{
	/* cleanup Lua */
	lua_close(L);
}

void LuaSystem::spawnEvent(int id, eventsource_t *source)
{
	//Ogre::LogManager::getSingleton().logMessage("LUA spawning: "+Ogre::StringConverter::toString(id)+" "+Ogre::String(source->instancename)+"("+Ogre::String(source->boxname)+")");
	//log the event
	if (curevents[id]==-1)
	{
		//this event has not been logged yet
		//search a free slot
		int pos=0;
		for (pos=0; pos<10; pos++) if (event_pool_id[pos]==-1) break;
		if (pos<10)
		{
			//we have found a free slot, allocate it
			curevents[id]=pos;
			event_pool_id[pos]=id;
			memcpy(&event_pool[pos], source, sizeof(eventsource_t));
		}
	}
}

void LuaSystem::framestep()
{
	//find out if there are new events
	for (int i=0; i<MAX_EVENTSOURCE; i++)
	{
		if (lastevents[i]==-1 && curevents[i]!=-1)
		{
			//this is a new event!
			eventsource_t *evt=&event_pool[curevents[i]];

			//call the handler at once
			if(evt->luahandler == -1)
			{
				// use default handler
				lua_getfield(L, LUA_GLOBALSINDEX, "eventHandler");
			} else
			{
				// use the sepcified handler
				lua_rawgeti(L, LUA_REGISTRYINDEX, evt->luahandler);
			}
			lua_pushstring(L, evt->instancename);
			lua_pushstring(L, evt->boxname);
			lua_call(L, 2, 0);
		}
	}
	//dispose the pool
	for (int i=0; i<10; i++) event_pool_id[i]=-1;
	//cycle the event table
	for (int i=0; i<MAX_EVENTSOURCE; i++) {lastevents[i]=curevents[i]; curevents[i]=-1;};

	// invoke framestep callbacks
	for (std::vector<String>::const_iterator c = framestepCallbacks.begin(); c != framestepCallbacks.end(); ++c)
	{
		// try to call the lua function
		Ogre::LogManager::getSingleton().logMessage(String(*c));
		lua_getglobal(L, c->c_str());
		if (lua_pcall(L, 0, 0, 0) != 0)
			Ogre::LogManager::getSingleton().logMessage("LUA error running callback function "+String(*c));
			Ogre::LogManager::getSingleton().logMessage(String(lua_tostring(L, -1)));
	}
}

void LuaSystem::error(char* msg, int err)
{
	Ogre::LogManager::getSingleton().logMessage("LUA error: "+Ogre::String(msg)+"("+Ogre::StringConverter::toString(err)+")");
	throw Exception(50,Ogre::String(msg)+"("+Ogre::StringConverter::toString(err)+")","lua file");
}

void LuaSystem::registerFunctions()
{
	lua_pushcclosure (L, SLUAflashMessage, 0);
	lua_setglobal (L, "flashMessage");
	lua_pushcclosure (L, SLUAlog, 0);
	lua_setglobal (L, "log");
	lua_pushcclosure (L, SLUAshowChooser, 0);
	lua_setglobal (L, "showChooser");
	lua_pushcclosure (L, SLUArepairVehicle, 0);
	lua_setglobal (L, "repairVehicle");
	lua_pushcclosure (L, SLUAremoveVehicle, 0);
	lua_setglobal (L, "removeVehicle");
	lua_pushcclosure (L, SLUAgetTime, 0);
	lua_setglobal (L, "getTime");
	lua_pushcclosure (L, SLUAregisterCallBack, 0);
	lua_setglobal (L, "registerCallBack");
	lua_pushcclosure (L, SLUAunregisterCallBack, 0);
	lua_setglobal (L, "unregisterCallBack");
	lua_pushcclosure (L, SLUAspawnObject, 0);
	lua_setglobal (L, "spawnObject");
	lua_pushcclosure (L, SLUAspawnBeam, 0);
	lua_setglobal (L, "spawnBeam");
	lua_pushcclosure (L, SLUApointArrow, 0);
	lua_setglobal (L, "pointArrow");
	lua_pushcclosure (L, SLUAstopTimer, 0);
	lua_setglobal (L, "stopTimer");
	lua_pushcclosure (L, SLUAstartTimer, 0);
	lua_setglobal (L, "startTimer");
	lua_pushcclosure (L, SLUAgetSetting, 0);
	lua_setglobal (L, "getSetting");
}

int LuaSystem::registerCallBack(lua_State *lua)
{
	const char *type = lua_tostring (lua, 1);
	const char *luafunction = lua_tostring (lua, 2);
	if(!strncmp(type, "framestep", 255))
	{
		// check for duplicate
		for (std::vector<String>::iterator c = framestepCallbacks.begin();c != framestepCallbacks.end(); ++c)
			if(!strncmp(c->c_str(), luafunction, 255))
				return 0;
		Ogre::LogManager::getSingleton().logMessage("LUA registering callback: "+String(type)+" to "+String(luafunction));
		framestepCallbacks.push_back(String(luafunction));
	}
	return 0;
}

int LuaSystem::unregisterCallBack(lua_State *lua)
{
	const char *type = lua_tostring (lua, 1);
	const char *luafunction = lua_tostring (lua, 2);
	if(!strncmp(type, "framestep", 255))
		// remove the entry
		for (std::vector<String>::iterator c = framestepCallbacks.begin();c != framestepCallbacks.end(); ++c)
			if(!strncmp(c->c_str(), luafunction, 255))
			{
				Ogre::LogManager::getSingleton().logMessage("LUA unregistering callback: "+String(type)+" to "+String(luafunction));
				framestepCallbacks.erase(c);
				return 0;
			}

	return 0;
}

//the LUA functions
int LuaSystem::getVersion (lua_State *lua)
{
	lua_pushstring(lua, LUA_INTERFACE_VERSION);
	return 1; // number of return values
}

int LuaSystem::panic (lua_State *lua)
{
	const char *msg = lua_tostring (lua, 1);
	Ogre::LogManager::getSingleton().logMessage("LUA PANIC: "+Ogre::String(msg));
	return 0; //this is the number of return values
}

int LuaSystem::flashMessage (lua_State *lua)
{
	const char *msg = lua_tostring (lua, 1);
	float flashtime = (float)lua_tonumber (lua, 2);
	if(flashtime < 1)
		flashtime = 1;
	if(mefl->getOverlayWrapper())
		mefl->getOverlayWrapper()->flashMessage((char*)msg, flashtime);
	return 0;
}

int LuaSystem::startTimer(lua_State *lua)
{
	mefl->startTimer();
	return 0; // number of return values
}

int LuaSystem::stopTimer(lua_State *lua)
{
	lua_pushnumber(lua, mefl->stopTimer()); // return value
	return 1; // number of return values
}

int LuaSystem::getTime(lua_State *lua)
{
	lua_pushnumber(lua, mefl->getTime()); // return value
	return 1; // number of return values
}

int LuaSystem::log(lua_State *lua)
{
	const char *msg = lua_tostring (lua, 1);
	Ogre::LogManager::getSingleton().logMessage("LUA log: "+Ogre::String(msg));
	return 0;
}

int LuaSystem::showChooser(lua_State *lua)
{
	const char *type = lua_tostring (lua, 1);
	const char *inst = lua_tostring (lua, 2);
	const char *box = lua_tostring (lua, 3);
#ifdef USE_MYGUI
	SelectorWindow::LoaderType ntype = SelectorWindow::LT_None;
	if (!strcmp("vehicle", type))   ntype = SelectorWindow::LT_Vehicle;
	if (!strcmp("truck", type))     ntype = SelectorWindow::LT_Truck;
	if (!strcmp("car", type))       ntype = SelectorWindow::LT_Truck;
	if (!strcmp("boat", type))      ntype = SelectorWindow::LT_Boat;
	if (!strcmp("airplane", type))  ntype = SelectorWindow::LT_Airplane;
	if (!strcmp("heli", type))      ntype = SelectorWindow::LT_Heli;
	if (!strcmp("trailer", type))   ntype = SelectorWindow::LT_Trailer;
	if (!strcmp("load", type))      ntype = SelectorWindow::LT_Load;
	if (!strcmp("allbeam", type))   ntype = SelectorWindow::LT_AllBeam;
	if (!strcmp("extension", type)) ntype = SelectorWindow::LT_Extension;
	if (ntype != SelectorWindow::LT_None)
		mefl->showLoad(ntype, (char*)inst, (char*)box);
#endif //USE_MYGUI
	return 0;
}

int LuaSystem::repairVehicle(lua_State *lua)
{
	const char *inst = lua_tostring (lua, 1);
	const char *box = lua_tostring (lua, 2);
	mefl->repairTruck((char*)inst, (char*)box);
	return 0;
}

int LuaSystem::removeVehicle(lua_State *lua)
{
	const char *inst = lua_tostring (lua, 1);
	const char *box = lua_tostring (lua, 2);
	mefl->removeTruck((char*)inst, (char*)box);
	return 0;
}

int LuaSystem::spawnObject(lua_State *lua)
{
	char *objectname = const_cast<char*>(lua_tostring(lua, 1));
	char *instancename = const_cast<char*>(lua_tostring(lua, 2));
	float px = lua_tonumber (lua, 3);
	float py = lua_tonumber (lua, 4);
	float pz = lua_tonumber (lua, 5);
	float rx = lua_tonumber (lua, 6);
	float ry = lua_tonumber (lua, 7);
	float rz = lua_tonumber (lua, 8);
	int luahandler = luaL_ref(lua, LUA_REGISTRYINDEX);

	// trying to create the new object
	SceneNode *bakeNode=mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode();
	mefl->loadObject(objectname, px, py, pz, rx, ry, rz, bakeNode, instancename, true, luahandler, objectname);
	return 0;
}

int LuaSystem::spawnBeam(lua_State *lua)
{
	char *truckname = const_cast<char*>(lua_tostring(lua, 1));
	char *instancename = const_cast<char*>(lua_tostring(lua, 2));
	float px = lua_tonumber (lua, 3);
	float py = lua_tonumber (lua, 4);
	float pz = lua_tonumber (lua, 5);
	float rx = lua_tonumber (lua, 6);
	float ry = lua_tonumber (lua, 7);
	float rz = lua_tonumber (lua, 8);

	// trying to create the new object
	Vector3 pos = Vector3(px, py, pz);
	int num = BeamExists(instancename);
	if(num == -1)
	{
		// non existent, create
		Ogre::LogManager::getSingleton().logMessage("LUA truck-create");
		num = mefl->addTruck(truckname, pos);
	}
	Ogre::LogManager::getSingleton().logMessage("LUA using trucknum "+Ogre::StringConverter::toString(num));
	beamMap[instancename] = num;
	Beam *truck = mefl->getTruck(num);
	truck->resetPosition(px, pz, false, py);
	truck->updateVisual();
	return 0;
}

int LuaSystem::pointArrow(lua_State *lua)
{
	float px = lua_tonumber (lua, 1);
	float py = lua_tonumber (lua, 2);
	float pz = lua_tonumber (lua, 3);
	char *text = const_cast<char*>(lua_tostring(lua, 4));
	if(px==0 && py ==0 && pz == 0)
		mefl->setDirectionArrow(0, Vector3::ZERO);
	else
		mefl->setDirectionArrow(text, Vector3(px, py, pz));
	return 0;
}

int LuaSystem::getSetting(lua_State *lua)
{
	char *text = const_cast<char*>(lua_tostring(lua, 1));
	if(!text) return 0;
	String res = SETTINGS.getSetting(String(text));

	lua_pushlstring(lua, res.c_str(), res.size()); // return value
	return 1; // number of return values
}

int LuaSystem::BeamExists(char *instance_name)
{
	std::map<String, int>::iterator allit = beamMap.find(instance_name);
	if(allit == beamMap.end())
		return -1;
	else
		return allit->second;
}

#endif //LUASCRIPT
