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
#ifndef __TRUCKHUD_H__
#define __TRUCKHUD_H__

#define COMMANDS_VISIBLE 25

#include "Beam.h"
#include "OgreLineStreamOverlayElement.h"

class TruckHUD : public MemoryAllocatedObject
{
public:
	TruckHUD();
	~TruckHUD();
	bool update(float dt, Beam *truck, Ogre::SceneManager *sm, Ogre::Camera* mCamera, Ogre::RenderWindow* mWindow, bool visible=true);
	void show(bool value);
	bool isVisible();
  
protected:

	Ogre::LineStreamOverlayElement *torqueLineStream; //!< pointer to the linestream overlay, which is the element that renders the curve
	Ogre::String lastTorqueModel; //!< name of the last used torque model, needed to detect a change in the model
	Ogre::Real lastTorqueRatio; //!< last RPM ratio, used to clear the last torque peak

	/**
	 * creates the required overlays to display a torque curve
	 */
	void initTorqueOverlay();


private:
    static TruckHUD *myInstance;
	std::map<int, float> maxVelos;
	std::map<int, float> minVelos;
	std::map<int, float> avVelos;
	Ogre::Overlay *truckHUD;
	float updatetime;
	int width, border;
	void checkOverflow(Ogre::OverlayElement* e);
};


#endif
