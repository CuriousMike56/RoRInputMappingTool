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
// created on 31th of July 2009 by Thomas Fischer

#ifdef USE_ANGELSCRIPT

#ifndef AS_USE_NAMESPACE
#define AS_USE_NAMESPACE
#endif //AS_USE_NAMESPACE

#ifndef AS_OGRE_H_
#define AS_OGRE_H_

#include "angelscript.h"
#include "Ogre.h"

void registerOgreObjects(AngelScript::asIScriptEngine *engine);

#endif //AS_OGRE_H_
#endif //ANGELSCRIPT

