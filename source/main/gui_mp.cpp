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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of September 2009

#ifdef USE_MYGUI
#ifdef USE_SOCKETW

#include "gui_mp.h"
#include "gui_manager.h"
#include "Settings.h"

#include "language.h"
#include "RoRFrameListener.h"
#include "network.h"
#include "PlayerColours.h"
#include "BeamFactory.h"

template<> GUI_Multiplayer * Singleton< GUI_Multiplayer >::ms_Singleton = 0;
GUI_Multiplayer* GUI_Multiplayer::getSingletonPtr(void)
{
	return ms_Singleton;
}
GUI_Multiplayer& GUI_Multiplayer::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

GUI_Multiplayer::GUI_Multiplayer(Network *_net, Ogre::Camera *cam) : net(_net), msgwin(0), mCamera(cam)
{
	lineheight=16;

	// tooltip window
	tooltipPanel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("PanelSmall", 0, 0, 200, 20,  MyGUI::Align::Default, "ToolTip");
	tooltipText = tooltipPanel->createWidget<MyGUI::StaticText>("StaticText", 4, 2, 200, 16,  MyGUI::Align::Default);
	tooltipText->setFontName("VeraMono");
	//tooltipPanel->setAlpha(0.9f);
	tooltipText->setFontHeight(16);
	tooltipPanel->setVisible(false);
	
	// message window
	msgwin = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCSX", 0, 0, 400, 300,  MyGUI::Align::Center, "Overlapped");
	msgwin->setCaption(_L("Player Information"));
	msgtext = msgwin->createWidget<MyGUI::Edit>("EditStretch", 0, 0, 400, 300,  MyGUI::Align::Default, "helptext");
	msgtext->setCaption("");
	msgtext->setEditWordWrap(true);
	msgtext->setEditStatic(true);
	msgwin->setVisible(false);


	// network quality warning
	netmsgwin = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("FlowContainer", 5, 30, 300, 40,  MyGUI::Align::Default, "Main");
	netmsgwin->setAlpha(0.8f);
	MyGUI::StaticImagePtr nimg = netmsgwin->createWidget<MyGUI::StaticImage>("StaticImage", 0, 0, 16, 16,  MyGUI::Align::Default, "Main");
	nimg->setImageTexture("error.png");
	netmsgtext = netmsgwin->createWidget<MyGUI::StaticText>("StaticText", 18, 2, 300, 40,  MyGUI::Align::Default, "helptext");
	netmsgtext->setCaption(_L("Slow  Network  Download"));
	netmsgtext->setFontName("VeraMoBd");
	netmsgtext->setTextColour(MyGUI::Colour::Red);
	netmsgtext->setFontHeight(lineheight);
	netmsgwin->setVisible(false);


	// now the main GUI
	MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
	int x=gui_area.width - 200, y=30;
	mpPanel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("FlowContainer", x, y, 200, gui_area.height,  MyGUI::Align::Default, "Main");
	mpPanel->setVisible(true);

	y=0;
	for(int i = 0; i < MAX_PEERS + 1; i++) // plus 1 for local entry
	{
		x=100; // space for icons
		player_row_t *row = &player_rows[i];
		row->playername = mpPanel->createWidget<MyGUI::StaticText>("StaticText", x, y+1, 200, lineheight,  MyGUI::Align::Default, "Main");
		row->playername->setCaption("Player " + StringConverter::toString(i));
		row->playername->setFontName("VeraMoBd");
		row->playername->setUserString("tooltip", "user name");
		row->playername->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->playername->setNeedToolTip(true);
		row->playername->setVisible(false);
		row->playername->setFontHeight(lineheight);
		row->playername->setAlpha(1);

		x -= 18;
		row->flagimg = mpPanel->createWidget<MyGUI::StaticImage>("StaticImage", x, y + 3, 16, 11,  MyGUI::Align::Default, "Main");
		row->flagimg->setUserString("tooltip", "user country");
		row->flagimg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->flagimg->setNeedToolTip(true);
		row->flagimg->setVisible(false);

		x -= 18;
		row->statimg = mpPanel->createWidget<MyGUI::StaticImage>("StaticImage", x, y, 16, 16,  MyGUI::Align::Default, "Main");
		row->statimg->setUserString("tooltip", "user authentication level");
		row->statimg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->statimg->setNeedToolTip(true);
		row->statimg->setVisible(false);

		x -= 18;
		row->userTruckOKImg = mpPanel->createWidget<MyGUI::StaticImage>("StaticImage", x, y, 16, 16,  MyGUI::Align::Default, "Main");
		row->userTruckOKImg->setUserString("tooltip", "truck loading state");
		row->userTruckOKImg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->userTruckOKImg->setNeedToolTip(true);
		row->userTruckOKImg->setVisible(false);
		row->userTruckOKImg->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI_Multiplayer::clickInfoIcon);

		x -= 18;
		row->userTruckOKRemoteImg = mpPanel->createWidget<MyGUI::StaticImage>("StaticImage", x, y, 16, 16,  MyGUI::Align::Default, "Main");
		row->userTruckOKRemoteImg->setUserString("tooltip", "remote truck loading state");
		row->userTruckOKRemoteImg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->userTruckOKRemoteImg->setNeedToolTip(true);
		row->userTruckOKRemoteImg->setVisible(false);
		row->userTruckOKRemoteImg->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI_Multiplayer::clickInfoIcon);
		
		x -= 18;
		row->usergoimg = mpPanel->createWidget<MyGUI::StaticImage>("StaticImage", x, y, 16, 16,  MyGUI::Align::Default, "Main");
		row->usergoimg->setUserString("num", StringConverter::toString(i));
		row->usergoimg->setUserString("tooltip", "go to user");
		row->usergoimg->setImageTexture("user_go.png");
		row->usergoimg->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		row->usergoimg->setNeedToolTip(true);
		row->usergoimg->setVisible(false);
		row->usergoimg->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI_Multiplayer::clickUserGoIcon);

		/*
		img = MyGUI::Gui::getInstance().createWidget<MyGUI::StaticImage>("StaticImage", x-36, y, 16, 16,  MyGUI::Align::Default, "Overlapped");
		img->setImageTexture("information.png");
		img->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI_Multiplayer::clickInfoIcon);
		img->eventToolTip += MyGUI::newDelegate(this, &GUI_Multiplayer::openToolTip);
		img->setNeedToolTip(true);
		img->setUserString("info", StringConverter::toString(i));
		img->setUserString("tooltip", "information about the user");
		*/

		y += lineheight;
	}
}

GUI_Multiplayer::~GUI_Multiplayer()
{
}

void GUI_Multiplayer::updateSlot(player_row_t *row, user_info_t *c, bool self)
{
	int x = 100;
	int y = row->playername->getPosition().top;
	// name
	row->playername->setCaption(c->username);
	Ogre::ColourValue col = PlayerColours::getSingleton().getColour(c->colournum);
	row->playername->setTextColour(MyGUI::Colour(col.r, col.g, col.b, col.a));
	row->playername->setVisible(true);
	x -= 18;
	
	// flag
	Ogre::vector<Ogre::String>::type parts = StringUtil::split(string(c->language), "_");
	if(parts.size() == 2)
	{
		String lang = parts[1];
		StringUtil::toLowerCase(lang);
		row->flagimg->setImageTexture(lang + ".png");
		row->flagimg->setUserString("tooltip", "user language: " + parts[0] + " user country: " + parts[1]);
		row->flagimg->setVisible(true);
		row->flagimg->setPosition(x, y);
		x -= 18;
	} else
	{
		row->flagimg->setVisible(false);
	}
	
	// auth
	if(c->authstatus == AUTH_NONE)
	{
		row->statimg->setVisible(false);
	} else if (c->authstatus & AUTH_ADMIN)
	{
		row->statimg->setVisible(true);
		row->statimg->setImageTexture("flag_red.png");
		row->statimg->setUserString("tooltip", "Server Administrator");
		row->statimg->setPosition(x, y);
		x -= 18;
	} else if (c->authstatus & AUTH_MOD)
	{
		row->statimg->setVisible(true);
		row->statimg->setImageTexture("flag_blue.png");
		row->statimg->setUserString("tooltip", "Server Moderator");
		row->statimg->setPosition(x, y);
		x -= 18;
	} else if (c->authstatus & AUTH_RANKED)
	{
		row->statimg->setVisible(true);
		row->statimg->setImageTexture("flag_green.png");
		row->statimg->setUserString("tooltip", "ranked user");
		row->statimg->setPosition(x, y);
		x -= 18;
	}

	// truck ok image
	if(!self)
	{
		row->userTruckOKImg->setVisible(true);
		row->userTruckOKRemoteImg->setVisible(true);
		row->userTruckOKImg->setUserString("uid", StringConverter::toString(c->uniqueid));
		row->userTruckOKRemoteImg->setUserString("uid", StringConverter::toString(c->uniqueid));
		row->userTruckOKImg->setPosition(x, y);
		x -= 10;
		row->userTruckOKRemoteImg->setPosition(x, y);
		x -= 10;

		int ok = BeamFactory::getSingleton().checkStreamsOK(c->uniqueid);
		if(ok == 0)
		{
			row->userTruckOKImg->setImageTexture("arrow_down_red.png");
			row->userTruckOKImg->setUserString("tooltip", "Truck loading errors");
		} else if(ok == 1)
		{
			row->userTruckOKImg->setImageTexture("arrow_down.png");
			row->userTruckOKImg->setUserString("tooltip", "Truck loaded correctly, no errors");
		} else if(ok == 2)
		{
			row->userTruckOKImg->setImageTexture("arrow_down_grey.png");
			row->userTruckOKImg->setUserString("tooltip", "no truck loaded");
		}

		int rok = BeamFactory::getSingleton().checkStreamsRemoteOK(c->uniqueid);
		if(rok == 0)
		{
			row->userTruckOKRemoteImg->setImageTexture("arrow_up_red.png");
			row->userTruckOKRemoteImg->setUserString("tooltip", "Remote Truck loading errors");
		} else if(rok == 1)
		{
			row->userTruckOKRemoteImg->setImageTexture("arrow_up.png");
			row->userTruckOKRemoteImg->setUserString("tooltip", "Remote Truck loaded correctly, no errors");
		} else if(rok == 2)
		{
			row->userTruckOKRemoteImg->setImageTexture("arrow_up_grey.png");
			row->userTruckOKRemoteImg->setUserString("tooltip", "No Trucks loaded");
		}
	} else
	{
		row->userTruckOKImg->setVisible(false);
		row->userTruckOKRemoteImg->setVisible(false);
	}
	
	// user go img
	row->usergoimg->setVisible(false);
	/*
	// disabled for now, since no use
	if(!self)
	{
		row->usergoimg->setVisible(true);
		row->usergoimg->setPosition(x, y);
		x -= 18;
		row->usergoimg->setUserString("uid", StringConverter::toString(c->uniqueid));
		if (eflsingleton && eflsingleton->getNetPointToUID() == c->uniqueid)
		{
			// active for this user
			row->usergoimg->setAlpha(1.0f);
		} else
		{
			// inactive for this user
			row->usergoimg->setAlpha(0.4f);
		}
	} else
	{
		row->usergoimg->setVisible(false);
	}
	*/
}

int GUI_Multiplayer::update()
{
	client_t clients[MAX_PEERS];
	int slotid = 0;
	
	MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
	int x=gui_area.width - 200, y=30;
	mpPanel->setPosition(x,y);

	// add local player to first slot always
	user_info_t *lu = net->getLocalUserData();
	updateSlot(&player_rows[slotid], lu, true);
	slotid++;

	// add remote players
	int res = net->getClientInfos(clients);
	if(res) return 1;
	for(int i = 0; i < MAX_PEERS; i++)
	{
		client_t *c = &clients[i];
		player_row_t *row = &player_rows[slotid];
		// only count up slotid for used slots, so there are no gap in the list
		if(c->used)
		{
			// used
			slotid++;
			try
			{
				updateSlot(row, &c->user, false);
			} catch(...)
			{
			}
		} else
		{
			// not used, hide everything
			row->flagimg->setVisible(false);
			row->playername->setVisible(false);
			row->statimg->setVisible(false);
			row->usergoimg->setVisible(false);
			row->userTruckOKImg->setVisible(false);
			row->userTruckOKRemoteImg->setVisible(false);
		}
	}
	
	if(eflsingleton && eflsingleton->getNetQuality(true) != 0)
	{
		netmsgwin->setVisible(true);
	} else if(eflsingleton && eflsingleton->getNetQuality(true) == 0)
	{
		netmsgwin->setVisible(false);
	}
	return 0;
}


void GUI_Multiplayer::clickUserGoIcon(MyGUI::WidgetPtr sender)
{
	int uid = StringConverter::parseInt(sender->getUserString("uid"));
	if (!eflsingleton)
		return;
	if(eflsingleton->getNetPointToUID() == uid)
		eflsingleton->setNetPointToUID(-1);
	else
		eflsingleton->setNetPointToUID(uid);
}

void GUI_Multiplayer::clickInfoIcon(MyGUI::WidgetPtr sender)
{
	//msgtext->setCaption("FOOBAR: "+sender->getUserString("info"));
	//msgwin->setVisible(true);
}

void GUI_Multiplayer::openToolTip(MyGUI::WidgetPtr sender, const MyGUI::ToolTipInfo &t)
{
	if(t.type == MyGUI::ToolTipInfo::Show)
	{
		string txt = sender->getUserString("tooltip");
		if(!txt.empty())
		{
			tooltipText->setCaption(txt);
			tooltipPanel->setVisible(true);
			tooltipPanel->setPosition(t.point - MyGUI::IntPoint(210, 8));
		}
	} else if(t.type == MyGUI::ToolTipInfo::Hide)
	{
		tooltipPanel->setVisible(false);
	}
}

void GUI_Multiplayer::setVisible(bool value)
{
	mpPanel->setVisible(value);
}

bool GUI_Multiplayer::getVisible()
{
	return mpPanel->getVisible();
}

#endif // USE_SOCKETW
#endif // USE_MYGUI
