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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 6th of May 2010

#include "OverlayWrapper.h"
#include "Ogre.h"
#include "TruckHUD.h"
#include "language.h"
#include "utils.h"
#include "ColoredTextAreaOverlayElement.h"
#include "OgreFontManager.h"

using namespace std;
using namespace Ogre;


// OverlayWrapper class
OverlayWrapper::OverlayWrapper(Ogre::RenderWindow* win) : 
	win(win)
{
	timeUntilUnflash=-1;
	init();
}

OverlayWrapper::~OverlayWrapper()
{
}


void OverlayWrapper::resizePanel(OverlayElement *oe)
{
	oe->setHeight(oe->getHeight()*(Real)win->getWidth()/(Real)win->getHeight());
	oe->setTop(oe->getTop()*(Real)win->getWidth()/(Real)win->getHeight());
}

void OverlayWrapper::reposPanel(OverlayElement *oe)
{
	oe->setTop(oe->getTop()*(Real)win->getWidth()/(Real)win->getHeight());
}

void OverlayWrapper::placeNeedle(SceneNode *node, float x, float y, float len)
{/**(Real)win->getHeight()/(Real)win->getWidth()*/
	node->setPosition((x-640.0)/444.0, (512-y)/444.0, -2.0);
	node->setScale(0.0025, 0.007*len, 0.007);
}

Ogre::Overlay *OverlayWrapper::loadOverlay(Ogre::String name)
{
	return OverlayManager::getSingleton().getByName(name);
}


Ogre::OverlayElement *OverlayWrapper::loadOverlayElement(Ogre::String name)
{
	return OverlayManager::getSingleton().getOverlayElement(name);
}

int OverlayWrapper::init()
{
	fpsLineStream = netLineStream = netlagLineStream = 0;

	directionOverlay = loadOverlay("tracks/DirectionArrow");
	directionArrowText = (TextAreaOverlayElement*)loadOverlayElement("tracks/DirectionArrow/Text");
	directionArrowDistance = (TextAreaOverlayElement*)loadOverlayElement("tracks/DirectionArrow/Distance");
	mouseOverlay = loadOverlay("tracks/MouseOverlay");
#ifdef HAS_EDITOR
	truckeditorOverlay = loadOverlay("tracks/TruckEditorOverlay");
#endif
	mDebugOverlay = loadOverlay("Core/DebugOverlay");
	mTimingDebugOverlay = loadOverlay("tracks/DebugBeamTiming");
	mTimingDebugOverlay->hide();


	machinedashboardOverlay = loadOverlay("tracks/MachineDashboardOverlay");
	airdashboardOverlay = loadOverlay("tracks/AirDashboardOverlay");
	airneedlesOverlay = loadOverlay("tracks/AirNeedlesOverlay");
	boatdashboardOverlay = loadOverlay("tracks/BoatDashboardOverlay");
	boatneedlesOverlay = loadOverlay("tracks/BoatNeedlesOverlay");
	dashboardOverlay = loadOverlay("tracks/DashboardOverlay");
	needlesOverlay = loadOverlay("tracks/NeedlesOverlay");
	needlesMaskOverlay = loadOverlay("tracks/NeedlesMaskOverlay");


	//adjust dashboard size for screen ratio
	resizePanel(loadOverlayElement("tracks/pressureo"));
	resizePanel(loadOverlayElement("tracks/pressureneedle"));
	MaterialPtr m = MaterialManager::getSingleton().getByName("tracks/pressureneedle_mat");
	if(!m.isNull())
		pressuretexture=m->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/speedo"));
	resizePanel(loadOverlayElement("tracks/tacho"));
	resizePanel(loadOverlayElement("tracks/anglo"));
	resizePanel(loadOverlayElement("tracks/instructions"));
	resizePanel(loadOverlayElement("tracks/machineinstructions"));
	resizePanel(loadOverlayElement("tracks/dashbar"));
	resizePanel(loadOverlayElement("tracks/dashfiller")	);
	resizePanel(loadOverlayElement("tracks/helppanel"));
	resizePanel(loadOverlayElement("tracks/machinehelppanel"));

	resizePanel(igno=loadOverlayElement("tracks/ign"));
	resizePanel(batto=loadOverlayElement("tracks/batt"));
	resizePanel(pbrakeo=loadOverlayElement("tracks/pbrake"));
	resizePanel(lockedo=loadOverlayElement("tracks/locked"));
	resizePanel(securedo=loadOverlayElement("tracks/secured"));
	resizePanel(lopresso=loadOverlayElement("tracks/lopress"));
	resizePanel(clutcho=loadOverlayElement("tracks/clutch"));
	resizePanel(lightso=loadOverlayElement("tracks/lights"));

	mouseElement = loadOverlayElement("mouse/pointer");

	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/machinedashbar"));
	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/machinedashfiller"));

	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/airdashbar"));
	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/airdashfiller"));
	
	OverlayElement* tempoe;
	resizePanel(tempoe=OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack1"));

	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack2"));
	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack3"));
	resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack4"));

	resizePanel(thro1=loadOverlayElement("tracks/thrust1"));
	resizePanel(thro2=loadOverlayElement("tracks/thrust2"));
	resizePanel(thro3=loadOverlayElement("tracks/thrust3"));
	resizePanel(thro4=loadOverlayElement("tracks/thrust4"));

	thrtop = 1.0f + tempoe->getTop() + thro1->getHeight() * 0.5f;
	thrheight = tempoe->getHeight() - thro1->getHeight() * 2.0f;
	throffset = thro1->getHeight() * 0.5f;

	engfireo1=loadOverlayElement("tracks/engfire1");
	engfireo2=loadOverlayElement("tracks/engfire2");
	engfireo3=loadOverlayElement("tracks/engfire3");
	engfireo4=loadOverlayElement("tracks/engfire4");
	engstarto1=loadOverlayElement("tracks/engstart1");
	engstarto2=loadOverlayElement("tracks/engstart2");
	engstarto3=loadOverlayElement("tracks/engstart3");
	engstarto4=loadOverlayElement("tracks/engstart4");
	resizePanel(loadOverlayElement("tracks/airrpm1"));
	resizePanel(loadOverlayElement("tracks/airrpm2"));
	resizePanel(loadOverlayElement("tracks/airrpm3"));
	resizePanel(loadOverlayElement("tracks/airrpm4"));
	resizePanel(loadOverlayElement("tracks/airpitch1"));
	resizePanel(loadOverlayElement("tracks/airpitch2"));
	resizePanel(loadOverlayElement("tracks/airpitch3"));
	resizePanel(loadOverlayElement("tracks/airpitch4"));
	resizePanel(loadOverlayElement("tracks/airtorque1"));
	resizePanel(loadOverlayElement("tracks/airtorque2"));
	resizePanel(loadOverlayElement("tracks/airtorque3"));
	resizePanel(loadOverlayElement("tracks/airtorque4"));
	resizePanel(loadOverlayElement("tracks/airspeed"));
	resizePanel(loadOverlayElement("tracks/vvi"));
	resizePanel(loadOverlayElement("tracks/altimeter"));
	resizePanel(loadOverlayElement("tracks/altimeter_val"));
	alt_value_taoe=(TextAreaOverlayElement*)loadOverlayElement("tracks/altimeter_val");
	boat_depth_value_taoe=(TextAreaOverlayElement*)loadOverlayElement("tracks/boatdepthmeter_val");
	resizePanel(loadOverlayElement("tracks/adi-tape"));
	resizePanel(loadOverlayElement("tracks/adi"));
	resizePanel(loadOverlayElement("tracks/adi-bugs"));
	adibugstexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/adi-bugs")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	aditapetexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/adi-tape")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/aoa"));
	resizePanel(loadOverlayElement("tracks/hsi"));
	resizePanel(loadOverlayElement("tracks/hsi-rose"));
	resizePanel(loadOverlayElement("tracks/hsi-bug"));
	resizePanel(loadOverlayElement("tracks/hsi-v"));
	resizePanel(loadOverlayElement("tracks/hsi-h"));
	hsirosetexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-rose")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	hsibugtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-bug")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	hsivtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-v")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	hsihtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-h")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	//autopilot
	reposPanel(loadOverlayElement("tracks/ap_hdg_pack"));
	reposPanel(loadOverlayElement("tracks/ap_wlv_but"));
	reposPanel(loadOverlayElement("tracks/ap_nav_but"));
	reposPanel(loadOverlayElement("tracks/ap_alt_pack"));
	reposPanel(loadOverlayElement("tracks/ap_vs_pack"));
	reposPanel(loadOverlayElement("tracks/ap_ias_pack"));
	reposPanel(loadOverlayElement("tracks/ap_gpws_but"));
	reposPanel(loadOverlayElement("tracks/ap_brks_but"));

	//boat
	resizePanel(loadOverlayElement("tracks/boatdashbar"));
	resizePanel(loadOverlayElement("tracks/boatdashfiller"));
	resizePanel(loadOverlayElement("tracks/boatthrusttrack1"));
	resizePanel(loadOverlayElement("tracks/boatthrusttrack2"));

	//resizePanel(boatmapo=loadOverlayElement("tracks/boatmap"));
	//resizePanel(boatmapdot=loadOverlayElement("tracks/boatreddot"));

	resizePanel(bthro1=loadOverlayElement("tracks/boatthrust1"));
	resizePanel(bthro2=loadOverlayElement("tracks/boatthrust2"));

	resizePanel(loadOverlayElement("tracks/boatspeed"));
	resizePanel(loadOverlayElement("tracks/boatsteer"));
	resizePanel(loadOverlayElement("tracks/boatspeedneedle"));
	resizePanel(loadOverlayElement("tracks/boatsteer/fg"));
	boatspeedtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/boatspeedneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	boatsteertexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/boatsteer/fg_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);


	//adjust dashboard mask
	//pitch mask
	OverlayElement *pmask=loadOverlayElement("tracks/pitchmask");
	pmask->setHeight(pmask->getHeight()*(Real)win->getWidth()/(Real)win->getHeight());
	pmask->setTop(pmask->getTop()*(Real)win->getWidth()/(Real)win->getHeight());
	//roll mask
	OverlayElement *rmask=loadOverlayElement("tracks/rollmask");
	rmask->setHeight(rmask->getHeight()*(Real)win->getWidth()/(Real)win->getHeight());
	rmask->setTop(rmask->getTop()*(Real)win->getWidth()/(Real)win->getHeight());

	//prepare needles
	resizePanel(loadOverlayElement("tracks/speedoneedle"));
	speedotexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/speedoneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/tachoneedle"));
	tachotexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/tachoneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/rollneedle"));
	rolltexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/rollneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/pitchneedle"));
	pitchtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/pitchneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/rollcorneedle"));
	rollcortexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/rollcorneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/turboneedle"));
	turbotexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/turboneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/airspeedneedle"));
	airspeedtexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airspeedneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/altimeterneedle"));
	altimetertexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/altimeterneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/vvineedle"));
	vvitexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/vvineedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/aoaneedle"));
	aoatexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/aoaneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);


	resizePanel(loadOverlayElement("tracks/airrpm1needle"));
	airrpm1texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airrpm2needle"));
	airrpm2texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airrpm3needle"));
	airrpm3texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airrpm4needle"));
	airrpm4texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/airpitch1needle"));
	airpitch1texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airpitch2needle"));
	airpitch2texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airpitch3needle"));
	airpitch3texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airpitch4needle"));
	airpitch4texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	resizePanel(loadOverlayElement("tracks/airtorque1needle"));
	airtorque1texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airtorque2needle"));
	airtorque2texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airtorque3needle"));
	airtorque3texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	resizePanel(loadOverlayElement("tracks/airtorque4needle"));
	airtorque4texture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
	//		speedotexture=((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/speedoneedle")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

	//        Entity *ecs=scm->createEntity("speed_needle", "needle.mesh");
	//        speed_node=scm->getRootSceneNode()->createChildSceneNode();
	//        speed_node->attachObject(ecs);
	//		needlesOverlay->add3D(speed_node);
	//		placeNeedle(win, speed_node, 1189, 936, 1.0);
	//tacho
	//        Entity *ect=scm->createEntity("tacho_needle", "needle.mesh");
	//        tach_node=scm->getRootSceneNode()->createChildSceneNode();
	//        tach_node->attachObject(ect);
	//		needlesOverlay->add3D(tach_node);
	//		placeNeedle(win, tach_node, 1011, 935, 1.0);
	//		//pitch
	//        Entity *ecp=scm->createEntity("pitch_needle", "needle.mesh");
	//		ecp->setMaterialName("tracks/whiteneedle");
	//        pitch_node=scm->getRootSceneNode()->createChildSceneNode();
	//        pitch_node->attachObject(ecp);
	//		needlesOverlay->add3D(pitch_node);
	//		placeNeedle(win, pitch_node, 876, 1014, 1.0);
	//roll
	//        Entity *ecr=scm->createEntity("roll_needle", "needle.mesh");
	//		ecr->setMaterialName("tracks/whiteneedle");
	//        roll_node=scm->getRootSceneNode()->createChildSceneNode();
	//        roll_node->attachObject(ecr);
	//		needlesOverlay->add3D(roll_node);
	//		placeNeedle(win, roll_node, 876, 924, 1.0);

	//rollcorr
	//		Entity *ecrc=scm->createEntity("rollcorr_needle", "needle.mesh");
	//		ecrc->setMaterialName("tracks/stabneedle");
	//		rollcorr_node=scm->getRootSceneNode()->createChildSceneNode();
	//		rollcorr_node->attachObject(ecrc);
	//		needlesOverlay->add3D(rollcorr_node);
	//		placeNeedle(win, rollcorr_node, 876, 924, 0.8);


	guiGear=loadOverlayElement("tracks/Gear");
	guiGear3D=loadOverlayElement("tracks/3DGear");
	guiRoll=loadOverlayElement("tracks/rollmask");

	guiAuto[0]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGearR");
	guiAuto[1]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGearN");
	guiAuto[2]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGearD");
	guiAuto[3]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGear2");
	guiAuto[4]=(TextAreaOverlayElement*)loadOverlayElement("tracks/AGear1");

	guiAuto3D[0]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearR");
	guiAuto3D[1]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearN");
	guiAuto3D[2]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearD");
	guiAuto3D[3]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGear2");
	guiAuto3D[4]=(TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGear1");

	guipedclutch=loadOverlayElement("tracks/pedalclutch");
	guipedbrake=loadOverlayElement("tracks/pedalbrake");
	guipedacc=loadOverlayElement("tracks/pedalacc");

	flashOverlay = loadOverlay("tracks/FlashMessage");

	OverlayContainer *flashPanel = static_cast<OverlayContainer*>(OverlayManager::getSingleton().createOverlayElement("Panel", "tracks/FlashMessage/Panel"));

	// create flash message
	flashMessageTE = static_cast<ColoredTextAreaOverlayElement*>(OverlayManager::getSingleton().createOverlayElement("ColoredTextArea", "tracks/Message2"));
	flashMessageTE->setMetricsMode(Ogre::GMM_RELATIVE);
	flashMessageTE->setPosition(0.1f, 0.1f);
	flashMessageTE->setDimensions(0.8f, 0.3f);
	flashMessageTE->setFontName("Cyberbit");
	flashMessageTE->setCharHeight(0.05f);
	flashMessageTE->setCaption("/");
	flashMessageTE->setColourTop(ColourValue(1.0f,0.6f, 0.0f));
	flashMessageTE->setColourBottom(ColourValue(0.8f,0.4f, 0.0f));
	flashPanel->addChild(flashMessageTE);
	flashOverlay->add2D(flashPanel);


	pressureOverlay = loadOverlay("tracks/PressureOverlay");
	pressureNeedleOverlay = loadOverlay("tracks/PressureNeedleOverlay");
	editorOverlay = loadOverlay("tracks/EditorOverlay");

	racing = loadOverlay("tracks/Racing");
	laptimemin = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimemin");
	laptimes = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimes");
	laptimems = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimems");
	lasttime = (TextAreaOverlayElement*)loadOverlayElement("tracks/LastTime");
	lasttime->setCaption("");
	
	editor_pos = loadOverlayElement("tracks/EditorPosition");
	editor_angles = loadOverlayElement("tracks/EditorAngles");
	editor_object = loadOverlayElement("tracks/EditorObject");
	

	truckhud = new TruckHUD();
	truckhud->show(false);


	return 0;
}

void OverlayWrapper::update(float dt)
{
	if (timeUntilUnflash > 0)
	{
		timeUntilUnflash-=dt;
	} else if (flashOverlay->isVisible())
	{
		flashOverlay->hide();
	}
}


void OverlayWrapper::flashMessage(Ogre::String txt, float time, float charHeight)
{
	flashMessage(txt.c_str(), time, charHeight);
}

void OverlayWrapper::flashMessage(const char* txt, float time, float charHeight)
{
	if(!txt || time < 0)
	{
		// hide
		flashMessageTE->setCaption("");
		flashOverlay->hide();
		timeUntilUnflash=0;
		return;
	}
	try
	{
		// catch any non UTF chars
		flashMessageTE->setCaption(txt);
	} catch(...)
	{
	}

	if(charHeight != -1)
		flashMessageTE->setCharHeight(charHeight);
	else
		// set original height
		flashMessageTE->setCharHeight(0.05f);

	flashOverlay->show();
	timeUntilUnflash=time;
}

void OverlayWrapper::flashMessage()
{
	flashOverlay->show();
	timeUntilUnflash=1;
}

void OverlayWrapper::hideFlashMessage()
{
	flashMessage(0, -1, -1);
	flashOverlay->hide();
}





void OverlayWrapper::showDebugOverlay(int mode)
{
	if (!mDebugOverlay || !mTimingDebugOverlay) return;
	if (mode > 0)
	{
		mDebugOverlay->show();
		if(mode > 1)
			mTimingDebugOverlay->show();
		else
			mTimingDebugOverlay->hide();
	}
	else
	{
		mDebugOverlay->hide();
		mTimingDebugOverlay->hide();
	}
}


void OverlayWrapper::showPressureOverlay(bool show)
{
	if (pressureOverlay)
	{
		if (show)
		{
			pressureOverlay->show();
			pressureNeedleOverlay->show();
		}
		else
		{
			pressureOverlay->hide();
			pressureNeedleOverlay->hide();
		}
	}
}

void OverlayWrapper::showEditorOverlay(bool show)
{
	if (editorOverlay)
	{
		if (show)
		{
			editorOverlay->show();
		}
		else
		{
			editorOverlay->hide();
		}
	}
}

void OverlayWrapper::showDashboardOverlays(bool show, int mode)
{
	if (needlesOverlay && dashboardOverlay)
	{
		if (show)
		{
			if (mode==TRUCK)
			{
				needlesMaskOverlay->show();
				needlesOverlay->show();
				dashboardOverlay->show();
			};
			if (mode==AIRPLANE)
			{
				airneedlesOverlay->show();
				airdashboardOverlay->show();
				//mouseOverlay->show();
			};
			if (mode==BOAT)
			{
				boatneedlesOverlay->show();
				boatdashboardOverlay->show();
			};
			if (mode==BOAT)
			{
				boatneedlesOverlay->show();
				boatdashboardOverlay->show();
			};
			if (mode==MACHINE)
			{
				machinedashboardOverlay->show();
			};
		}
		else
		{
			machinedashboardOverlay->hide();
			needlesMaskOverlay->hide();
			needlesOverlay->hide();
			dashboardOverlay->hide();
			//for the airplane
			airneedlesOverlay->hide();
			airdashboardOverlay->hide();
			//mouseOverlay->hide();
			boatneedlesOverlay->hide();
			boatdashboardOverlay->hide();
		}
	}
}

void OverlayWrapper::updateStats(bool detailed)
{
	static String currFps = _L("Current FPS: ");
	static String avgFps = _L("Average FPS: ");
	static String bestFps = _L("Best FPS: ");
	static String worstFps = _L("Worst FPS: ");
	static String tris = _L("Triangle Count: ");
	const RenderTarget::FrameStats& stats = win->getStatistics();

	// update stats when necessary
	try
	{
		OverlayElement* guiAvg = OverlayManager::getSingleton().getOverlayElement("Core/AverageFps");
		OverlayElement* guiCurr = OverlayManager::getSingleton().getOverlayElement("Core/CurrFps");
		OverlayElement* guiBest = OverlayManager::getSingleton().getOverlayElement("Core/BestFps");
		OverlayElement* guiWorst = OverlayManager::getSingleton().getOverlayElement("Core/WorstFps");


		guiAvg->setCaption(avgFps + StringConverter::toString(stats.avgFPS));
		guiCurr->setCaption(currFps + StringConverter::toString(stats.lastFPS));
		guiBest->setCaption(bestFps + StringConverter::toString(stats.bestFPS) + " " + StringConverter::toString(stats.bestFrameTime)+" ms");
		guiWorst->setCaption(worstFps + StringConverter::toString(stats.worstFPS) + " " + StringConverter::toString(stats.worstFrameTime)+" ms");

		OverlayElement* guiTris = OverlayManager::getSingleton().getOverlayElement("Core/NumTris");
		String triss = tris + StringConverter::toString(stats.triangleCount);
		if(stats.triangleCount > 1000000)
			triss = tris + StringConverter::toString(stats.triangleCount/1000000.0f) + " M";
		else if(stats.triangleCount > 1000)
			triss = tris + StringConverter::toString(stats.triangleCount/1000.0f) + " k";
		guiTris->setCaption(triss);

		OverlayElement* guiDbg = OverlayManager::getSingleton().getOverlayElement("Core/DebugText");
		String debugText = "";
		// TODO: TOFIX
		/*
		for(int t=0;t<free_truck;t++)
		{
			if(!trucks[t]) continue;
			if(!trucks[t]->debugText.empty())
				debugText += StringConverter::toString(t) + ": " + trucks[t]->debugText + "\n";
		}
		guiDbg->setCaption(debugText);
		*/

		// create some memory texts
		String memoryText;
		if(TextureManager::getSingleton().getMemoryUsage() > 1)
			memoryText += "Textures: " + formatBytes(TextureManager::getSingleton().getMemoryUsage()) + " / " + formatBytes(TextureManager::getSingleton().getMemoryBudget()) + "\n";
		if(CompositorManager::getSingleton().getMemoryUsage() > 1)
			memoryText += "Compositors: " + formatBytes(CompositorManager::getSingleton().getMemoryUsage()) + " / " + formatBytes(CompositorManager::getSingleton().getMemoryBudget()) + "\n";
		if(FontManager::getSingleton().getMemoryUsage() > 1)
			memoryText += "Fonts: " + formatBytes(FontManager::getSingleton().getMemoryUsage()) + " / " + formatBytes(FontManager::getSingleton().getMemoryBudget()) + "\n";
		if(GpuProgramManager::getSingleton().getMemoryUsage() > 1)
			memoryText += "GPU Program: " + formatBytes(GpuProgramManager::getSingleton().getMemoryUsage()) + " / " + formatBytes(GpuProgramManager::getSingleton().getMemoryBudget()) + "\n";
		if(HighLevelGpuProgramManager ::getSingleton().getMemoryUsage() >1)
			memoryText += "HL GPU Program: " + formatBytes(HighLevelGpuProgramManager ::getSingleton().getMemoryUsage()) + " / " + formatBytes(HighLevelGpuProgramManager ::getSingleton().getMemoryBudget()) + "\n";
		if(MaterialManager::getSingleton().getMemoryUsage() > 1)
			memoryText += "Materials: " + formatBytes(MaterialManager::getSingleton().getMemoryUsage()) + " / " + formatBytes(MaterialManager::getSingleton().getMemoryBudget()) + "\n";
		if(MeshManager::getSingleton().getMemoryUsage() > 1)
			memoryText += "Meshes: " + formatBytes(MeshManager::getSingleton().getMemoryUsage()) + " / " + formatBytes(MeshManager::getSingleton().getMemoryBudget()) + "\n";
		if(SkeletonManager::getSingleton().getMemoryUsage() > 1)
			memoryText += "Skeletons: " + formatBytes(SkeletonManager::getSingleton().getMemoryUsage()) + " / " + formatBytes(SkeletonManager::getSingleton().getMemoryBudget()) + "\n";
		if(MaterialManager::getSingleton().getMemoryUsage() > 1)
			memoryText += "Materials: " + formatBytes(MaterialManager::getSingleton().getMemoryUsage()) + " / " + formatBytes(MaterialManager::getSingleton().getMemoryBudget()) + "\n";
		memoryText += "\n";
		memoryText += "RoR: " + formatBytes(MemoryWrapper::getMemoryAllocated()) + "\n";

		OverlayElement* memoryDbg = OverlayManager::getSingleton().getOverlayElement("Core/MemoryText");
		memoryDbg->setCaption(memoryText);



		float sumMem = TextureManager::getSingleton().getMemoryUsage() + CompositorManager::getSingleton().getMemoryUsage() + FontManager::getSingleton().getMemoryUsage() + GpuProgramManager::getSingleton().getMemoryUsage() + HighLevelGpuProgramManager ::getSingleton().getMemoryUsage() + MaterialManager::getSingleton().getMemoryUsage() + MeshManager::getSingleton().getMemoryUsage() + SkeletonManager::getSingleton().getMemoryUsage() + MaterialManager::getSingleton().getMemoryUsage();
		String sumMemoryText = "Memory (Ogre): " + formatBytes(sumMem) + "\n";

		OverlayElement* memorySumDbg = OverlayManager::getSingleton().getOverlayElement("Core/CurrMemory");
		memorySumDbg->setCaption(sumMemoryText);


		if(detailed)
		{
			static int framecounter = 0;
			static int fpscount=0;
			static ColourValue cr = ColourValue(0.3f,0.3f,1.0f);
			static ColourValue cg = ColourValue(0.3f,1.0f,0.3f);
			static ColourValue gr = ColourValue(0.8f,0.8f,0.8f);
			// we view the extended stats, so draw the FPS graph :)
			if(!fpsLineStream)
			{
				float graphHeight = 0.1f;
				float border = graphHeight * 0.1f;
				float posY = 0;
				MaterialPtr backMat = MaterialManager::getSingleton().create("linestream_white", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				TextureUnitState *t = backMat->getTechnique(0)->getPass(0)->createTextureUnitState();
				t->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_CURRENT, ColourValue::White);
				t->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_CURRENT, 0.3);
				backMat->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);

				backMat = MaterialManager::getSingleton().create("linestream_lines", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				t = backMat->getTechnique(0)->getPass(0)->createTextureUnitState();

				fpsLineStream = dynamic_cast<Ogre::LineStreamOverlayElement *>(OverlayManager::getSingleton().createOverlayElement("LineStream", "FPSCurveLineStream"));
				fpsLineStream->_setPosition(0.7f, posY);
				fpsLineStream->_setDimensions(0.3f, graphHeight);
				fpsLineStream->setMaterialName("linestream_lines");
				fpsLineStream->setNumberOfSamplesForTrace(400);
				fpsLineStream->setNumberOfTraces(2);
				fpsLineStream->setTitle(gr, "Frame stats");

				fpsLineStream->setMoveMode(0);
				fpsLineStream->createVertexBuffer();
				mDebugOverlay->add2D(fpsLineStream);
				OverlayContainer *debugLineStreamPanel = (OverlayContainer *) (OverlayManager::getSingleton().createOverlayElement("Panel","debugLineStreamPanel"));
				debugLineStreamPanel->_setPosition(0.7f, posY);
				debugLineStreamPanel->_setDimensions(0.4f, graphHeight);
				debugLineStreamPanel->setMaterialName("linestream_white");
				mDebugOverlay->add2D(debugLineStreamPanel);
				posY += graphHeight + border;

				fpsLineStream->setTraceInfo(0, cr, "FPS");
				fpsLineStream->setTraceInfo(1, cg, "Physic Frames");

				// TODO: TOFIX
				/*
				if(netmode)
				{
					// net traffic
					netLineStream = dynamic_cast<Ogre::LineStreamOverlayElement *>(OverlayManager::getSingleton().createOverlayElement("LineStream", "NETCurveLineStream"));
					netLineStream->_setPosition(0.7f, posY);
					netLineStream->_setDimensions(0.3f, graphHeight);
					netLineStream->setMaterialName("linestream_lines");
					netLineStream->setNumberOfSamplesForTrace(400);
					netLineStream->setNumberOfTraces(2);
					netLineStream->setMoveMode(0);
					netLineStream->createVertexBuffer();
					netLineStream->setTitle(gr, "Network Traffic");
					mDebugOverlay->add2D(netLineStream);
					OverlayContainer *debugLineStreamPanel = (OverlayContainer *) (OverlayManager::getSingleton().createOverlayElement("Panel","debugLineStreamPanel2"));
					debugLineStreamPanel->_setPosition(0.7f, posY);
					debugLineStreamPanel->_setDimensions(0.4f, graphHeight);
					debugLineStreamPanel->setMaterialName("linestream_white");
					mDebugOverlay->add2D(debugLineStreamPanel);
					posY += graphHeight + border;

					// net lag
					netlagLineStream = dynamic_cast<Ogre::LineStreamOverlayElement *>(OverlayManager::getSingleton().createOverlayElement("LineStream", "NETLAGCurveLineStream"));
					netlagLineStream->_setPosition(0.7f, posY);
					netlagLineStream->_setDimensions(0.3f, graphHeight);
					netlagLineStream->setMaterialName("linestream_lines");
					netlagLineStream->setNumberOfSamplesForTrace(400);
					netlagLineStream->setNumberOfTraces(10);
					netlagLineStream->setMoveMode(0);
					netlagLineStream->createVertexBuffer();
					netlagLineStream->setTitle(gr, "Network Lag");
					mDebugOverlay->add2D(netlagLineStream);
					debugLineStreamPanel = (OverlayContainer *) (OverlayManager::getSingleton().createOverlayElement("Panel","debugLineStreamPanel3"));
					debugLineStreamPanel->_setPosition(0.7f, posY);
					debugLineStreamPanel->_setDimensions(0.4f, graphHeight);
					debugLineStreamPanel->setMaterialName("linestream_white");
					mDebugOverlay->add2D(debugLineStreamPanel);
				}

				if(netmode && netLineStream)
				{
					netLineStream->setTraceInfo(0, cr, "Traffic Up");
					netLineStream->setTraceInfo(1, cg, "Traffic Down");
					netlagLineStream->setTraceInfo(0, cr, "Average");
					for(int i=1;i<10;i++)
						netlagLineStream->setTraceInfo(i, ColourValue(0.8f,0.8f,0.8f), "");
				}
				*/

			}
			if(!fpsLineStream)
				return;
			if(framecounter > 5)
			{
				char tmp[10]="";
				float fps = fpscount/6.0f;
				sprintf(tmp, "%0.0f", fps);

				fpsLineStream->setTraceInfo(0, cr, "FPS: " + String(tmp));
				//fpsLineStream->setTraceInfo(1, cg, "Physic Lag: " + StringConverter::toString(truckSteps));

				fpsLineStream->setTraceValue(0, fpscount/6.0f);
				//fpsLineStream->setTraceValue(1, truckSteps);
#ifdef USE_SOCKETW
				// TODO: TOFIX
				/*
				if(netmode && net && netLineStream)
				{
					// in kB/s not B/s
					memset(tmp, 0, 10);
					float speedUp = net->getSpeedUp()/1024.0f;
					sprintf(tmp, "%0.1fkB/s", speedUp);
					netLineStream->setTraceValue(0, speedUp);

					netLineStream->setTraceInfo(0, cr, "Traffic Up: "+String(tmp));

					float speedDown = net->getSpeedDown()/1024.0f;
					memset(tmp, 0, 10);
					sprintf(tmp, "%0.1fkB/s", speedDown);
					netLineStream->setTraceValue(1, speedDown);
					netLineStream->setTraceInfo(1, cg, "Traffic Down: "+String(tmp));

					netLineStream->moveForward();

#if 0
					// TODO: fix lag data
					std::map<int, float> &lag = net->getLagData();
					int c=0;
					float sum=0;
					for(std::map<int, float>::iterator it = lag.begin(); it!= lag.end(); it++)
					{
						int n = it->first;
						float lag = fabs(it->second);
						//LogManager::getSingleton().logMessage("lag("+StringConverter::toString(c)+")="+StringConverter::toString(lag));
						sum += lag;
						c++;
						netlagLineStream->setTraceValue(c, lag);
					}

					float av = sum / (float)(c);
					netlagLineStream->setTraceValue(0, av);
					memset(tmp, 0, 10);
					sprintf(tmp, "%0.0fms", av);

					netlagLineStream->setTraceInfo(0, cr, "Average:" + String(tmp));
					netlagLineStream->moveForward();
#endif //0
				}
				*/
#endif //SOCKETW
				fpsLineStream->moveForward();
				fpscount = 0;
				framecounter = 0;
			} else
			{
				fpscount += stats.lastFPS;
				framecounter++;
			}
		}
	}
	catch(...)
	{
		// ignore
	}
}