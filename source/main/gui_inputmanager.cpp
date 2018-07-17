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

#include "gui_inputmanager.h"

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#include <windows.h>
#endif

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
MyGUI::Char translateWin32Text(MyGUI::KeyCode kc)
{
    static WCHAR deadKey = 0;

    BYTE keyState[256];
    HKL  layout = GetKeyboardLayout(0);
    if ( GetKeyboardState(keyState) == 0 )
        return 0;

    int code = *((int*)&kc);
    unsigned int vk = MapVirtualKeyEx((UINT)code, 3, layout);
    if ( vk == 0 )
        return 0;

    WCHAR buff[3] = { 0, 0, 0 };
    int ascii = ToUnicodeEx(vk, (UINT)code, keyState, buff, 3, 0, layout);
    if (ascii == 1 && deadKey != '\0' )
    {
        // A dead key is stored and we have just converted a character key
        // Combine the two into a single character
        WCHAR wcBuff[3] = { buff[0], deadKey, '\0' };
        WCHAR out[3];

        deadKey = '\0';
        if(FoldStringW(MAP_PRECOMPOSED, (LPWSTR)wcBuff, 3, (LPWSTR)out, 3))
            return out[0];
    }
    else if (ascii == 1)
    {
        // We have a single character
        deadKey = '\0';
        return buff[0];
    }
    else if(ascii == 2)
    {
        // Convert a non-combining diacritical mark into a combining diacritical mark
        // Combining versions range from 0x300 to 0x36F; only 5 (for French) have been mapped below
        // http://www.fileformat.info/info/unicode/block/combining_diacritical_marks/images.htm
        switch(buff[0])	{
        case 0x5E: // Circumflex accent: �
            deadKey = 0x302; break;
        case 0x60: // Grave accent: �
            deadKey = 0x300; break;
        case 0xA8: // Diaeresis: �
            deadKey = 0x308; break;
        case 0xB4: // Acute accent: �
            deadKey = 0x301; break;
        case 0xB8: // Cedilla: �
            deadKey = 0x327; break;
        default:
            deadKey = buff[0]; break;
        }
    }

    return 0;
}
#endif

GUIInputManager::GUIInputManager() :
    //mInputManager(0),
    //mKeyboard(0),
    //mMouse(0),
    mCursorX(0),
    mCursorY(0),
    width(0),
    height(0)
{
}

GUIInputManager::~GUIInputManager()
{
}


bool GUIInputManager::mouseMoved(const OIS::MouseEvent& _arg)
{
	mCursorX = _arg.state.X.abs;
	mCursorY = _arg.state.Y.abs;
    checkPosition();

    injectMouseMove(mCursorX, mCursorY, _arg.state.Z.abs);
    return true;
}

bool GUIInputManager::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    injectMousePress(mCursorX, mCursorY, MyGUI::MouseButton::Enum(_id));
    return true;
}

bool GUIInputManager::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    injectMouseRelease(mCursorX, mCursorY, MyGUI::MouseButton::Enum(_id));
    return true;
}

bool GUIInputManager::keyPressed(const OIS::KeyEvent& _arg)
{
    MyGUI::Char text = (MyGUI::Char)_arg.text;
    MyGUI::KeyCode key = MyGUI::KeyCode::Enum(_arg.key);
    int scan_code = key.toValue();

    if (scan_code > 70 && scan_code < 84)
    {
        static MyGUI::Char nums[13] = { 55, 56, 57, 45, 52, 53, 54, 43, 49, 50, 51, 48, 46 };
        text = nums[scan_code-71];
    }
    else if (key == MyGUI::KeyCode::Divide)
    {
        text = '/';
    }
    else
    {
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
        text = translateWin32Text(key);
#endif
    }

    injectKeyPress(key, text);
    return true;
}

bool GUIInputManager::keyReleased(const OIS::KeyEvent& _arg)
{
    injectKeyRelease(MyGUI::KeyCode::Enum(_arg.key));
    return true;
}

void GUIInputManager::setInputViewSize(int _width, int _height)
{
    this->width = _width;
    this->height = _height;

    checkPosition();
}

void GUIInputManager::setMousePosition(int _x, int _y)
{
    mCursorX = _x;
    mCursorY = _y;

    checkPosition();
}

void GUIInputManager::checkPosition()
{
    if (mCursorX < 0)
        mCursorX = 0;
    else if (mCursorX >= this->width)
        mCursorX = this->width - 1;

    if (mCursorY < 0)
        mCursorY = 0;
    else if (mCursorY >= this->height)
        mCursorY = this->height - 1;
}

void GUIInputManager::updateCursorPosition()
{
    //if(!mMouse) return;
    //const OIS::MouseState &ms = mMouse->getMouseState();
    //injectMouseMove(mCursorX, mCursorY, ms.Z.abs);
}

#endif //MYGUI


