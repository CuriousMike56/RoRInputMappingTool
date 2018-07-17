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

#ifndef __LOADPROGRESS_H__
#define __LOADPROGRESS_H__

#include "Singleton.h"
#include <BaseLayout.h>
#include <Ogre.h>

ATTRIBUTE_CLASS_LAYOUT(LoadingWindow, "LoadingWindow.layout");
class LoadingWindow :
	public wraps::BaseLayout,
	public Singleton2<LoadingWindow>
{
	friend class Singleton2<LoadingWindow>;
	LoadingWindow();
	~LoadingWindow();
public:

	void setProgress(int _percent, const Ogre::String& _text = "", bool _updateRenderFrame = true);
	void setAutotrack(const Ogre::String& _text = "", bool _updateRenderFrame = true);

	void hide();

	bool getFrameForced();
private:
	void renderOneFrame();
private:
	ATTRIBUTE_FIELD_WIDGET_NAME(LoadingWindow, mBarProgress, "Bar");
	MyGUI::Progress* mBarProgress;
	ATTRIBUTE_FIELD_WIDGET_NAME(LoadingWindow, mInfoStaticText, "Info");
	MyGUI::StaticText* mInfoStaticText;

	bool mFrameForced;
};

#endif // __LOADPROGRESS_H__

#endif //MYGUI

