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

#ifndef CHARACTER_H_
#define CHARACTER_H_

#include <Ogre.h>
#include <OgreVector3.h>
#include "Streamable.h"
#include "MovableText.h"
#include "rormemory.h"

class Water;
class Beam;
class Collisions;
class HeightFinder;
class MapControl;
class Network;
class MapEntity;

enum {CHARCMD_POSITION, CHARCMD_ATTACH};

class Character : public Streamable, public MemoryAllocatedObject
{
	friend class CharacterFactory;
	friend class Network;

public:
	Character(Ogre::Camera *cam, Collisions *c, Network *net, HeightFinder *h, Water *w, MapControl *m, Ogre::SceneManager *scm, int source=-1, unsigned int streamid=0, int colourNumber=0, bool remote=true);
	~Character();
	
	void setVisible(bool v);
	bool getVisible();

	void setOrientation(Ogre::Quaternion);
	Ogre::Quaternion getOrientation();
	Ogre::Vector3 getPosition();
	void setPosition(Ogre::Vector3 pos);
	float getAngle();
	
	void updateNetLabel();
	void updateCharacterColour();

	void move(Ogre::Vector3 v);
	int setBeamCoupling(bool enabled, Beam *truck=0);
	
	void update(float dt);
	Ogre::SceneNode *getSceneNode();

	void setAngle(float angle) { persoangle = angle; };
	void setColour(int number) { this->colourNumber = number; };

	static unsigned int characterCounter;
	void updateMapIcon();
	void setCollisions(Collisions *c);
	void setHFinder(HeightFinder *h);
	void setWater(Water *w);
	int getUID() { return source; };

	bool getPhysicsEnabled() { return physicsEnabled; };
	void setPhysicsEnabled(bool val) { physicsEnabled=val; };
	bool isRemote() { return remote; };

protected:
	Ogre::Vector3 position;
	Collisions *collisions;
	HeightFinder *hfinder;
	Ogre::Camera *mCamera;
	Water *water;
	Beam *beamCoupling;
	MapControl *map;
	Ogre::SceneManager *scm;
	Network *net;
	int source;
	unsigned int streamid;
	bool remote;
	bool physicsEnabled;
	int colourNumber;
	Ogre::MovableText *netMT;
	MapEntity *mapEnt;
	Ogre::String networkUsername;
	int networkAuthLevel;
	Ogre::String lastmode;
	
	Ogre::SceneNode *personode;
	Ogre::AnimationStateSet *persoanim;
	unsigned int myNumber;
	Ogre::String myName;
	float persoangle;
	float persospeed;
	float persovspeed;
	bool perso_canjump;
	Ogre::Vector3 lastpersopos;
	void setAnimationMode(Ogre::String mode, float time=0);

	typedef struct header_netdata_t
	{
		int command;
	} header_netdata_t;


	typedef struct pos_netdata_t
	{
		int command;
		Ogre::Vector3 pos;
		Ogre::Quaternion rot;
		Ogre::String animationMode;
		Ogre::Real animationTime;
	} pos_netdata_t;

	typedef struct attach_netdata_t
	{
		int command;
		bool enabled;
		int source_id;
		int stream_id;
		int position;
	} attach_netdata_t;

	// overloaded from Streamable:
	Ogre::Timer netTimer;
	int last_net_time;
	void sendStreamSetup();
	void sendStreamData();
	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);
	void setUID(int uid); 

	void updateNetLabelSize();
};

#endif
