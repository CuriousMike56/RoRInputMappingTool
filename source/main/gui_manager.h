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
#ifdef USE_MYGUI 

#ifndef GUI_MANAGER_H__
#define GUI_MANAGER_H__

#include <Ogre.h>
#include <MyGUI.h>
#include "gui_inputmanager.h"
#include "rormemory.h"

#define GETMYGUI GUIManager::getSingleton().getGUI()

namespace MyGUI { class OgrePlatform; }

class GUIManager : public GUIInputManager, public Ogre::FrameListener, public Ogre::WindowEventListener, public Ogre::Singleton<GUIManager>, public MemoryAllocatedObject
{
public:
	static GUIManager& getSingleton(void);
	static GUIManager* getSingletonPtr(void);

	GUIManager(Ogre::Root *root, Ogre::SceneManager *mgr, Ogre::RenderWindow *win);
	virtual ~GUIManager();

	void destroy();

	MyGUI::Gui* getGUI() { return mGUI; }

protected:
	virtual void injectMouseMove(int _absx, int _absy, int _absz);
	virtual void injectMousePress(int _absx, int _absy, MyGUI::MouseButton _id);
	virtual void injectMouseRelease(int _absx, int _absy, MyGUI::MouseButton _id);
	virtual void injectKeyPress(MyGUI::KeyCode _key, MyGUI::Char _text);
	virtual void injectKeyRelease(MyGUI::KeyCode _key);

private:
	bool create();
	void createGui();
	void destroyGui();

	virtual bool frameStarted(const Ogre::FrameEvent& _evt);
	virtual bool frameEnded(const Ogre::FrameEvent& _evt);
	virtual void windowResized(Ogre::RenderWindow* _rw);
	virtual void windowClosed(Ogre::RenderWindow* _rw);

	void eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result);

private:
	MyGUI::Gui* mGUI;
	MyGUI::OgrePlatform* mPlatform;

	Ogre::Root *mRoot;
	Ogre::SceneManager* mSceneManager;
	Ogre::RenderWindow* mWindow;

	bool mExit;

	Ogre::String mResourceFileName;
};

#endif // GUI_MANAGER_H__

#endif //MYGUI

