#ifdef USE_MPLATFORM
#ifndef MPLATFORM_FD_H
#define MPLATFORM_FD_H

#include "Ogre.h"
#include "mplatform_base.h"
#include "rornet.h"
#include "SocketW.h"

class MPlatform_FD : public MPlatform_Base
{
	public:
		MPlatform_FD();
		~MPlatform_FD();

		bool connect();
		bool disconnect();
		bool update(Ogre::Vector3 pos, Ogre::Quaternion quat, mstat_t statinfo);

	private:
		mstat_t	mStatinfo;
		SWInetSocket mySocket;
		SWBaseSocket::SWBaseError error;

};

#endif
#endif
