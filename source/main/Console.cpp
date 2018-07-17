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

#include "Console.h"
#include "ScriptEngine.h"
#include "InputEngine.h"
#include "OgreLogManager.h"

Console::Console()
{
	initialiseByAttributes(this);

	// make it 1/3 of screen height and place over screen
	MyGUI::IntSize size = MyGUI::RenderManager::getInstance().getViewSize();
	size.height = size.height/3;
	mMainWidget->setCoord(0, -size.height, size.width, size.height);

	mCommandEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &Console::eventButtonPressed);
	mCommandEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &Console::eventCommandAccept);

	mHistoryPosition = 0;
	mHistory.push_back("");

	setVisible(false);

	pthread_mutex_init(&mWaitingMessagesMutex, NULL);

	MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate( this, &Console::frameEntered );
	Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);
}

Console::~Console()
{
	Ogre::LogManager::getSingleton().getDefaultLog()->removeListener(this);
	MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate( this, &Console::frameEntered );

	pthread_mutex_destroy(&mWaitingMessagesMutex);
}

void Console::setVisible(bool _visible)
{
	mVisible = _visible;

	mMainWidget->setEnabledSilent(_visible);
	if (_visible)
	{
		MyGUI::InputManager::getInstance().setKeyFocusWidget(mCommandEdit);
	}
	else
	{
		MyGUI::InputManager::getInstance().resetKeyFocusWidget(mCommandEdit);
	}
}

bool Console::getVisible()
{
	return mVisible;
}

void Console::print(const MyGUI::UString &_text)
{
	pthread_mutex_lock(&mWaitingMessagesMutex);
	mWaitingMessages.push_back(_text);
	pthread_mutex_unlock(&mWaitingMessagesMutex);
}

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
std::wstring ansi_to_utf16(const std::string& _source)
{
	const char* srcPtr = _source.c_str();
	int tmpSize = MultiByteToWideChar( CP_ACP, 0, srcPtr, -1, 0, 0 );
	WCHAR* tmpBuff = new WCHAR [ tmpSize + 1 ];
	MultiByteToWideChar( CP_ACP, 0, srcPtr, -1, tmpBuff, tmpSize );
	std::wstring ret = tmpBuff;
	delete[] tmpBuff;
	return ret;
}
#endif

void Console::messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName )
{
	Ogre::String msg = message;
	//this->print(logName+": "+message);
	// strip script engine things
	if(message.substr(0,4) == "SE| ")
		msg = message.substr(4);
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
	this->print(ansi_to_utf16(msg));
#else
	this->print(msg);
#endif
}

void Console::frameEntered(float _frame)
{
	// only copy the content and then unlock again
	// this prevents that loggers are hung due to time consuming iteration over it
	std::vector<MyGUI::UString> tmpWaitingMessages;
	pthread_mutex_lock(&mWaitingMessagesMutex);
	tmpWaitingMessages = mWaitingMessages;
	mWaitingMessages.clear();
	pthread_mutex_unlock(&mWaitingMessagesMutex);

	if (tmpWaitingMessages.empty())
		return;
	
	for (std::vector<MyGUI::UString>::iterator iter = tmpWaitingMessages.begin(); iter != tmpWaitingMessages.end(); ++iter)
	{
		if (!mLogEdit->getCaption().empty())
			mLogEdit->addText("\n" + *iter);
		else
			mLogEdit->addText(*iter);

		mLogEdit->setTextSelection(mLogEdit->getTextLength(), mLogEdit->getTextLength());
	}
}

void Console::eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	if(_key == MyGUI::KeyCode::Escape || _key == MyGUI::KeyCode::Enum(INPUTENGINE.getKeboardKeyForCommand(EV_COMMON_CONSOLEDISPLAY)))
	{
		setVisible(false);
		// delete last character (to avoid printing `)
		size_t lastChar = mCommandEdit->getTextLength() - 1;
		if (_key != MyGUI::KeyCode::Escape && mCommandEdit->getCaption()[lastChar] == '`')
			mCommandEdit->eraseText(lastChar);
		return;
	}

	switch(_key.toValue())
	{
	case MyGUI::KeyCode::ArrowUp:
		if(mHistoryPosition > 0)
		{
			// first we save what we was writing
			if (mHistoryPosition == (int)mHistory.size() - 1)
			{
				mHistory[mHistoryPosition] = mCommandEdit->getCaption();
			}
			mHistoryPosition--;
			mCommandEdit->setCaption(mHistory[mHistoryPosition]);
		}
		break;

	case MyGUI::KeyCode::ArrowDown:
		if(mHistoryPosition < (int)mHistory.size() - 1)
		{
			mHistoryPosition++;
			mCommandEdit->setCaption(mHistory[mHistoryPosition]);
		}
		break;

	case MyGUI::KeyCode::PageUp:
		if (mLogEdit->getVScrollPosition() > (size_t)mLogEdit->getHeight())
			mLogEdit->setVScrollPosition(mLogEdit->getVScrollPosition() - mLogEdit->getHeight());
		else
			mLogEdit->setVScrollPosition(0);
		break;

	case MyGUI::KeyCode::PageDown:
		mLogEdit->setVScrollPosition(mLogEdit->getVScrollPosition() + mLogEdit->getHeight());
		break;
	}
}

void Console::eventCommandAccept(MyGUI::Edit* _sender)
{
	MyGUI::UString command = _sender->getCaption();

	// special command
	if(command == "hide")
	{
		setVisible(false);
	}
	else
	{
		print(command);
#ifdef USE_ANGELSCRIPT
		ScriptEngine::getSingleton().executeString(command);
#endif //ANGELSCRIPT
	}
	*mHistory.rbegin() = command;
	mHistory.push_back(""); // new, empty last entry
	mHistoryPosition = mHistory.size() - 1; // switch to the new line
	mCommandEdit->setCaption(mHistory[mHistoryPosition]);
}

#endif //MYGUI

