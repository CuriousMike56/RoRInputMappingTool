/* This class is the skeleton of motion platform control in RoR.
	Please derive your own class from this to make it work with your motion platform!

	Imre, Nagy Jr. (ror@nebulon.hu)
*/

#ifdef USE_MPLATFORM
#ifndef MPLATFORM_BASE_H
#define MPLATFORM_BASE_H

#include "Ogre.h"
#include "rornet.h"


typedef struct			// struct is used for motion platforms
{
	int time;

	float	x;			// absolute coordinates
	float	y;
	float	z;

	float	x_acc;		// accelerations regarding to different vectors
	float	y_acc;
	float	z_acc;

	float	head;		// orientations
	float	roll;
	float	pitch;

	float	head_acc;	// accelerations of orientations
	float	roll_acc;
	float	pitch_acc;

	float	steer;		// user inputs
	float	throttle;
	float	brake;
	float	clutch;

	float	speed;		// different stats
	float	rpm;
	int		gear;
	float	avg_friction;
} mstat_t;

class MPlatform_Base
{
	public:
		MPlatform_Base();
		virtual ~MPlatform_Base();

		virtual bool connect();		// initialize connection to motion platform 
		virtual bool disconnect();	// clean up connection with motion platform

		// update motion platform. returning false if cannot update, like sending buffer is full
		virtual bool update(Ogre::Vector3 pos, Ogre::Quaternion quat, mstat_t statinfo);
		virtual bool update(float posx, float posy, float posz, float roll, float pitch, float head, float roll_acc, float pitch_acc, float head_acc);


	private:
};

#endif
#endif
