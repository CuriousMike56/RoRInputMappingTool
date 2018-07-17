/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009,2010 Pierre-Michel Ricordel
Copyright 2007,2008,2009,2010 Thomas Fischer

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

// created on 30th of April 2010 by Thomas Fischer

// macro that checks for the correct ogre version and aborts compilation if not correct
#if OGRE_VERSION < 0x010701
#error You need at least Ogre version 1.7.1, older versions are not supported
#endif

#ifndef RORPREREQUISITES_H__
#define RORPREREQUISITES_H__

class AeroEngine;
class Airfoil;
class Airbrake;
class Autopilot;
class Beam;
class BeamEngine;
class BeamThreadStats;
class Buoyance;
class CmdKeyInertia;
class Collisions;
class ColoredTextAreaOverlayElement;
class DustPool;
class Flexable;
class FlexAirfoil;
class FlexBody;
class FlexMesh;
class FlexObj;
class HeightFinder;
class MaterialFunctionMapper;
class MapTextureCreator;
class Mirrors;
class Network;
class OverlayWrapper;
class PointColDetector;
class PositionStorage;
class Rail;
class RailGroup;
class Replay;
class RigsOfRods;
class RoRFrameListener;
class Screwprop;
class Skidmark;
class Skin;
class SlideNode;
class SoundScriptInstance;
class SoundScriptManager;
class Turboprop;
class TruckHUD;
class Water;

namespace Ogre
{
	class MovableText;
};

#ifdef USE_LUA
class LuaSystem;
#endif //USE_LUA

#ifdef USE_SOCKETW
class SWBaseSocket;
#endif //USE_SOCKETW

// some platform fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

// forward typedefs for structs

// little helper macro that should prevent typos and increase readability
// "FWDCLSTRUCT(node)" will be "typedef struct node node_t"
#define FWDCLSTRUCT(x) typedef struct x x##_t


FWDCLSTRUCT(node);
FWDCLSTRUCT(beam);
FWDCLSTRUCT(shock);
FWDCLSTRUCT(collcab_rate);
FWDCLSTRUCT(soundsource);
FWDCLSTRUCT(contacter);
FWDCLSTRUCT(rigidifier);
FWDCLSTRUCT(wheel);
FWDCLSTRUCT(vwheel);
FWDCLSTRUCT(ropable);
FWDCLSTRUCT(wing);
FWDCLSTRUCT(command);
FWDCLSTRUCT(rotator);
FWDCLSTRUCT(flare);
FWDCLSTRUCT(prop);
FWDCLSTRUCT(rope);
FWDCLSTRUCT(exhaust);
FWDCLSTRUCT(cparticle);
FWDCLSTRUCT(debugtext);
FWDCLSTRUCT(rig);
FWDCLSTRUCT(collision_box);
FWDCLSTRUCT(tie);
FWDCLSTRUCT(hook);

FWDCLSTRUCT(ground_model);
FWDCLSTRUCT(client);
FWDCLSTRUCT(authorinfo);


#endif //RORPREREQUISITES_H__
