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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 13th of August 2009
#ifdef USE_MYGUI 

#ifndef GUI_MENU_H__
#define GUI_MENU_H__

#include <MyGUI.h>
#include "OgreSingleton.h"
#include "OgrePrerequisites.h"
#include "rormemory.h"

class RoRFrameListener;

class GUI_MainMenu : public Ogre::Singleton< GUI_MainMenu >, public MemoryAllocatedObject
{
public:
	GUI_MainMenu(RoRFrameListener *efl);
	~GUI_MainMenu();
	static GUI_MainMenu& getSingleton(void);
	static GUI_MainMenu* getSingletonPtr(void);

	bool getVisible();
	void setVisible(bool value);
protected:
	RoRFrameListener *mefl;
	MyGUI::MenuBarPtr mainmenu;
	void onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item);
};

#endif //GUI_MENU_H__

#endif //MYGUI

