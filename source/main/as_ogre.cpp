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

#include "as_ogre.h"

using namespace Ogre;
using namespace AngelScript;

// helper/wrapper functions first
static void Vector3DefaultConstructor(Vector3 *self)
{
	new(self) Vector3();
}

static void Vector3CopyConstructor(const Vector3 &other, Vector3 *self)
{
	new(self) Vector3(other);
}

static void Vector3InitConstructor(float x, float y, float z, Vector3 *self)
{
	new(self) Vector3(x,y,z);
}

static void Vector3InitConstructorScaler(float s, Vector3 *self)
{
	new(self) Vector3(s,s,s);
}

// main registration method
void registerOgreObjects(AngelScript::asIScriptEngine *engine)
{
	int r;

	// note: this is sorted by the appearance in OgreVector3.h
	// Ogre::Vector3
	r = engine->RegisterObjectType("Vector3", sizeof(Ogre::Vector3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA); assert( r >= 0 );


	// Register the object properties
	r = engine->RegisterObjectProperty("Vector3", "float x", offsetof(Ogre::Vector3, x)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("Vector3", "float y", offsetof(Ogre::Vector3, y)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("Vector3", "float z", offsetof(Ogre::Vector3, z)); assert( r >= 0 );


	// Register the object methods
	r = engine->RegisterObjectMethod("Vector3", "float length() const", asMETHOD(Vector3,length), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", "float squaredLength() const", asMETHOD(Vector3,squaredLength), asCALL_THISCALL); assert( r >= 0 );

	// Register the object operators
	r = engine->RegisterObjectMethod("Vector3", "Vector3 &f(const Vector3 &in)", asMETHODPR(Vector3, operator =, (const Vector3 &), Vector3&), asCALL_THISCALL); assert( r >= 0 );

	// TODO : register the rest
	/*
	//r = engine->RegisterObjectMethod("Vector3", "Vector3 &f(const float &in)",   asMETHODPR(Vector3, operator =, (const float &)  , Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_CONSTRUCT,  "void f()",                        asFUNCTION(Vector3DefaultConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_CONSTRUCT,  "void f(float, float, float)",     asFUNCTION(Vector3InitConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_CONSTRUCT,  "void f(const vector3 &in)",       asFUNCTION(Vector3CopyConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_CONSTRUCT,  "void f(float)",                   asFUNCTION(Vector3InitConstructorScaler), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_EQUAL,      "bool f(const Vector3 in)",        asMETHODPR(Vector3, operator==,(const Vector3&), const bool), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_NOTEQUAL,   "bool f(const Vector3 in)",        asMETHODPR(Vector3, operator!=,(const Vector3&), const bool), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_ADD,        "Vector3 f(const Vector3 &in)",    asMETHODPR(Vector3, operator+,(const Vector3&), const Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_SUBTRACT,   "Vector3 f(const Vector3 &in)",    asMETHODPR(Vector3, operator-,(const Vector3&), const Vector3), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_MULTIPLY,   "Vector3 f(const float in)",       asMETHODPR(Vector3, operator*,(const float&), const Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_MULTIPLY,   "Vector3 f(const Vector3 &in)",    asMETHODPR(Vector3, operator*,(const Vector3&), const Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_DIVIDE,     "Vector3 f(const float in)",       asMETHODPR(Vector3, operator/,(const float&), const Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_DIVIDE,     "Vector3 f(const Vector3 &in)",    asMETHODPR(Vector3, operator/,(const Vector3&), const Vector3), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_ADD,        "Vector3 f()",                     asMETHODPR(Vector3, operator+,(), const Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_SUBTRACT,   "Vector3 f()",                     asMETHODPR(Vector3, operator-,(), const Vector3), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_MULTIPLY,   "Vector3 f(const float, const Vector3 &in)", asMETHODPR(Vector3, operator*,(const float, const Vector3&), const Vector3), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_ADD_ASSIGN, "Vector3 &f(const Vector3 &in)",   asMETHODPR(Vector3,operator+=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_ADD_ASSIGN, "Vector3 &f(const float &in)",     asMETHODPR(Vector3,operator+=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_SUB_ASSIGN, "Vector3 &f(const Vector3 &in)",   asMETHODPR(Vector3,operator-=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_SUB_ASSIGN, "Vector3 &f(const float &in)",     asMETHODPR(Vector3,operator-=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_MUL_ASSIGN, "Vector3 &f(const Vector3 &in)",   asMETHODPR(Vector3,operator*=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_MUL_ASSIGN, "Vector3 &f(const float &in)",     asMETHODPR(Vector3,operator*=,(float),Vector3&), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_MUL_ASSIGN, "Vector3& operator @= ( const Vector3& rkVector f( const Vector3& rkVector )", asMETHOD(Ogre::Vector3, f), asCALL_THISCALL); assert(r>=0);
	
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_DIV_ASSIGN, "Vector3 &f(const Vector3 &in)",   asMETHODPR(Vector3,operator/=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Vector3", asBEHAVE_DIV_ASSIGN, "Vector3 &f(const float &in)",     asMETHODPR(Vector3,operator/=,(float),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	*/

}
#endif //ANGELSCRIPT
