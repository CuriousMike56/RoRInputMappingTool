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
#include "TorqueCurve.h"
#include "Ogre.h"

using namespace Ogre;
using namespace std;

const String TorqueCurve::customModel = "CustomModel";

TorqueCurve::TorqueCurve() : usedSpline(0), usedModel("")
{
	loadDefaultTorqueModels();
	setTorqueModel("default");
}

TorqueCurve::~TorqueCurve()
{
	splines.clear();
}

Real TorqueCurve::getEngineTorque(Real rpmRatio)
{
	if(!usedSpline) return 1.0f; //return a good value upon error?
	return usedSpline->interpolate(rpmRatio).y;
}

int TorqueCurve::loadDefaultTorqueModels()
{
	LogManager::getSingleton().logMessage("loading default torque Curves");
	// check if we have a config file
	String group = "";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource("torque_models.cfg");
	} catch(...)
	{
	}
	// emit a warning if we did not found the file
	if (group.empty())
	{
		LogManager::getSingleton().logMessage("torque_models.cfg not found");
		return 1;
	}

	// open the file for reading
	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource("torque_models.cfg", group);
	String line = "";
	String currentModel = "";

	while (!ds->eof())
	{
		line=ds->getLine();
		StringUtil::trim(line);

		if (line.empty() || line[0]==';')
			continue;

		Ogre::vector< String >::type args = StringUtil::split(line, ",");

		if (args.size() == 1)
		{
			currentModel = line;
			continue;
		}

		// process the line if we got a model
		if(!currentModel.empty())
			processLine(args, currentModel);
	}
	return 0;
}

int TorqueCurve::processLine(String line, String model)
{
	return processLine(StringUtil::split(line, ","), model);
}

int TorqueCurve::processLine(Ogre::vector< String >::type args, String model)
{
	// if its just one arguments, it must be a known model
	if (args.size() == 1)
		return setTorqueModel(args[0]);

	// we only accept 2 arguments
	if (args.size() != 2)
		return 1;
	// parse the data
	float pointx = StringConverter::parseReal(args[0]);
	float pointy = StringConverter::parseReal(args[1]);
	Vector3 point = Vector3(pointx,pointy,0);

	// find the spline to attach the points
	if(splines.find(model) == splines.end())
		splines[model] = SimpleSpline();

	// attach the points to the spline
	// LogManager::getSingleton().logMessage("curve "+model+" : " + StringConverter::toString(point));
	splines[model].addPoint(point);

	// special case for custom model:
	// we set it as active curve as well!
	if(model == TorqueCurve::customModel)
		setTorqueModel(TorqueCurve::customModel);

	return 0;
}

int TorqueCurve::setTorqueModel(String name)
{
	//LogManager::getSingleton().logMessage("using torque curve: " + name);
	// check if we have such a model loaded
	if (splines.find(name) == splines.end())
	{
		LogManager::getSingleton().logMessage("Torquemodel "+String(name)+" not found! ignoring that and using default model...");
		return 1;
	}
	// use the model
	usedSpline = &splines.find(name)->second;
	usedModel = name;
	return 0;
}
