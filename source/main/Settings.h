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
// created by thomas fischer, 4th of January 2009
#ifndef SETTINGS_H
#define SETTINGS_H

#include "Ogre.h"
#include <map>

#define SETTINGS Settings::Instance()

class Settings
{
public:
	static Settings & Instance();
	Ogre::String getSetting(Ogre::String key);
	Ogre::String getSettingScriptSafe(Ogre::String key);
	void setSetting(Ogre::String key, Ogre::String value);
	bool setupPaths();
	void loadSettings(Ogre::String configFile, bool overwrite=false);
	void saveSettings(Ogre::String configFile);
	void saveSettings();

	void checkGUID();
	void createGUID();

#ifdef USE_ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

protected:
	Settings();
	~Settings();
	Settings(const Settings&);
	Settings& operator= (const Settings&);
	static Settings* myInstance;

	// members
	std::map<Ogre::String, Ogre::String> settings;

	// methods
	void path_descend(char* path);
	void path_add(char* path, const char* dirname);

	bool get_system_paths(char *program_path, char *user_path);

};
#endif
