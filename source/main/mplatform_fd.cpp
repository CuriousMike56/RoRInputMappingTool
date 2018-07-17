#ifdef USE_MPLATFORM
#include "mplatform_fd.h"

MPlatform_FD::MPlatform_FD()
{}

MPlatform_FD::~MPlatform_FD()
{}


bool MPlatform_FD::connect()
{
	
	if (mySocket.connect(12345, "127.0.0.1"))
	{
		Ogre::LogManager::getSingleton().logMessage("Connected to feedback server: ");
		return true;
	}
	else
	{
		Ogre::LogManager::getSingleton().logMessage("Unable to connect to feedback server: ");
		return false;
	}
}

bool MPlatform_FD::disconnect()
{
	return true;
}

bool MPlatform_FD::update(Ogre::Vector3 pos, Ogre::Quaternion quat, mstat_t statinfo)
{
	memset(&mStatinfo, 0, sizeof(mstat_t));

	/* get global position	*/
	float x = pos.x;
	float y = pos.y;
	float z = pos.z;

	/* get orientation */
	float head  = 0.0f;
	float pitch = 0.0f;
	float roll  = 0.0f;

	/* get misc info */
	float throttle	= 0.0f;
	float brake		= 0.0f;
	float clutch	= 0.0f;
	float speed		= 0.0f;
	float steering	= 0.0f;

	/* placebo values */

	mStatinfo.x = 1.23f;
	mStatinfo.y = 2.34f;
	mStatinfo.z = 3.45f;

	//mySocket.send(buf, length, error);
	mySocket.send((char *)&mStatinfo, sizeof(mstat_t), &error);
//	if (error) {return false;}
	return true;
}
#endif
