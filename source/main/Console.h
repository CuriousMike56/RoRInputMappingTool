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

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "Singleton.h"
#include <BaseLayout.h>
#include "rormemory.h"
#include <OgreLog.h>
#include <pthread.h>

ATTRIBUTE_CLASS_LAYOUT(Console, "Console.layout");
class Console :
	public wraps::BaseLayout,
	public Singleton2<Console>,
	public MemoryAllocatedObject,
	public Ogre::LogListener
{
	friend class Singleton2<Console>;
	Console();
	~Console();
public:

	void setVisible(bool _visible);
	bool getVisible();

	void print(const MyGUI::UString &text);

	// method from Ogre::LogListener
	virtual void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName );
	// print waiting messages
	void frameEntered(float _frame);
protected:
	void eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void eventCommandAccept(MyGUI::Edit* _sender);

	ATTRIBUTE_FIELD_WIDGET_NAME(Console, mLogEdit, "Log");
	MyGUI::Edit* mLogEdit;
	ATTRIBUTE_FIELD_WIDGET_NAME(Console, mCommandEdit, "Command");
	MyGUI::Edit* mCommandEdit;

	bool mVisible;

	pthread_mutex_t mWaitingMessagesMutex;
	std::vector<MyGUI::UString> mWaitingMessages;

	int mHistoryPosition;
	std::vector<MyGUI::UString> mHistory;
};

#endif // __CONSOLE_H__

#endif //MYGUI

