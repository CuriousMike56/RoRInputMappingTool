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

#ifndef __LuaSystem_H__
#define __LuaSystem_H__

#define LUA_INTERFACE_VERSION "0.1.0"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "Ogre.h"
using namespace Ogre;
#include "collisions.h"
#include "RoRFrameListener.h"
#include <vector>
#include <map>

class LuaSystem : public MemoryAllocatedObject
{
private:
	lua_State* L;
	RoRFrameListener* mefl;
	int event_pool_id[10];
	eventsource_t event_pool[10];
	int lastevents[MAX_EVENTSOURCE];
	int curevents[MAX_EVENTSOURCE];
	std::vector<String> framestepCallbacks;
	std::map<String, int> beamMap;
	int BeamExists(char *instance_name);
	bool fileExists(const char *filename);

public:
	LuaSystem(RoRFrameListener *efl);
	~LuaSystem();
	void spawnEvent(int id, eventsource_t *source);
	void framestep();
	void error(char* msg, int err);
	void registerFunctions();
	
	//the LUA functions
	int getVersion(lua_State *lua);
	int panic(lua_State *lua);
	int flashMessage(lua_State *lua);
	int log(lua_State *lua);
	int showChooser(lua_State *lua);
	int repairVehicle(lua_State *lua);
	int removeVehicle(lua_State *lua);
	int getTime(lua_State *lua);
	int registerCallBack(lua_State *lua);
	int unregisterCallBack(lua_State *lua);
	int spawnObject(lua_State *lua);
	int spawnBeam(lua_State *lua);
	int pointArrow(lua_State *lua);
	int startTimer(lua_State *lua);
	int stopTimer(lua_State *lua);
	int getSetting(lua_State *lua);

	void loadTerrain(Ogre::String terrainname);
};

#endif

#endif //LUASCRIPT
