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
#include "IngameConsole.h"
#include "RoRFrameListener.h"
#include "language.h"

using namespace std;
using namespace Ogre;

IngameConsole::IngameConsole()
{
	consoleOverlay = 0;
	displaymode=-1;
	top=width=left=height=0;
	lineheight = 16; //lineheight = fontheight
	bordersize = 10;
	isvisible=false;
	cursorBlink=false;
	cursorBlinkState=false;
	lastCursorBlink=0;
	scrollOffset=0;
	scrolling=false;
	mefl=0;
}

IngameConsole::~IngameConsole()
{
}

void IngameConsole::setFontSize(int size)
{
	lineheight = size;
	destroyOverlays();
	createOverlays();
	updateDisplay();
}

void IngameConsole::setMode(RoRFrameListener *efl, int mode, bool visible)
{
	this->mefl=efl;
	//LogManager::getSingleton().logMessage("CONSOLE setmode (" + StringConverter::toString(mode) + ") ...");

	// visibility
	if(visible != isvisible && !visible && consoleOverlay)
	{
		consoleOverlay->hide();
		isvisible = false;
	}
	else if(visible != isvisible && visible && consoleOverlay)
	{
		consoleOverlay->show();
		isvisible = true;
		updateDisplay();
	}
	// mode changes
	if(mode == displaymode)
		return;

	if(mode >= NETCHAT_END)
		mode = NETCHAT_LEFT_SMALL;
	if(mode < NETCHAT_LEFT_SMALL)
		mode = NETCHAT_LEFT_SMALL;
	displaymode = mode;

	unsigned int screenWidth, screenHeight, depth;
	int left, top;
	RenderWindow *rw = mefl->getRenderWindow();
	rw->getMetrics(screenWidth, screenHeight, depth, left, top);

	if (mode == NETCHAT_LEFT_SMALL) // 0
	{
		resize(0, (int)(((float)screenHeight) * 0.56), 600, (int)(ceil(((float)screenHeight) * 0.2)));
	}
	else if (mode == NETCHAT_LEFT_FULL) // 1
	{
    resize(0, 0, 600, (int)(((float)screenHeight) * 0.8));
	}
	else if (mode == NETCHAT_TOP) // 2
	{
		resize((int)(((float)screenWidth)*0.5) - 300, 0, 600, 115);
	}
	else if (mode == NETCHAT_MAP) // 3
	{
		resize(205, (int)(((float)screenHeight) * 0.56), screenWidth-210, (int)(ceil(((float)screenHeight) * 0.2)));
	}
	
	//LogManager::getSingleton().logMessage("CONSOLE mode set.");
}

void IngameConsole::resize(int _left, int _top, int _width, int _height)
{
	top=_top;
	left=_left;
	width=_width;
	height=_height;
	//LogManager::getSingleton().logMessage("CONSOLE resizing (" + StringConverter::toString(left) + ", " + StringConverter::toString(top) + ", " + StringConverter::toString(width) + ", " + StringConverter::toString(height) + ") ...");
	destroyOverlays();
	createOverlays();
	updateDisplay();
	//LogManager::getSingleton().logMessage("CONSOLE resized.");
}

void IngameConsole::setVisible(bool visible)
{
	if(mefl)
		setMode(mefl, displaymode, visible);
}

void IngameConsole::toggleVisible(RoRFrameListener *efl)
{
	setMode(efl, displaymode, !isvisible);
}

int IngameConsole::toggleMode(RoRFrameListener *efl)
{
	setMode(efl, displaymode+1, isvisible);
	return displaymode;
}

int IngameConsole::getMode()
{
	return displaymode;
}


bool IngameConsole::destroyOverlays()
{
	if(consoleOverlay)
	{
		OverlayManager::getSingleton().destroyOverlayElement((OverlayElement *)consoleOverlay);
	}
	std::vector<ColoredTextAreaOverlayElement *>::iterator i;
	for(i = chatLines.begin(); i!=chatLines.end(); i++)
	{
		OverlayManager::getSingleton().destroyOverlayElement((OverlayElement *)*i);
	}
	chatLines.clear();
	return true;
}

bool IngameConsole::createOverlays()
{
	OverlayManager& overlayManager = OverlayManager::getSingleton();

	// Create a panel
	consoleOverlay = static_cast<OverlayContainer*>(overlayManager.createOverlayElement("Panel", "tracks/ConsoleOverlay"));
	consoleOverlay->setMetricsMode(Ogre::GMM_PIXELS);
	consoleOverlay->setPosition(left, top);
	consoleOverlay->setDimensions(width, height);
	if(left==0)
		consoleOverlay->setMaterialName("tracks/netchat2/bg/left");
	else
		consoleOverlay->setMaterialName("tracks/netchat2/bg/box");

	int linespace = 0; //space between lines
	int chatlines = (int)(floor((double)((height - bordersize)  / (double)(lineheight + linespace))));
	// Create the chatlines
	for(int i=0;i<chatlines;i++)
	{
		char tmp[255];
		sprintf(tmp,"tracks/ConsoleOverlay/Chatline%d",i);
		ColoredTextAreaOverlayElement *textArea = static_cast<ColoredTextAreaOverlayElement*>(overlayManager.createOverlayElement("ColoredTextArea", tmp));
		textArea->setMetricsMode(Ogre::GMM_PIXELS);
		textArea->setPosition(bordersize, bordersize + (lineheight + linespace) * i);
		textArea->setDimensions(width, lineheight);
		textArea->setCharHeight(lineheight + 2);
		textArea->setFontName("Cyberbit");
		textArea->setValueBottom(0.7);
		textArea->setValueTop(0.9);
		textArea->setCaption("");
		//textArea->setCaption(UTFString("^1Line^7 ")+StringConverter::toString(i));
		textArea->show();
		chatLines.push_back(textArea);
		// Add the text area to the panel
		consoleOverlay->addChild(textArea);
	}
	OverlayManager::getSingleton().getByName("tracks/netchat2")->add2D(consoleOverlay);
	OverlayManager::getSingleton().getByName("tracks/netchat2")->show();
	if(isvisible)
		consoleOverlay->show();
	else
		consoleOverlay->hide();
	return true;
}

bool IngameConsole::update(float dt)
{
	if(cursorBlink)
	{
		lastCursorBlink -= dt;
		if(lastCursorBlink <= 0)
		{
			cursorBlinkState = !cursorBlinkState;
			lastCursorBlink = 0.3;
			updateEnterText();
		}
	}
	return true;
}

bool IngameConsole::addText(UTFString msg, bool addtime)
{
	try {
		chatBuffer.push_back(msg);
		updateDisplay();
		return true;
	} catch(...)
	{}
	return false;
}

void IngameConsole::updateEnterText()
{
	static const UTFString cursor = "_";
	std::vector<ColoredTextAreaOverlayElement *>::reverse_iterator enterline = chatLines.rbegin();
	try
	{
		if(cursorBlinkState)
		{
			(*enterline)->setCaption("^7> " + enterText + cursor);
		}
		else
		{
			(*enterline)->setCaption("^7> " + enterText);
		}
	} catch(Ogre::Exception& e)
	{
		(*enterline)->setCaption("^7> ^1CODEC ERROR: "+e.getFullDescription());
	}
}

bool IngameConsole::setEnterText(UTFString msg, bool visible, bool cursor)
{
	enterText = msg;
	cursorBlink = cursor;
	std::vector<ColoredTextAreaOverlayElement *>::reverse_iterator enterline = chatLines.rbegin();
	if(!visible)
	{
		(*enterline)->setCaption("");
	}else
		updateEnterText();
	return true;
}

void IngameConsole::updateDisplay()
{
	if(!isvisible)
		return;
	if(chatLines.size() == 0 || chatBuffer.size() == 0)
		return;
	std::vector<ColoredTextAreaOverlayElement *>::reverse_iterator i;
	std::vector<UTFString>::reverse_iterator ib;
	// chatLines + 1 because last line is for entering something!
	for(i = chatLines.rbegin() + 1, ib = chatBuffer.rbegin() + (scrollOffset); i!=chatLines.rend() && ib != chatBuffer.rend(); i++, ib++)
	{
		//LogManager::getSingleton().logMessage(*ib);
		try
		{
			(*i)->setCaption(*ib);
		}
		catch(...)
		{
			(*i)->setCaption(_L("unkown encoding problem"));
		}
	}
}


// XXX: TODO: get this working!
void IngameConsole::scrollPageUp()
{
	return;
	if(chatBuffer.size() < chatLines.size())
		return;
	int max = (int)(ceil((double)chatBuffer.size() / (double)chatLines.size()));
	scrollOffset++;
	if(scrollOffset > max)
		scrollOffset = max;
	LogManager::getSingleton().logMessage(StringConverter::toString(scrollOffset));
	scrolling=true;
	updateDisplay();
}

void IngameConsole::scrollPageDown()
{
	return;
	if(chatBuffer.size() < chatLines.size())
		return;
	scrollOffset--;
	if(scrollOffset < 0)
		scrollOffset = 0;
	LogManager::getSingleton().logMessage(StringConverter::toString(scrollOffset));
	scrolling=true;
	updateDisplay();
}

void IngameConsole::noScroll()
{
	return;
	scrollOffset=0;
	scrolling=false;
	updateDisplay();
}
