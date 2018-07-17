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

#ifndef __INGAMECHAT_H__
#define __INGAMECHAT_H__


#include <vector>
#include "Ogre.h"
#include "OgreTextAreaOverlayElement.h"
#include "ColoredTextAreaOverlayElement.h"
#include "Singleton.h"
#include "rormemory.h"

#define NETCHAT IngameConsole::getInstance()

enum {NETCHAT_LEFT_SMALL, NETCHAT_LEFT_FULL, NETCHAT_TOP, NETCHAT_MAP, NETCHAT_END};
enum {NETCHAT_MSG_CHAT, NETCHAT_MSG_COMMAND};

class RoRFrameListener;

class IngameConsole :
	public Singleton2<IngameConsole>,
	public MemoryAllocatedObject
{
	friend class Singleton2<IngameConsole>;
	IngameConsole();
	~IngameConsole();
public:
	bool update(float dt);
	void setMode(RoRFrameListener *mefl, int mode, bool visible);
	int getMode();
	int toggleMode(RoRFrameListener *mefl);
	void toggleVisible(RoRFrameListener *mefl);
	bool addText(Ogre::UTFString msg, bool addtime=false);
	bool setEnterText(Ogre::UTFString msg, bool visible=true, bool cursor=false);
	void resize(int left, int top, int width, int height);
	bool getVisible() { return isvisible; };
	void setVisible(bool visible);

	int getFontSize() { return lineheight; };
	void setFontSize(int size);

	int parseCommand(Ogre::UTFString &msg, std::vector<Ogre::UTFString> &arguments);

	void scrollPageUp();
	void scrollPageDown();
	void noScroll();

private:
    static IngameConsole *myInstance;

	int top, left, width, height;
	int lineheight, bordersize;
	int displaymode;
	RoRFrameListener *mefl;

	// cursor stuff
	Ogre::UTFString enterText;
	bool cursorBlink;
	bool cursorBlinkState;
	float lastCursorBlink;

	//scroll stuff
	int scrollOffset;
	bool scrolling;

	bool isvisible;
	bool createOverlays();
	bool destroyOverlays();
	void updateEnterText();

	Ogre::OverlayContainer *consoleOverlay;
	std::vector < ColoredTextAreaOverlayElement * > chatLines;
	std::vector < Ogre::UTFString > chatBuffer;
	void updateDisplay();
};


#endif
