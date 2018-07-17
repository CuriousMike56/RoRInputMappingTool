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

#ifndef GUI_INPUTMANAGER_H__
#define GUI_INPUTMANAGER_H__

#include <MyGUI.h>
#include <OIS.h>
class InputEngine;

class GUIInputManager
// DO NOT use the listeners here, it will conflict with the real listeners in the inputmanager!
// rather forward the events from the inputmanager
//: public OIS::MouseListener, public OIS::KeyListener
{
	friend class InputEngine;
public:
    GUIInputManager();
    virtual ~GUIInputManager();

    void setInputViewSize(int _width, int _height);

    virtual void injectMouseMove(int _absx, int _absy, int _absz) { }
    virtual void injectMousePress(int _absx, int _absy, MyGUI::MouseButton _id) { }
    virtual void injectMouseRelease(int _absx, int _absy, MyGUI::MouseButton _id) { }
    virtual void injectKeyPress(MyGUI::KeyCode _key, MyGUI::Char _text) { }
    virtual void injectKeyRelease(MyGUI::KeyCode _key) { }

    void setMousePosition(int _x, int _y);
    void updateCursorPosition();

protected:
    virtual bool mouseMoved(const OIS::MouseEvent& _arg);
    virtual bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    virtual bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
    virtual bool keyPressed(const OIS::KeyEvent& _arg);
    virtual bool keyReleased(const OIS::KeyEvent& _arg);

    void checkPosition();

private:
    //OIS::InputManager* mInputManager;
    //OIS::Keyboard* mKeyboard;
    //OIS::Mouse* mMouse;

    int mCursorX, mCursorY, width, height;
};

#endif // GUI_INPUTMANAGER_H__
#endif //MYGUI

