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
#include "Replay.h"

#include "Ogre.h"
#ifdef USE_MYGUI 
#include "gui_manager.h"
#endif // MYGUI
#include "rormemory.h"
#include "language.h"

using namespace Ogre;

Replay::Replay(Beam *b, int _numFrames)
{
	numNodes = b->getNodeCount();
	numBeams = b->getBeamCount();
	numFrames = _numFrames;

	replayTimer = new Timer();

	// DO NOT get memory here, get memory when we use it first time!
	nodes = 0;
	beams = 0;
	times = 0;

	unsigned long bsize = (numNodes * numFrames * sizeof(node_simple_t) + numBeams * numFrames * sizeof(beam_simple_t) + numFrames * sizeof(unsigned long)) / 1024.0f;
	LogManager::getSingleton().logMessage("replay buffer size: " + StringConverter::toString(bsize) + " kB");

	writeIndex = 0;
	firstRun = 1;

#ifdef USE_MYGUI 
	// windowing
	int width = 300;
	int height = 60;
	int x = (MyGUI::RenderManager::getInstance().getViewSize().width - width) / 2;
	int y = 0;

	panel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("Panel", x, y, width, height, MyGUI::Align::HCenter | MyGUI::Align::Top, "Back");
	//panel->setCaption(_L("Replay"));
	panel->setAlpha(0.6);

	pr = panel->createWidget<MyGUI::Progress>("Progress", 10, 10, 280, 20,  MyGUI::Align::Default);
	pr->setProgressRange(_numFrames);
	pr->setProgressPosition(0);


	txt = panel->createWidget<MyGUI::StaticText>("StaticText", 10, 30, 280, 20,  MyGUI::Align::Default);
	txt->setCaption(_L("Position:"));

	panel->setVisible(false);
#endif // MYGUI
}

Replay::~Replay()
{
	if(nodes)
	{
		ror_free(nodes); nodes=0;
		ror_free(beams); beams=0;
		ror_free(times); times=0;
	}
	delete replayTimer;
}

void *Replay::getWriteBuffer(int type)
{
	if(!nodes)
	{
		// get memory
		nodes = (node_simple_t*)ror_calloc(numNodes * numFrames, sizeof(node_simple_t));
		beams = (beam_simple_t*)ror_calloc(numBeams * numFrames, sizeof(beam_simple_t));	
		times = (unsigned long*)ror_calloc(numFrames, sizeof(unsigned long));
	}
	void *ptr = 0;
	times[writeIndex] = replayTimer->getMicroseconds();
	if(type == 0)
	{
		// nodes
		ptr = (void *)(nodes + (writeIndex * numNodes));
	}else if(type == 1)
	{
		// beams
		ptr = (void *)(beams + (writeIndex * numBeams));
	}
	return ptr;
}

void Replay::writeDone()
{
	writeIndex++;
	if(writeIndex == numFrames)
	{
		firstRun = 0;
		writeIndex = 0;
	}
}

//we take negative offsets only
void *Replay::getReadBuffer(int offset, int type, unsigned long &time)
{
	if (offset >= 0) offset=-1;
	if (offset <= -numFrames) offset = -numFrames + 1;
	
	int delta = writeIndex + offset;
	if (delta < 0)
		if (firstRun && type == 0)
			return (void *)(nodes);
		else if (firstRun && type == 1)
			return (void *)(beams);
		else
			delta += numFrames;
	
	// set the time
	time = times[delta];
	curFrameTime = time;
	curOffset = offset;
	updateGUI();
	
	// return buffer pointer
	if(type == 0)
		return (void *)(nodes + delta * numNodes);
	else if(type == 1)
		return (void *)(beams + delta * numBeams);
	return 0;
}

void Replay::updateGUI()
{
#ifdef USE_MYGUI 
	char tmp[128]="";
	unsigned long t = curFrameTime;
	sprintf(tmp, "Position: %0.6f s, frame %i / %i", ((float)t)/1000000.0f, curOffset, numFrames);
	txt->setCaption(String(tmp));
	//LogManager::getSingleton().logMessage(">>>2>"+StringConverter::toString(times[writeIndex]) + " /3> "+StringConverter::toString(curFrameTime));
	pr->setProgressPosition(abs(curOffset));
#endif // MYGUI
}

unsigned long Replay::getLastReadTime()
{
	return curFrameTime;
}

void Replay::setVisible(bool value)
{
#ifdef USE_MYGUI 
	panel->setVisible(value);
	// we need no mouse yet
	//MyGUI::PointerManager::getInstance().setVisible(value);
#endif //MYGUI
}

bool Replay::getVisible()
{
#ifdef USE_MYGUI 
	return panel->getVisible();
#else
	return false;
#endif //MYGUI
}


