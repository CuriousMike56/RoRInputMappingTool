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
#ifdef USE_SOCKETW

#include "network.h"
#include "NetworkStreamManager.h"
#include "RoRFrameListener.h"
#include "ColoredTextAreaOverlayElement.h"
#include "IngameConsole.h"
#include "CacheSystem.h"
#include "BeamFactory.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#ifdef USE_ANGELSCRIPT
#include "ScriptEngine.h"
#endif //ANGELSCRIPT
#include "turboprop.h"
#include "sha1.h"
#include "Settings.h"
#include "utils.h"
#include "language.h"
#include "errorutils.h"
#include "rormemory.h"
#include "gui_mp.h"

#ifdef USE_CRASHRPT
# include "crashrpt.h"
#endif

using namespace RoR; //CSHA1

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//#include <CFUserNotification.h>
#endif

Network *net_instance;

void *s_sendthreadstart(void* vid)
{
#ifdef USE_CRASHRPT
	if(SETTINGS.getSetting("NoCrashRpt").empty())
	{
		// add the crash handler for this thread
		CrThreadAutoInstallHelper cr_thread_install_helper;
		assert(cr_thread_install_helper.m_nInstallStatus==0);
	}
#endif //USE_CRASHRPT
	net_instance->sendthreadstart();
	return NULL;
}

void *s_receivethreadstart(void* vid)
{
#ifdef USE_CRASHRPT
	if(SETTINGS.getSetting("NoCrashRpt").empty())
	{
		// add the crash handler for this thread
		CrThreadAutoInstallHelper cr_thread_install_helper(0);
		assert(cr_thread_install_helper.m_nInstallStatus==0);
	}
#endif //USE_CRASHRPT
	net_instance->receivethreadstart();
	return NULL;
}

Timer Network::timer = Ogre::Timer();
unsigned int Network::myuid=0;

Network::Network(Beam **btrucks, std::string servername, long sport, RoRFrameListener *efl): lagDataClients(), initiated(false)
{
	// update factories network objects

	NetworkStreamManager::getSingleton().net = this;
	CharacterFactory::getSingleton().setNetwork(this);
	ChatSystemFactory::getSingleton().setNetwork(this);

	//
	memset(&server_settings, 0, sizeof(server_info_t));
	memset(&userdata, 0, sizeof(user_info_t));
	shutdown=false;
#ifdef USE_OPENAL
	ssm=SoundScriptManager::getSingleton();
#endif //USE_OPENAL
	mySname = servername;
	mySport = sport;
	mefl = efl;
	myauthlevel = AUTH_NONE;
	net_instance=this;
	nickname = "";
	trucks=btrucks;
	myuid=0;

	speed_time=0;
	speed_bytes_sent = speed_bytes_sent_tmp = speed_bytes_recv = speed_bytes_recv_tmp = 0;

	rconauthed=0;
	last_time=0;
	send_buffer=0;
	pthread_cond_init(&send_work_cv, NULL);
	pthread_mutex_init(&msgsend_mutex, NULL);
	pthread_mutex_init(&send_work_mutex, NULL);
	pthread_mutex_init(&dl_data_mutex, NULL);
	pthread_mutex_init(&clients_mutex, NULL);

	// reset client list
	pthread_mutex_lock(&clients_mutex);
	for (int i=0; i<MAX_PEERS; i++)
	{
		clients[i].used=false;
		memset(&clients[i].user, 0, sizeof(user_info_t));
	}
	pthread_mutex_unlock(&clients_mutex);

	// direct start, no vehicle required
	initiated = true;
}

Network::~Network()
{
	shutdown=true;
	pthread_mutex_destroy(&clients_mutex);
	pthread_mutex_destroy(&send_work_mutex);
	pthread_mutex_destroy(&dl_data_mutex);
	pthread_cond_destroy(&send_work_cv);
}

void Network::netFatalError(String errormsg, bool exitProgram)
{
	if(shutdown)
		return;

	SWBaseSocket::SWBaseError error;
	socket.set_timeout(1, 1000);
	socket.disconnect(&error);
	showError(_L("Network Connection Problem"), _L("Network fatal error: ")+errormsg);
	if(exitProgram)
		exit(124);
}

bool Network::connect()
{
	//here we go
	//begin setup with the caller thread
	SWBaseSocket::SWBaseError error;

	//manage the server socket
	socket.set_timeout(10, 10000); // 10 seconds timeout set as default
	socket.connect(mySport, mySname, &error);
	if (error!=SWBaseSocket::ok)
	{
		//this is an error!
		netFatalError("Establishing network session: ", false);
		return false;
	}
	//say hello to the server
	if (sendmessage(&socket, MSG2_HELLO, 0, (unsigned int)strlen(RORNET_VERSION), (char *)RORNET_VERSION))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending hello", false);
		return false;
	}

	header_t header;
	char buffer[MAX_MESSAGE_LENGTH];
	//get server version
	if (receivemessage(&socket, &header, buffer, 255))
	{
		//this is an error!
		netFatalError("Establishing network session: error getting server version", false);
		return false;
	}
	if(header.command != MSG2_HELLO)
	{
		netFatalError("Establishing network session: error getting server hello");
		return false;
	}

	// save server settings
	memcpy(&server_settings, buffer, sizeof(server_info_t));

	if (strncmp(server_settings.protocolversion, RORNET_VERSION, strlen(RORNET_VERSION)))
	{
		netFatalError("Establishing network session: wrong server version, you are using version '" + String(RORNET_VERSION) + "' and the server is using '"+String(server_settings.protocolversion)+"'");
		return false;
	}
	// first handshake done, increase the timeout, important!
	socket.set_timeout(0, 0);

	//send credencials
	nickname = SETTINGS.getSetting("Nickname");
	String nick = nickname;
	StringUtil::toLowerCase(nick);
	if (nick==String("pricorde") || nick==String("thomas"))
		nickname = "Anonymous";

	char pwbuffer[250];
	memset(pwbuffer, 0, 250);
	strncpy(pwbuffer, SETTINGS.getSetting("Server password").c_str(), 250);

	char sha1pwresult[250];
	memset(sha1pwresult, 0, 250);
	if(strnlen(pwbuffer, 250)>0)
	{
		CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)pwbuffer, strnlen(pwbuffer, 250));
		sha1.Final();
		sha1.ReportHash(sha1pwresult, CSHA1::REPORT_HEX_SHORT);
	}

	String usertoken = SETTINGS.getSetting("User Token");
	char usertokensha1result[250];
	memset(usertokensha1result, 0, 250);
	if(usertoken.size()>0)
	{
		CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)usertoken.c_str(), usertoken.size());
		sha1.Final();
		sha1.ReportHash(usertokensha1result, CSHA1::REPORT_HEX_SHORT);
	}

	// construct user credentials
	user_info_t c;
	memset(&c, 0, sizeof(user_info_t));
	strncpy(c.username, nickname.c_str(), 20);
	strncpy(c.serverpassword, sha1pwresult, 40);
	strncpy(c.usertoken, usertokensha1result, 40);
	strncpy(c.clientversion, ROR_VERSION_STRING, strnlen(ROR_VERSION_STRING, 25));
	strcpy(c.clientname, "RoR");
	String lang = SETTINGS.getSetting("Language Short");
	strncpy(c.language, lang.c_str(), std::min<int>(lang.size(), 10));
	String guid = SETTINGS.getSetting("GUID");
	strncpy(c.clientGUID, guid.c_str(), std::min<int>(guid.size(), 10));
	strcpy(c.sessiontype, "normal");
	if (sendmessage(&socket, MSG2_USER_INFO, 0, sizeof(user_info_t), (char*)&c))
	{
		//this is an error!
		netFatalError("Establishing network session: error sending user info", false);
		return false;
	}
	//now this is important, getting authorization
	if (receivemessage(&socket, &header, buffer, 255))
	{
		//this is an error!
		netFatalError("Establishing network session: error getting server authorization", false);
		return false;
	}
	if (header.command==MSG2_FULL)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, server has too many players", false);
		return false;
	}
	else if (header.command==MSG2_BANNED)
	{
		char tmp[512];
		memset(tmp, 0, 512);
		if(buffer && strnlen(buffer, 20)>0)
		{
			buffer[header.size]=0;
			sprintf(tmp, "Establishing network session: sorry, you are banned:\n%s", buffer);
			netFatalError(tmp);
		} else
		{
			netFatalError("Establishing network session: sorry, you are banned!", false);
		}

		return false;
	}
	else if (header.command==MSG2_WRONG_PW)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, wrong password!", false);
		return false;
	}
	else if (header.command==MSG2_WRONG_VER)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, wrong protocol version!", false);
		return false;
	}
	if (header.command!=MSG2_WELCOME)
	{
		//this is an error!
		netFatalError("Establishing network session: sorry, unknown server response", false);
		return false;
	}
	//okay keep our uid
	myuid = header.source;

	// we get our userdata back
	memcpy(&userdata, buffer, std::min<int>(sizeof(user_info_t), header.size));

	//start the handling threads
	pthread_create(&sendthread, NULL, s_sendthreadstart, (void*)(0));
	pthread_create(&receivethread, NULL, s_receivethreadstart, (void*)(0));

	return true;
}

Ogre::String Network::getNickname(bool colour)
{
	// returns coloured nickname
	int nickColour = 8;
	if(myauthlevel & AUTH_NONE)   nickColour = 8; // grey
	if(myauthlevel & AUTH_BOT )   nickColour = 4; // blue
	if(myauthlevel & AUTH_RANKED) nickColour = 2; // green
	if(myauthlevel & AUTH_MOD)    nickColour = 1; // red
	if(myauthlevel & AUTH_ADMIN)  nickColour = 1; // red

	String nick = ColoredTextAreaOverlayElement::StripColors(nickname);
	if(colour)
		return String("^") + StringConverter::toString(nickColour) + nick + String("^7");

	return nick;
}

int Network::sendMessageRaw(SWInetSocket *socket, char *buffer, unsigned int msgsize)
{
	//LogManager::getSingleton().logMessage("* sending raw message: " + StringConverter::toString(msgsize));

	pthread_mutex_lock(&msgsend_mutex); //we use a mutex because a chat message can be sent asynchronously
	SWBaseSocket::SWBaseError error;

	int rlen=0;
	while (rlen<(int)msgsize)
	{
		int sendnum=socket->send(buffer+rlen, msgsize-rlen, &error);
		if (sendnum<0)
		{
			LogManager::getSingleton().logMessage("NET send error: " + StringConverter::toString(sendnum));
			return -1;
		}
		rlen+=sendnum;
	}
	pthread_mutex_unlock(&msgsend_mutex);
	return 0;
}

int Network::sendmessage(SWInetSocket *socket, int type, unsigned int streamid, unsigned int len, char* content)
{
	pthread_mutex_lock(&msgsend_mutex); //we use a mutex because a chat message can be sent asynchronously
	SWBaseSocket::SWBaseError error;
	header_t head;
	memset(&head, 0, sizeof(header_t));
	head.command=type;
	head.source=myuid;
	head.size=len;
	head.streamid=streamid;
	int hlen=0;

	// construct buffer
	const int msgsize = sizeof(header_t) + len;

	if(msgsize >= MAX_MESSAGE_LENGTH)
	{
    	return -2;
	}

	char buffer[MAX_MESSAGE_LENGTH];
	memset(buffer, 0, MAX_MESSAGE_LENGTH);
	memcpy(buffer, (char *)&head, sizeof(header_t));
	memcpy(buffer+sizeof(header_t), content, len);

	int rlen=0;
	speed_bytes_sent_tmp += msgsize;
	while (rlen<(int)msgsize)
	{
		int sendnum=socket->send(buffer+rlen, msgsize-rlen, &error);
		if (sendnum<0)
		{
			LogManager::getSingleton().logMessage("NET send error: " + StringConverter::toString(sendnum));
			return -1;
		}
		rlen+=sendnum;
	}
	pthread_mutex_unlock(&msgsend_mutex);
	calcSpeed();
	return 0;
}

int Network::receivemessage(SWInetSocket *socket, header_t *head, char* content, unsigned int bufferlen)
{
	SWBaseSocket::SWBaseError error;

	char buffer[MAX_MESSAGE_LENGTH];
	//ensure that the buffer is clean after each received message!
	memset(buffer, 0, MAX_MESSAGE_LENGTH);

	int hlen=0;
	while (hlen<(int)sizeof(header_t))
	{
		int recvnum=socket->recv(buffer+hlen, sizeof(header_t)-hlen,&error);
		if (recvnum<0)
		{
			LogManager::getSingleton().logMessage("NET receive error 1: " + StringConverter::toString(recvnum));
			return -1;
		}
		hlen+=recvnum;
	}

	memcpy(head, buffer, sizeof(header_t));

	if(head->size >= MAX_MESSAGE_LENGTH)
	{
    	return -3;
	}

	if(head->size>0)
	{
		//read the rest
		while (hlen<(int)sizeof(header_t)+(int)head->size)
		{
			int recvnum=socket->recv(buffer+hlen, (head->size+sizeof(header_t))-hlen,&error);
			if (recvnum<0)
			{
				LogManager::getSingleton().logMessage("NET receive error 2: "+ StringConverter::toString(recvnum));
				return -1;
			}
			hlen+=recvnum;
		}
	}
	speed_bytes_recv_tmp += head->size + sizeof(header_t);

	memcpy(content, buffer+sizeof(header_t), bufferlen);
	calcSpeed();
	return 0;
}


int Network::getSpeedUp()
{
	return speed_bytes_sent;
}

int Network::getSpeedDown()
{
	return speed_bytes_recv;
}

void Network::calcSpeed()
{
	int t = timer.getMilliseconds();
	if(t - speed_time > 1000)
	{
		// we measure bytes / second
		speed_bytes_sent = speed_bytes_sent_tmp;
		speed_bytes_sent_tmp = 0;
		speed_bytes_recv = speed_bytes_recv_tmp;
		speed_bytes_recv_tmp = 0;
		speed_time = t;
	}
}

void Network::sendthreadstart()
{
	LogManager::getSingleton().logMessage("Sendthread starting");
	while (!shutdown)
	{
		// wait for data...
		NetworkStreamManager::getSingleton().sendStreams(this, &socket);

	}
}

void Network::disconnect()
{
	shutdown=true;
	sendmessage(&socket, MSG2_USER_LEAVE, 0, 0, 0);
	SWBaseSocket::SWBaseError error;
	socket.set_timeout(1, 1000);
	socket.disconnect(&error);
	LogManager::getSingleton().logMessage("Network error while disconnecting: ");
}


unsigned long Network::getNetTime()
{
	return timer.getMilliseconds();
}

void Network::receivethreadstart()
{
	header_t header;

	char *buffer=(char*)ror_malloc(MAX_MESSAGE_LENGTH);
	bool autoDl = (SETTINGS.getSetting("AutoDownload") == "Yes");
	std::deque < stream_reg_t > streamCreationResults;
	LogManager::getSingleton().logMessage("Receivethread starting");
	// unlimited timeout, important!

	// wait for beamfactory to be existant before doing anything
	// otherwise you can get runtime conditions
	while(!BeamFactory::getSingletonPtr())
	{
#ifndef WIN32
			sleep(1);
#else
			Sleep(1000);
#endif
	};

	socket.set_timeout(0,0);
	while (!shutdown)
	{
		//get one message
		int err=receivemessage(&socket, &header, buffer, MAX_MESSAGE_LENGTH);
		//LogManager::getSingleton().logMessage("received data: " + StringConverter::toString(header.command) + ", source: "+StringConverter::toString(header.source) + ":"+StringConverter::toString(header.streamid) + ", size: "+StringConverter::toString(header.size));
		if (err)
		{
			//this is an error!
			char errmsg[256];
			sprintf(errmsg, "Error %i while receiving data", err);
			netFatalError(errmsg);
			return;
		}

		// check for stream registration errors and notify the remote client
		if(BeamFactory::getSingletonPtr() && BeamFactory::getSingletonPtr()->getStreamRegistrationResults(&streamCreationResults))
		{
			while (!streamCreationResults.empty())
			{
				stream_reg_t r = streamCreationResults.front();
				stream_register_t reg = r.reg;
				sendmessage(&socket, MSG2_STREAM_REGISTER_RESULT, 0, sizeof(stream_register_t), (char *)&reg);
				streamCreationResults.pop_front();
			}
		}

		// TODO: produce new streamable classes when required
		if(header.command == MSG2_STREAM_REGISTER)
		{
			if(header.source == (int)myuid)
				// our own stream, ignore
				continue;
			stream_register_t *reg = (stream_register_t *)buffer;
			client_t *client = getClientInfo(header.source);
			int playerColour = 0;
			if(client) playerColour = client->user.colournum;

			String typeStr = "unkown";
			switch(reg->type)
			{
				case 0: typeStr="truck"; break;
				case 1: typeStr="character"; break;
				case 3: typeStr="chat"; break;
			};
			LogManager::getSingleton().logMessage(" * received stream registration: " + StringConverter::toString(header.source) + ": "+StringConverter::toString(header.streamid) + ", type: "+typeStr);

			if(reg->type == 0)
			{
				// truck
				BeamFactory::getSingleton().createRemote(header.source, header.streamid, reg, playerColour);
			} else if (reg->type == 1)
			{
				// person
				CharacterFactory::getSingleton().createRemote(header.source, header.streamid, reg, playerColour);
			} else if (reg->type == 2)
			{
				// previously AITRAFFIC, unused for now
			} else if (reg->type == 3)
			{
				// chat stream
				ChatSystemFactory::getSingleton().createRemote(header.source, header.streamid, reg, playerColour);
			}
			continue;
		}
		else if(header.command == MSG2_STREAM_REGISTER_RESULT)
		{
			stream_register_t *reg = (stream_register_t *)buffer;
			BeamFactory::getSingleton().addStreamRegistrationResults(header.source, reg);
			LogManager::getSingleton().logMessage(" * received stream registration result: " + StringConverter::toString(header.source) + ": "+StringConverter::toString(header.streamid));
		}
		else if(header.command == MSG2_CHAT && header.source == -1)
		{
			ChatSystem *cs = ChatSystemFactory::getSingleton().getFirstChatSystem();
			if(cs) cs->addReceivedPacket(header, buffer);
			continue;
		}
		else if(header.command == MSG2_NETQUALITY && header.source == -1)
		{
			if(header.size != sizeof(int))
				continue;
			int quality = *(int *)buffer;
			if(eflsingleton)
				eflsingleton->setNetQuality(quality);
			continue;
		}
		else if(header.command == MSG2_USER_LEAVE)
		{
			if(header.source == (int)myuid)
			{
				netFatalError("disconnected: remote side closed the connection", false);
				return;
			}

			// remove all things that belong to that user
			client_t *client = getClientInfo(header.source);
			if(client)
				client->used = false;

			// now remove all possible streams
			NetworkStreamManager::getSingleton().removeUser(header.source);
			continue;
		}
		else if(header.command == MSG2_USER_INFO || header.command == MSG2_USER_JOIN)
		{
			if(header.source == (int)myuid)
			{
				// we got data about ourself!
				memcpy(&userdata, buffer, sizeof(user_info_t));
				CharacterFactory::getSingleton().localUserAttributesChanged(myuid);
				// update our nickname
				nickname = String(userdata.username);
				// update auth status
				myauthlevel = userdata.authstatus;
			} else
			{
				user_info_t *cinfo = (user_info_t*) buffer;
				// data about someone else, try to update the array
				bool found = false; // whether to add a new client
				client_t *client = getClientInfo(header.source);
				if(client)
				{
					memcpy(&client->user, cinfo, sizeof(user_info_t));

					// inform the streamfactories of a attribute change
					CharacterFactory::getSingleton().netUserAttributesChanged(header.source, -1);
					BeamFactory::getSingleton().netUserAttributesChanged(header.source, -1);
					found = true;
				} else
				{
					// find a free entry
					pthread_mutex_lock(&clients_mutex);
					for (int i=0; i<MAX_PEERS; i++)
					{
						if (clients[i].used)
							continue;
						clients[i].used = true;
						memcpy(&clients[i].user, cinfo, sizeof(user_info_t));

						// inform the streamfactories of a attribute change
						CharacterFactory::getSingleton().netUserAttributesChanged(header.source, -1);
						BeamFactory::getSingleton().netUserAttributesChanged(header.source, -1);
						break;
					}
					pthread_mutex_unlock(&clients_mutex);
				}
			}
			continue;
		}
		//debugPacket("receive-1", &header, buffer);
		NetworkStreamManager::getSingleton().pushReceivedStreamMessage(header, buffer);
	}
}

int Network::getClientInfos(client_t c[MAX_PEERS])
{
	if(!initiated) return 1;
	pthread_mutex_lock(&clients_mutex);
	for(int i=0;i<MAX_PEERS;i++)
		c[i] = clients[i]; // copy the whole client list
	pthread_mutex_unlock(&clients_mutex);
	return 0;
}

client_t *Network::getClientInfo(unsigned int uid)
{
// this is a deadlock here
//	pthread_mutex_lock(&clients_mutex);
	client_t *c = 0;
	for (int i=0; i<MAX_PEERS; i++)
	{
		if (clients[i].user.uniqueid == uid)
			c = &clients[i];
	}
//	pthread_mutex_unlock(&clients_mutex);
	return c;
}

void Network::debugPacket(const char *name, header_t *header, char *buffer)
{
	char sha1result[250];
	memset(sha1result, 0, 250);
	if(buffer)
	{
		CSHA1 sha1;
		sha1.UpdateHash((uint8_t *)buffer, header->size);
		sha1.Final();
		sha1.ReportHash(sha1result, CSHA1::REPORT_HEX_SHORT);
	}

	char tmp[256]="";
	sprintf(tmp, "++ %s: %d:%d, %d, %d, hash: %s", name, header->source, header->streamid, header->command, header->size, sha1result);
	LogManager::getSingleton().logMessage(tmp);
	//String hex = hexdump(buffer, header->size);
	//LogManager::getSingleton().logMessage(hex);
}

#endif //SOCKETW

