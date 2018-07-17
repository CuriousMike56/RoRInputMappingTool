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

#ifndef ChatSystem_H_
#define ChatSystem_H_

#include <Ogre.h>
#include <OgreVector3.h>
#include "Streamable.h"
#include "MovableText.h"
#include "StreamableFactory.h"
#include "IngameConsole.h"
#include "rormemory.h"

class MapControl;
class Network;
class ChatSystemFactory;

class ChatSystem : public Streamable, public MemoryAllocatedObject
{
	friend class ChatSystemFactory;
	friend class Network;

public:
	ChatSystem(Network *net, int source=-1, unsigned int streamid=0, int colourNumber=0, bool remote=true);
	~ChatSystem();

	void sendChat(Ogre::UTFString chatline);
protected:
	Network *net;
	int source;
	int streamid;
	int colourNumber;
	bool remote;
	Ogre::UTFString username;
	Ogre::String mNickColour;
	void sendStreamSetup();
	void sendStreamData();
	void receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len);
};

class ChatSystemFactory : public StreamableFactory < ChatSystemFactory, ChatSystem >, public MemoryAllocatedObject
{
	friend class Network;
public:
	ChatSystemFactory(Network *net);
	~ChatSystemFactory();

	ChatSystem *createLocal(int playerColour);
	ChatSystem *createRemoteInstance(stream_reg_t *reg);

	void setNetwork(Network *net) { this->net = net; };
	ChatSystem *getFirstChatSystem();

protected:
	Network *net;
	// functions used by friends
	void netUserAttributesChanged(int source, int streamid) {};
	void localUserAttributesChanged(int newid) {};

	bool syncRemoteStreams();
};

#endif
