#ifdef USE_MPLATFORM
#include "mplatform_base.h"

MPlatform_Base::MPlatform_Base()
{}

MPlatform_Base::~MPlatform_Base()
{}

bool MPlatform_Base::connect()
{
	return false;
}

bool MPlatform_Base::disconnect()
{
	return false;
}

bool MPlatform_Base::update(Ogre::Vector3 pos, Ogre::Quaternion quat, mstat_t statinfo)
{
	return false;
}

bool MPlatform_Base::update(float posx, float posy, float posz, float roll, float pitch, float head, float acc_roll, float acc_pitch, float acc_head)
{
	return false;
}
#endif
