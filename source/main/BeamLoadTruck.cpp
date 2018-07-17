

#include "Beam.h"

#include "engine.h"
#include "autopilot.h"
#include "ScopeLog.h"
#include "skinmanager.h"
#include "FlexObj.h"
#include "FlexAirfoil.h"
#include "turboprop.h"
#include "screwprop.h"
#include "buoyance.h"
#include "airbrake.h"
#include "TorqueCurve.h"
#include "Settings.h"
#include "CmdKeyInertia.h"
#include "MeshObject.h"

// TODO not really needed for truck loading, or used very rarely
#include "collisions.h"
#include "FlexBody.h"

//to load a truck file
int Beam::loadTruck(const char* fname, SceneManager *manager, SceneNode *parent, Real px, Real py, Real pz, Quaternion rot, collision_box_t *spawnbox)
{
	ScopeLog log("beam_"+String(fname));
	//FILE *fd;
	char line[1024];
	int linecounter = 0;
	int mode=0, savedmode=0;
	int hasfixes=0;
	int wingstart=-1;
	int leftlight=0;
	int rightlight=0;
	int managedmaterials_doublesided=0; // not by default
	float wingarea=0.0;
	int currentScriptCommandNumber=-1;
	bool enable_background_loading = (SETTINGS.getSetting("Background Loading") == "Yes");

	//convert ry
	//ry=ry*3.14159/180.0;
	LogManager::getSingleton().logMessage("BEAM: Start of truck loading");
	String group = "";
	String filename = String(fname);

	try
	{
		if(!CACHE.checkResourceLoaded(filename, group))
		{
			LogManager::getSingleton().logMessage("Can't open truck file '"+filename+"'");
			return -1;
		}
	} catch(Ogre::Exception& e)
	{
		if(e.getNumber() == Ogre::Exception::ERR_ITEM_NOT_FOUND)
		{
			LogManager::getSingleton().logMessage("Can't open truck file '"+filename+"'");
			return -1;
		}
	}

	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(filename, group);
	linecounter = 0;

//		fd=fopen(fname, "r");
//		if (!fd) {
//			LogManager::getSingleton().logMessage("Can't open truck file '"+String(fname)+"'");
//			exit(1);
//		};
	LogManager::getSingleton().logMessage("Parsing '"+String(fname)+"'");
	//skip first line
//		fscanf(fd," %[^\n\r]",line);
	ds->readLine(line, 1023);
	// read in truckname for real
	strncpy(realtruckname, line, 255);

	// some temp vars used
	Real inertia_startDelay=-1, inertia_stopDelay=-1;
	char inertia_default_startFunction[50]="", inertia_default_stopFunction[50]="";

	while (!ds->eof())
	{
		size_t ll=ds->readLine(line, 1023);
		linecounter++;
//			fscanf(fd," %[^\n\r]",line);
		//LogManager::getSingleton().logMessage(line);
		//        printf("Mode %i Line:'%s'\n",mode, line);
		if (ll==0 || line[0]==';' || line[0]=='/')
		{
			//    printf("%s\n", line+1);
			continue;
		};

		if (!strcmp("end",line))
		{
			LogManager::getSingleton().logMessage("BEAM: End of truck loading");
			loading_finished=1;break;
		};

		if (!strcmp("patchEngineTorque",line)) {patchEngineTorque=true;continue;};
		if (!strcmp("end_commandlist",line) && mode == 35) {mode=0;continue;};
		if (!strcmp("end_description",line) && mode == 29) {mode=0;continue;};
		if (!strcmp("end_comment",line)  && mode == 30) {mode=savedmode;continue;};
		if (!strcmp("end_section",line)  && mode == 52) {mode=savedmode;continue;};
		if (mode==29)
		{
			// description
			//parse dscription, and ignore every possible keyword
			//char *tmp = strdup(line);
			description.push_back(std::string(line));
			continue;
		}

		if (mode==30)
		{
			// comment
			// ignore everything
			continue;
		}
		if (mode==52)
		{
			// ignored truck section
			continue;
		}
		if (!strcmp("nodes",line)) {mode=1;continue;};
		if (!strcmp("beams",line)) {mode=2;continue;};
		if (!strcmp("fixes",line)) {mode=3;continue;};
		if (!strcmp("shocks",line)) {mode=4;continue;};
		if (!strcmp("hydros",line)) {mode=5;continue;};
		if (!strcmp("wheels",line)) {mode=6;continue;};
		if (!strcmp("globals",line)) {mode=7;continue;};
		if (!strcmp("cameras",line)) {mode=8;continue;};
		if (!strcmp("engine",line)) {mode=9;continue;};
		if (!strcmp("texcoords",line)) {mode=10;continue;};
		if (!strcmp("cab",line)) {mode=11;continue;};
		if (!strcmp("commands",line)) {mode=12;continue;};
		if (!strcmp("commands2",line)) {mode=120;continue;};
		if (!strcmp("forwardcommands",line)) {forwardcommands=1;continue;};
		if (!strcmp("importcommands",line)) {importcommands=1;continue;};
		if (!strcmp("rollon",line)) {wheel_contact_requested=true;continue;};
		if (!strcmp("rescuer",line)) {rescuer=true;continue;};
		if (!strcmp("contacters",line)) {mode=13;continue;};
		if (!strcmp("ropes",line)) {mode=14;continue;};
		if (!strcmp("ropables",line)) {mode=15;continue;};
		if (!strcmp("ties",line)) {mode=16;continue;};
		if (!strcmp("help",line)) {mode=17;continue;};
		if (!strcmp("cinecam",line)) {mode=18;continue;};
		if (!strcmp("flares",line)) {mode=19;continue;};
		if (!strcmp("props",line)) {mode=20;continue;};
		if (!strcmp("globeams",line)) {mode=21;continue;};
		if (!strcmp("wings",line)) {mode=22;continue;};
		if (!strcmp("turboprops",line)) {mode=23;continue;};
		if (!strcmp("fusedrag",line)) {mode=24;continue;};
		if (!strcmp("engoption",line)) {mode=25;continue;};
		if (!strcmp("brakes",line)) {mode=26;continue;};
		if (!strcmp("rotators",line)) {mode=27;continue;};
		if (!strcmp("screwprops",line)) {mode=28;continue;};
		if (!strcmp("description",line)) {mode=29;continue;};
		if (!strcmp("comment",line)) {mode=30; savedmode=mode; continue;};
		if (!strcmp("wheels2",line)) {mode=31;continue;};
		if (!strcmp("guisettings",line)) {mode=32;continue;};
		if (!strcmp("minimass",line)) {mode=33;continue;};
		if (!strcmp("exhausts",line)) {mode=34;continue;};
		if (!strcmp("turboprops2",line)) {mode=35;continue;};
		if (!strcmp("pistonprops",line)) {mode=36;continue;};
		//apparently mode 37 is reserved for other use
		if (!strcmp("particles",line)) {mode=38;continue;};
		if (!strcmp("turbojets",line)) {mode=39;continue;};
		if (!strcmp("rigidifiers",line)) {mode=40;continue;};
		if (!strcmp("airbrakes",line)) {mode=41;continue;};
		if (!strcmp("meshwheels",line)) {mode=42;continue;};
		if (!strcmp("flexbodies",line)) {mode=43;continue;};
		if (!strcmp("hookgroup",line)) {mode=44; continue;};
		if (!strncmp("materialflarebindings",line, 21)) {mode=46; continue;};
		if (!strcmp("disabledefaultsounds",line)) {disable_default_sounds=true;continue;};
		if (!strcmp("soundsources",line)) {mode=47;continue;};
		if (!strcmp("envmap",line)) {mode=48;continue;};
		if (!strcmp("managedmaterials",line)) {mode=49;continue;};
		if (!strncmp("sectionconfig",line, 13)) {savedmode=mode;mode=50; /* NOT continue */};
		if (!strncmp("section",line, 7) && mode!=50) {mode=51; /* NOT continue */};
		/* 52 = reserved for ignored section */
		if (!strcmp("torquecurve",line)) {mode=53;continue;};
		if (!strcmp("advdrag",line)) {mode=54;continue;};
		if (!strcmp("axles",line)) {mode=55;continue;};
		if (!strcmp("shocks2",line)) {mode=56;continue;};
		if (!strcmp("railgroups",line)) {mode=63;continue;}
		if (!strcmp("slidenodes",line)) {mode=64;continue;}
		if (!strcmp("flares2",line)) {mode=65;continue;};
		if (!strcmp("animators",line)) {mode=66;continue;};
		if (!strncmp("enable_advanced_deformation", line, 27))
		{
			// parse the optional threshold value
			beam_creak = 0.0f;
			LogManager::getSingleton().logMessage("Advanced deformation beam physics enabled");
			continue;
		}
		if (!strcmp("commandlist",line))
		{
			int result = sscanf(line,"commandlist %d", &currentScriptCommandNumber);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (commandlist) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			mode=37;
			continue;
		};
		if (!strncmp("fileinfo", line, 8))
		{
			int result = sscanf(line,"fileinfo %s, %i, %i", uniquetruckid, &categoryid, &truckversion);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (fileinfo) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			mode=0;
			continue;
		}
		if (!strncmp("fileformatversion", line, 17))
		{
			int fileformatversion;
			int result = sscanf(line,"fileformatversion %i", &fileformatversion);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (fileformatversion) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (fileformatversion > TRUCKFILEFORMATVERSION) {
				LogManager::getSingleton().logMessage("The Truck File " + String(fname) +" is for a newer version or RoR! trying to continue ...");
				continue;
			}
			mode=0;
			continue;
		}
		if (!strncmp("author", line, 6))
		{
			int authorid;
			char authorname[255], authoremail[255], authortype[255];
			authorinfo_t author;
			author.id = -1;
			strcpy(author.email, "unknown");
			strcpy(author.name,  "unknown");
			strcpy(author.type,  "unknown");

			int result = sscanf(line,"author %s %i %s %s", authortype, &authorid, authorname, authoremail);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (author) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//replace '_' with ' '
			char *tmp = authorname;
			while (*tmp!=0) {if (*tmp=='_') *tmp=' ';tmp++;};
			//fill the struct now
			author.id = authorid;
			if(strnlen(authortype, 250) > 0)
				strncpy(author.type, authortype, 255);
			if(strnlen(authorname, 250) > 0)
				strncpy(author.name, authorname, 255);
			if(strnlen(authoremail, 250) > 0)
				strncpy(author.email, authoremail, 255);
			authors.push_back(author);
			mode=0;
			continue;
		}

		if(!strcmp("slidenode_connect_instantly", line))
			slideNodesConnectInstantly=true;

		if (!strncmp("add_animation", line, 13))
		{
			/*
			 * this command has several layers for splitting up the line:
			 * 1. ',' the top level will be split up with a comma to separate the main options
			 * 2. ':' the second level will be split up with colon, it separates the entry name and its value
			 * 3. '|' the third level is used to specify multiple values for the entry value
			 */
			int animnum = 0;
			float ratio = 0.0f, opt1 = -1.0f, opt2 = -1.0f;
			// parse the line
			if(String(line).size() < 14)
			{
				LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Not enough Options parsed, trying to continue ...");
				continue;
			}
			Ogre::vector<Ogre::String>::type options = Ogre::StringUtil::split(String(line).substr(14), ","); // "add_animation " = 14 characters
			// check for common errors
			if (options.size() < 4)
			{
				LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Not enough Options parsed, trying to continue ...");
				continue;
			}
			if (!free_prop)
			{
				LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". No prop to animate existing, trying to continue ...");
				continue;
			}

			// always use the last prop
			prop_t *prop = &props[free_prop-1];

			// look for a free anim slot, important: do not over the borders!
			while (prop->animFlags[animnum] && animnum < 10)
				animnum++;

			// all slots used?
			if (animnum >= 10)
			{
				LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Cant animate a prop more then 10 times. Trying to continue...");
				continue;
			}

			// parse the arguments individually
			for(unsigned int i=0;i<options.size();i++)
			{
				if(i == 0)
				{
					ratio = StringConverter::parseReal(options[i]);
					//set ratio
					if (ratio)
						prop->animratio[animnum]=ratio;
					else
						LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Animation-Ratio = 0 ?");
				} else if(i == 1)
				{
					opt1 = StringConverter::parseReal(options[i]);
					prop->animOpt1[animnum] = opt1;
				} else if(i == 2)
				{
					opt2 = StringConverter::parseReal(options[i]);
					prop->animOpt2[animnum] = opt2;
					prop->animOpt3[animnum] = 0.0f;
					prop->animOpt4[animnum] = 0.0f;
					prop->animOpt5[animnum] = 0.0f;
					prop->animKeyState[animnum] = -1.0f;
				} else
				{
					// parse the rest
					Ogre::vector<Ogre::String>::type args2 = Ogre::StringUtil::split(options[i], ":");
					if(args2.size() == 0)
						continue;

					// trim spaces from the entry
					Ogre::StringUtil::trim(args2[0]);
					if(args2.size() >= 2) Ogre::StringUtil::trim(args2[1]);

					if(args2[0] == "source" && args2.size() == 2)
					{
						//set source identification flag
						Ogre::vector<Ogre::String>::type args3 = Ogre::StringUtil::split(args2[1], "|");
						for(unsigned int j=0;j<args3.size();j++)
						{
							String sourceStr = args3[j];
							Ogre::StringUtil::trim(sourceStr);
							if (sourceStr == "airspeed")             { prop->animFlags[animnum] |= (ANIM_FLAG_AIRSPEED); }
							else if (sourceStr == "vvi")             { prop->animFlags[animnum] |= (ANIM_FLAG_VVI); }
							else if (sourceStr == "altimeter100k")   { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "altimeter10k")    { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "altimeter1k")     { prop->animFlags[animnum] |= (ANIM_FLAG_ALTIMETER); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "aoa")             { prop->animFlags[animnum] |= (ANIM_FLAG_AOA); }
							else if (sourceStr == "flap")            { prop->animFlags[animnum] |= (ANIM_FLAG_FLAP); }
							else if (sourceStr == "airbrake")        { prop->animFlags[animnum] |= (ANIM_FLAG_AIRBRAKE); }
							else if (sourceStr == "roll")            { prop->animFlags[animnum] |= (ANIM_FLAG_ROLL); }
							else if (sourceStr == "pitch")           { prop->animFlags[animnum] |= (ANIM_FLAG_PITCH); }
							else if (sourceStr == "throttle1")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "throttle2")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "throttle3")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "throttle4")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "throttle5")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "throttle6")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "throttle7")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "throttle8")       { prop->animFlags[animnum] |= (ANIM_FLAG_THROTTLE); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "rpm1")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "rpm2")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "rpm3")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "rpm4")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "rpm5")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "rpm6")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "rpm7")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "rpm8")            { prop->animFlags[animnum] |= (ANIM_FLAG_RPM); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "aerotorq1")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "aerotorq2")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "aerotorq3")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "aerotorq4")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "aerotorq5")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "aerotorq6")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "aerotorq7")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "aerotorq8")       { prop->animFlags[animnum] |= (ANIM_FLAG_AETORQUE); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "aeropit1")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "aeropit2")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "aeropit3")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "aeropit4")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "aeropit5")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "aeropit6")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "aeropit7")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "aeropit8")        { prop->animFlags[animnum] |= (ANIM_FLAG_AEPITCH); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "aerostatus1")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "aerostatus2")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "aerostatus3")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "aerostatus4")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "aerostatus5")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 5.0f; }
							else if (sourceStr == "aerostatus6")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 6.0f; }
							else if (sourceStr == "aerostatus7")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 7.0f; }
							else if (sourceStr == "aerostatus8")     { prop->animFlags[animnum] |= (ANIM_FLAG_AESTATUS); prop->animOpt3[animnum] = 8.0f; }
							else if (sourceStr == "brakes")          { prop->animFlags[animnum] |= (ANIM_FLAG_BRAKE); }
							else if (sourceStr == "accel")           { prop->animFlags[animnum] |= (ANIM_FLAG_ACCEL); }
							else if (sourceStr == "clutch")          { prop->animFlags[animnum] |= (ANIM_FLAG_CLUTCH); }
							else if (sourceStr == "speedo")          { prop->animFlags[animnum] |= (ANIM_FLAG_SPEEDO); }
							else if (sourceStr == "tacho")           { prop->animFlags[animnum] |= (ANIM_FLAG_TACHO); }
							else if (sourceStr == "turbo")           { prop->animFlags[animnum] |= (ANIM_FLAG_TURBO); }
							else if (sourceStr == "parking")         { prop->animFlags[animnum] |= (ANIM_FLAG_PBRAKE); }
							else if (sourceStr == "shifterman1")     { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 1.0f; }
							else if (sourceStr == "shifterman2")     { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 2.0f; }
							else if (sourceStr == "sequential")      { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 3.0f; }
							else if (sourceStr == "shifterlin")      { prop->animFlags[animnum] |= (ANIM_FLAG_SHIFTER); prop->animOpt3[animnum] = 4.0f; }
							else if (sourceStr == "torque")          { prop->animFlags[animnum] |= (ANIM_FLAG_TORQUE); }
							else if (sourceStr == "heading")         { prop->animFlags[animnum] |= (ANIM_FLAG_HEADING); }
							else if (sourceStr == "difflock")        { prop->animFlags[animnum] |= (ANIM_FLAG_DIFFLOCK); }
							else if (sourceStr == "steeringwheel")   { prop->animFlags[animnum] |= (ANIM_FLAG_STEERING); }
							else if (sourceStr == "aileron")         { prop->animFlags[animnum] |= (ANIM_FLAG_AILERONS); }
							else if (sourceStr == "elevator")        { prop->animFlags[animnum] |= (ANIM_FLAG_ELEVATORS); }
							else if (sourceStr == "rudderair")       { prop->animFlags[animnum] |= (ANIM_FLAG_ARUDDER); }
							else if (sourceStr == "rudderboat")      { prop->animFlags[animnum] |= (ANIM_FLAG_BRUDDER); }
							else if (sourceStr == "throttleboat")    { prop->animFlags[animnum] |= (ANIM_FLAG_BTHROTTLE); }
							else if (sourceStr == "permanent")       { prop->animFlags[animnum] |= (ANIM_FLAG_PERMANENT); }
							else if (sourceStr == "event")           { prop->animFlags[animnum] |= (ANIM_FLAG_EVENT); }
						}

						if (prop->animFlags[animnum] == 0)
							LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Failed to identify source.");
						else
							LogManager::getSingleton().logMessage("Animation source set to prop#: " + StringConverter::toString(free_prop-1) + ", flag " +StringConverter::toString(prop->animFlags[animnum]) + ", Animationnumber: " + StringConverter::toString(animnum));
					}
					else if(args2[0] == "mode" && args2.size() == 2)
					{
						//set mode identification flag
						Ogre::vector<Ogre::String>::type args3 = Ogre::StringUtil::split(args2[1], "|");
						for(unsigned int j=0;j<args3.size();j++)
						{
							String modeStr = args3[j];
							Ogre::StringUtil::trim(modeStr);
							if (modeStr == "x-rotation")      prop->animMode[animnum] |= (ANIM_MODE_ROTA_X);
							else if (modeStr == "y-rotation") prop->animMode[animnum] |= (ANIM_MODE_ROTA_Y);
							else if (modeStr == "z-rotation") prop->animMode[animnum] |= (ANIM_MODE_ROTA_Z);
							else if (modeStr == "x-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_X);
							else if (modeStr == "y-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_Y);
							else if (modeStr == "z-offset")   prop->animMode[animnum] |= (ANIM_MODE_OFFSET_Z);
						}

						if (prop->animMode[animnum] == 0)
							LogManager::getSingleton().logMessage("Error parsing File (add_animation) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Failed to identify animation mode.");
						else
							LogManager::getSingleton().logMessage("Animation mode set to prop#: " + StringConverter::toString(free_prop-1)+ ", mode " +StringConverter::toString(prop->animMode[animnum]) + ", Animationnumber: " + StringConverter::toString(animnum));
					}
					else if (args2[0] == "autoanimate" && args2.size() == 1)
					{
						// TODO: re-add check for invalid cases
						prop->animMode[animnum] |= (ANIM_MODE_AUTOANIMATE);

						if     (prop->animMode[animnum] & ANIM_MODE_ROTA_X)   { prop->animOpt1[animnum] = opt1 + prop->rotaX; prop->animOpt2[animnum] = opt2 + prop->rotaX; prop->animOpt4[animnum] = prop->rotaX; }
						else if(prop->animMode[animnum] & ANIM_MODE_ROTA_Y)   { prop->animOpt1[animnum] = opt1 + prop->rotaY; prop->animOpt2[animnum] = opt2 + prop->rotaY; prop->animOpt4[animnum] = prop->rotaY; }
						else if(prop->animMode[animnum] & ANIM_MODE_ROTA_Z)   { prop->animOpt1[animnum] = opt1 + prop->rotaZ; prop->animOpt2[animnum] = opt2 + prop->rotaZ; prop->animOpt4[animnum] = prop->rotaZ; }
						else if(prop->animMode[animnum] & ANIM_MODE_OFFSET_X) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetX; prop->animOpt2[animnum] = opt2 + prop->orgoffsetX; prop->animOpt4[animnum] = prop->orgoffsetX; }
						else if(prop->animMode[animnum] & ANIM_MODE_OFFSET_Y) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetY; prop->animOpt2[animnum] = opt2 + prop->orgoffsetY; prop->animOpt4[animnum] = prop->orgoffsetY; }
						else if(prop->animMode[animnum] & ANIM_MODE_OFFSET_Z) { prop->animOpt1[animnum] = opt1 + prop->orgoffsetZ; prop->animOpt2[animnum] = opt2 + prop->orgoffsetZ; prop->animOpt4[animnum] = prop->orgoffsetZ; }
						LogManager::getSingleton().logMessage("Animation mode Autoanimation added to prop#: " + StringConverter::toString(free_prop-1) + " , Animationnumber: " + StringConverter::toString(animnum));
					}
					else if (args2[0] == "noflip" && args2.size() == 1)
					{
						prop->animMode[animnum] |= (ANIM_MODE_NOFLIP);
					}
					else if (args2[0] == "bounce" && args2.size() == 1)
					{
						prop->animMode[animnum] |= (ANIM_MODE_BOUNCE);
						prop->animOpt5[animnum] = 1.0f;
					}
					else if (args2[0] == "eventlock" && args2.size() == 1)
					{
						prop->animKeyState[animnum] = 0.0f;
						prop->lastanimKS[animnum] = 0.0f;
					}
					else if (args2[0] == "event" && args2.size() == 2)
					{
						// we are using keys as source
						prop->animFlags[animnum] |= ANIM_FLAG_EVENT;

						// now parse the keys
						prop->animFlags[animnum] |= ANIM_FLAG_EVENT;
						Ogre::vector<Ogre::String>::type args3 = Ogre::StringUtil::split(args2[1], "|");
						for(unsigned int j=0;j<args3.size();j++)
						{
							String eventStr = args3[j];
							Ogre::StringUtil::trim(eventStr);
							Ogre::StringUtil::toUpperCase(eventStr);
							int evtID = INPUTENGINE.resolveEventName(eventStr);
							if(evtID != -1)
								prop->animKey[animnum] = evtID;
							else
								LogManager::getSingleton().logMessage("Animation event unkown: " + eventStr);
						}
					}

				}
			}
			continue;
		}


		if (!strncmp("set_managedmaterials_options", line, 28))
		{
			int result = sscanf(line,"set_managedmaterials_options %d", &managedmaterials_doublesided);
			if (result < 1 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_managedmaterials_options) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			continue;
		}
		if (!strncmp("set_beam_defaults_scale", line, 23))
		{
			int result = sscanf(line,"set_beam_defaults_scale %f, %f, %f, %f", &default_spring_scale, &default_damp_scale, &default_deform_scale, &default_break_scale);
			if (result < 4 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_beam_defaults_scale) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			continue;
		}
		if (!strncmp("set_beam_defaults", line, 17))
		{
			char default_beam_material2[256]="";
			float tmpdefault_plastic_coef=-1.0f;
			int result = sscanf(line,"set_beam_defaults %f, %f, %f, %f, %f, %s, %f", &default_spring, &default_damp, &default_deform,&default_break,&default_beam_diameter, default_beam_material2, &tmpdefault_plastic_coef);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (set_beam_defaults) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(strnlen(default_beam_material2, 255))
			{
				MaterialPtr mat = MaterialManager::getSingleton().getByName(String(default_beam_material2));
				if(!mat.isNull())
					strncpy(default_beam_material, default_beam_material2, 256);
				else
					LogManager::getSingleton().logMessage("beam material '" + String(default_beam_material2) + "' not found!");
			}
			if (default_spring<0) default_spring=DEFAULT_SPRING;
			if (default_damp<0) default_damp=DEFAULT_DAMP;
			if (default_deform<0) default_deform=BEAM_DEFORM;
			if (default_break<0) default_break=BEAM_BREAK;
			if (default_beam_diameter<0) default_beam_diameter=DEFAULT_BEAM_DIAMETER;
			if (tmpdefault_plastic_coef>=0.0f)
			{
				beam_creak=0.0f;
				default_plastic_coef=tmpdefault_plastic_coef;
			}
			if(default_spring_scale != 1 || default_damp_scale != 1 || default_deform_scale != 1 || default_break_scale != 1)
			{
				LogManager::getSingleton().logMessage("Due to using set_beam_defaults_scale, this set_beam_defaults was interpreted as  " + \
					StringConverter::toString(default_spring * default_spring_scale) + ", " + \
					StringConverter::toString(default_damp   * default_damp_scale) + ", " + \
					StringConverter::toString(default_deform * default_deform_scale) + ", " + \
					StringConverter::toString(default_break  * default_break_scale) + ". In " + \
					String(fname) +" line " + StringConverter::toString(linecounter) + ".");

			}
			continue;
		}
		if (!strncmp("set_inertia_defaults", line, 20))
		{
			int result = sscanf(line,"set_inertia_defaults %f, %f, %s %s",&inertia_startDelay, &inertia_stopDelay, inertia_default_startFunction, inertia_default_stopFunction);
			if (result < 1 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_inertia_defaults) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (inertia_startDelay < 0 || inertia_stopDelay < 0)
			{
				// reset it
				inertia_startDelay = -1;
				inertia_stopDelay  = -1;
				memset(inertia_default_startFunction, 0, sizeof(inertia_default_startFunction));
				memset(inertia_default_stopFunction, 0, sizeof(inertia_default_stopFunction));
			}
			continue;
		}

		if (!strncmp("set_node_defaults", line, 17))
		{
			int result = sscanf(line,"set_node_defaults %f, %f, %f, %f, %s", &default_node_loadweight, &default_node_friction, &default_node_volume, &default_node_surface, default_node_options);
			if (result < 1 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_node_defaults) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (default_node_friction < 0)   default_node_friction=NODE_FRICTION_COEF_DEFAULT;
			if (default_node_volume < 0)     default_node_volume=NODE_VOLUME_COEF_DEFAULT;
			if (default_node_surface < 0)    default_node_surface=NODE_SURFACE_COEF_DEFAULT;
			if (default_node_loadweight < 0) default_node_loadweight=NODE_LOADWEIGHT_DEFAULT;
			if (result <= 4) memset(default_node_options, 0, sizeof default_node_options);
			continue;
		}

		if (!strncmp("set_skeleton_settings", line, 21))
		{
			int result = sscanf(line,"set_skeleton_settings %f, %f", &fadeDist, &skeleton_beam_diameter);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_skeleton_settings) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(fadeDist<0)
				fadeDist=150;
			if(skeleton_beam_diameter<0)
				skeleton_beam_diameter=BEAM_SKELETON_DIAMETER;
			continue;
		}

		if (!strcmp("backmesh",line))
		{
			//close the current mesh
			subtexcoords[free_sub]=free_texcoord;
			subcabs[free_sub]=free_cab;
			if(free_sub >= MAX_SUBMESHES)
			{
				LogManager::getSingleton().logMessage("submesh limit reached ("+StringConverter::toString(MAX_SUBMESHES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//make it normal
			subisback[free_sub]=0;
			free_sub++;

			//add an extra front mesh
			int i;
			int start;
			//texcoords
			if (free_sub==1) start=0; else start=subtexcoords[free_sub-2];
			for (i=start; i<subtexcoords[free_sub-1]; i++)
			{
				texcoords[free_texcoord]=texcoords[i];;
				free_texcoord++;
			}
			//cab
			if (free_sub==1) start=0; else start=subcabs[free_sub-2];
			for (i=start; i<subcabs[free_sub-1]; i++)
			{
				cabs[free_cab*3]=cabs[i*3];
				cabs[free_cab*3+1]=cabs[i*3+1];
				cabs[free_cab*3+2]=cabs[i*3+2];
				free_cab++;
			}
			//finish it, this is a window
			subisback[free_sub]=2;
			//close the current mesh
			subtexcoords[free_sub]=free_texcoord;
			subcabs[free_sub]=free_cab;
			//make is transparent
			free_sub++;


			//add an extra back mesh
			//texcoords
			if (free_sub==1) start=0; else start=subtexcoords[free_sub-2];
			for (i=start; i<subtexcoords[free_sub-1]; i++)
			{
				texcoords[free_texcoord]=texcoords[i];;
				free_texcoord++;
			}
			//cab
			if (free_sub==1) start=0; else start=subcabs[free_sub-2];
			for (i=start; i<subcabs[free_sub-1]; i++)
			{
				cabs[free_cab*3]=cabs[i*3+1];
				cabs[free_cab*3+1]=cabs[i*3];
				cabs[free_cab*3+2]=cabs[i*3+2];
				free_cab++;
			}
			//we don't finish, there will be a submesh statement later
			subisback[free_sub]=1;
			continue;
		};
		if (!strcmp("submesh",line))
		{
			subtexcoords[free_sub]=free_texcoord;
			subcabs[free_sub]=free_cab;
			if(free_sub >= MAX_SUBMESHES)
			{
				LogManager::getSingleton().logMessage("submesh limit reached ("+StringConverter::toString(MAX_SUBMESHES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			free_sub++;
			//initialize the next
			subisback[free_sub]=0;
			continue;
		};

		if (!strncmp("set_collision_range", line, 21))
		{
			int result = sscanf(line,"set_collision_range %f", &collrange);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (set_collision_range) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(collrange<0)
				collrange=DEFAULT_COLLISION_RANGE;
			continue;
		};

		if (mode==1)
		{
			//parse nodes
			int id=0;
			float x=0, y=0, z=0, mass=0;
			char options[255] = "n";
			int result = sscanf(line,"%i, %f, %f, %f, %s %f",&id,&x,&y,&z,options, &mass);
			// catch some errors
			if (result < 4 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				//LogManager::getSingleton().logMessage(strerror(errno));
				continue;
			}
			if (id != free_node)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Node) " + String(fname) +" line " + StringConverter::toString(linecounter) + ":");
				LogManager::getSingleton().logMessage("Error: lost sync in nodes numbers after node " + StringConverter::toString(free_node) + "(got " + StringConverter::toString(id) + " instead)");
				exit(2);
			};

			if(free_node >= MAX_NODES)
			{
				LogManager::getSingleton().logMessage("nodes limit reached ("+StringConverter::toString(MAX_NODES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			Vector3 npos = Vector3(px, py, pz) + rot * Vector3(x,y,z);
			init_node(id, npos.x, npos.y, npos.z, NODE_NORMAL, 10, 0, 0, free_node, -1, default_node_friction, default_node_volume, default_node_surface, default_node_loadweight);
			nodes[id].iIsSkin=true;

			if 	(default_node_loadweight >= 0.0f)
			{
				nodes[id].masstype=NODE_LOADED;
				nodes[id].overrideMass=true;
				nodes[id].mass=default_node_loadweight;
			}

			// merge options and default_node_options
			strncpy(options, ((String(default_node_options) + String(options)).c_str()), 250);

			// now 'parse' the options
			char *options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'l':	// load node
						if(mass != 0)
						{
							nodes[id].masstype=NODE_LOADED;
							nodes[id].overrideMass=true;
							nodes[id].mass=mass;
						}
						else
						{
							nodes[id].masstype=NODE_LOADED;
							masscount++;
						}
						break;
					case 'x':	//exhaust
						if (disable_smoke)
							break;
						if(smokeId == 0 && smokeRef != 0)
						{
							exhaust_t e;
							e.emitterNode = id;
							e.directionNode = smokeRef;
							e.isOldFormat = true;
							//smokeId=id;
							e.smokeNode = parent->createChildSceneNode();
							//ParticleSystemManager *pSysM=ParticleSystemManager::getSingletonPtr();
							char wname[256];
							sprintf(wname, "exhaust-%d-%s", (int)exhausts.size(), truckname);
							//if (pSysM) smoker=pSysM->createSystem(wname, "tracks/Smoke");
							e.smoker=manager->createParticleSystem(wname, "tracks/Smoke");
							// ParticleSystem* pSys = ParticleSystemManager::getSingleton().createSystem("exhaust", "tracks/Smoke");
							if (!e.smoker)
								continue;
							e.smokeNode->attachObject(e.smoker);
							e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
							exhausts.push_back(e);

							nodes[smokeId].isHot=true;
							nodes[id].isHot=true;
						}
						smokeId = id;
						break;
					case 'y':	//exhaust reference
						if (disable_smoke)
							break;
						if(smokeId != 0 && smokeRef == 0)
						{
							exhaust_t e;
							e.emitterNode = smokeId;
							e.directionNode = id;
							e.isOldFormat = true;
							//smokeId=id;
							e.smokeNode = parent->createChildSceneNode();
							//ParticleSystemManager *pSysM=ParticleSystemManager::getSingletonPtr();
							char wname[256];
							sprintf(wname, "exhaust-%d-%s", (int)exhausts.size(), truckname);
							//if (pSysM) smoker=pSysM->createSystem(wname, "tracks/Smoke");
							e.smoker=manager->createParticleSystem(wname, "tracks/Smoke");
							// ParticleSystem* pSys = ParticleSystemManager::getSingleton().createSystem("exhaust", "tracks/Smoke");
							if (!e.smoker)
								continue;
							e.smokeNode->attachObject(e.smoker);
							e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
							exhausts.push_back(e);

							nodes[smokeId].isHot=true;
							nodes[id].isHot=true;
						}
						smokeRef = id;
						break;
					case 'c':	//contactless
						nodes[id].contactless = 1;
						break;
					case 'h':	//hook
						// emulate the old behaviour using new fancy hookgroups
						hook_t h;
						h.hookNode=&nodes[id];
						h.locked=UNLOCKED;
						h.lockNode=0;
						h.lockTruck=0;
						h.lockNodes=true;
						h.group=-1;
						hooks.push_back(h);
						break;
					case 'e':	//editor
						if (!networking)
							editorId=id;
						break;
					case 'b':	//buoy
						nodes[id].buoyancy=10000.0f;
						break;
					case 'p':	//diasble particles
						nodes[id].disable_particles=true;
					break;
					case 'L':	//Log data:
						LogManager::getSingleton().logMessage("Node " + StringConverter::toString(id) + "  settings. Node load mass: " + StringConverter::toString(nodes[id].mass) + ", friction coefficient: " + StringConverter::toString(default_node_friction) + " and buoyancy volume coefficient: " + StringConverter::toString(default_node_volume) + " Fluid drag surface coefficient: " + StringConverter::toString(default_node_surface)+ " Particle mode: " + StringConverter::toString(nodes[id].disable_particles));
					break;
				}
				options_pointer++;
			}
			free_node++;
		}
		else if (mode==2)
		{
			//parse beams
			int id1, id2;
			char options[50] = "v";
			int type=BEAM_NORMAL;
			int result = sscanf(line,"%i, %i, %s",&id1,&id2,options);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Beam) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in beams section ("
					+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(3);
			};

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			//skip if a beam already exists
			int i;
			for (i=0; i<free_beam; i++)
			{
				if ((beams[i].p1==&nodes[id1] && beams[i].p2==&nodes[id2]) || (beams[i].p1==&nodes[id2] && beams[i].p2==&nodes[id1]))
				{
					LogManager::getSingleton().logMessage("Skipping duplicate beams: from node "+StringConverter::toString(id1)+" to node "+StringConverter::toString(id2));
					continue;
				}
			}

			// FIXME: separate init_beam and setup_beam to be able to set all parameters after creation
			// this is just ugly:
			char *options_pointer = options;
			while (*options_pointer != 0) {
				if(*options_pointer=='i') {
					type=BEAM_INVISIBLE;
					break;
				}
				options_pointer++;
			}


			// TODO Unused Varaible
			//float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
			/*
			if(beam_length < 0.01f)
			{
				LogManager::getSingleton().logMessage("Error: beam "+StringConverter::toString(free_beam)+" is too short ("+StringConverter::toString(beam_length)+"m)");
				LogManager::getSingleton().logMessage("Error: beam "+StringConverter::toString(free_beam)+" is between node "+StringConverter::toString(id1)+" and node "+StringConverter::toString(id2)+".");
				//LogManager::getSingleton().logMessage("will ignore this beam.");
				exit(8);
			}
			*/

			int pos=add_beam(&nodes[id1], &nodes[id2], manager, \
				parent, type, default_break * default_break_scale, default_spring * default_spring_scale, \
				default_damp * default_damp_scale, -1, -1, -1, 1, \
				default_beam_diameter);

			// now 'parse' the options
			options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'i':	// invisible
						beams[pos].type=BEAM_INVISIBLE;
						break;
					case 'v':	// visible
						beams[pos].type=BEAM_NORMAL;
						break;
					case 'r':
						beams[pos].bounded=ROPE;
						break;
					case 's':
						beams[pos].bounded=SUPPORTBEAM;
						break;
				}
				options_pointer++;
			}
		}
		else if (mode==4)
		{
			//parse shocks
			int id1, id2;
			float s, d, sbound,lbound,precomp;
			char options[50]="n";
			int result = sscanf(line,"%i, %i, %f, %f, %f, %f, %f, %s", &id1, &id2, &s, &d, &sbound, &lbound, &precomp, options);
			if (result < 7 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Shock) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			// checks ...
			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_shock >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("shock limit reached ("+StringConverter::toString(MAX_SHOCKS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in shocks section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(4);
			}


			// options
			int htype = BEAM_HYDRO;
			int shockflag = SHOCK_FLAG_NORMAL;

			// now 'parse' the options
			char *options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'i':	// invisible
						htype=BEAM_INVISIBLE_HYDRO;
						shockflag |= SHOCK_FLAG_INVISIBLE;
						break;
					case 'l':
					case 'L':
						shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
						shockflag |= SHOCK_FLAG_LACTIVE;
						free_active_shock++; // this has no array associated with it. its just to determine if there are active shocks!
						break;
					case 'r':
					case 'R':
						shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
						shockflag |= SHOCK_FLAG_RACTIVE;
						free_active_shock++; // this has no array associated with it. its just to determine if there are active shocks!
						break;
					case 'm':
						{
							// metric values: calculate sbound and lbound now
							float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
							sbound = sbound / beam_length;
							lbound = lbound / beam_length;
						}
						break;
				}
				options_pointer++;
			}
			int pos = add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break*4.0, s, d, -1.0, sbound, lbound, precomp);
			beams[pos].shock = &shocks[free_shock];
			shocks[free_shock].beamid = pos;
			shocks[free_shock].flags = shockflag;
			free_shock++;
		}
		else if (mode==56)
		{
			//parse shocks2
			int id1, id2;
			float sin=-1.0f,din=-1.0f,psin=-1.0f,pdin=-1.0f,sout=-1.0f,dout=-1.0f,psout=-1.0f,pdout=-1.0f,sbound=-1.0f,lbound=-1.0f,precomp=-1.0f;
			char options[50]="n";
			int result = sscanf(line,"%i, %i, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %s", &id1, &id2, &sin, &din, &psin, &pdin, &sout, &dout, &psout, &pdout, &sbound, &lbound, &precomp, options);

			if (result < 13 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Shock) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			// checks ...
			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_shock >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("shock limit reached ("+StringConverter::toString(MAX_SHOCKS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in shocks2 section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(4);
			}

			if ( sin == -1.0f || din == -1.0f || psin == -1.0f || pdin == -1.0f || sout == -1.0f || dout == -1.0f || psout == -1.0f || pdout == -1.0f || sbound == -1.0f || lbound == -1.0f || precomp == -1.0f)
			{
				LogManager::getSingleton().logMessage("Error: Wrong values in shocks2 section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(123);
			}

			// options
			int htype=BEAM_HYDRO;
			int shockflag = SHOCK_FLAG_NORMAL | SHOCK_FLAG_ISSHOCK2;

			// now 'parse' the options
			char *options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'i':
						// invisible
						htype=BEAM_INVISIBLE_HYDRO;
						shockflag |= SHOCK_FLAG_INVISIBLE;
						break;
					case 's':
						// passive shock
						shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
						shockflag |= SHOCK_FLAG_SOFTBUMP;
						break;
					case 'm':
						{
							// metric values: calculate sbound and lbound now
							float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
							sbound = sbound / beam_length;
							lbound = lbound / beam_length;
						}
					case 'M':
						{
							// metric values: calculate sbound and lbound now
							float beam_length = nodes[id1].AbsPosition.distance(nodes[id2].AbsPosition);
							sbound = (beam_length - sbound) / beam_length;
							lbound = (lbound - beam_length) / beam_length;

							if (lbound < 0)
							{
								LogManager::getSingleton().logMessage("Error parsing File (Shocks) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Metric shock length calculation failed, longbound less then beams spawn length, reset to beams spawn length (longbound=0).");
								lbound = 0.0f;
							}

							if (sbound > 1)
							{
								LogManager::getSingleton().logMessage("Error parsing File (Shocks) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Metric shock length calculation failed, shortbound less then 0 meters, reset to 0 meters (shortbound=1).");
								sbound = 1.0f;
							}
						}
						break;
				}
				options_pointer++;
			}
			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break*4.0, sin, din, -1.0, sbound, lbound, precomp);
			beams[pos].bounded=SHOCK2;
			beams[pos].shock = &shocks[free_shock];
			shocks[free_shock].springin = sin;
			shocks[free_shock].dampin = din;
			shocks[free_shock].sprogin = psin;
			shocks[free_shock].dprogin = pdin;
			shocks[free_shock].springout = sout;
			shocks[free_shock].dampout = dout;
			shocks[free_shock].sprogout = psout;
			shocks[free_shock].dprogout = pdout;
			shocks[free_shock].beamid = pos;
			shocks[free_shock].flags = shockflag;
			free_shock++;
		}

		else if (mode==3)
		{
			//parse fixes
			int id;
			int result = sscanf(line,"%i",&id);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Fixes) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (id>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in fixes section ("
					+StringConverter::toString(id)+")");
				exit(5);
			};
			nodes[id].locked=1;
			hasfixes=1;
		}
		else if (mode==5)
		{
			//parse hydros
			int id1, id2;
			float ratio;
			char options[50] = "n";
			Real startDelay=0;
			Real stopDelay=0;
			char startFunction[50]="";
			char stopFunction[50]="";

			int result = sscanf(line,"%i, %i, %f, %s %f, %f, %s %s",&id1,&id2,&ratio,options,&startDelay,&stopDelay,startFunction,stopFunction);
			if (result < 3 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Hydro) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			int htype=BEAM_HYDRO;

			// FIXME: separate init_beam and setup_beam to be able to set all parameters after creation
			// this is just ugly:
			char *options_pointer = options;
			while (*options_pointer != 0)
			{
				if(*options_pointer=='i')
				{
					htype=BEAM_INVISIBLE_HYDRO;
					break;
				}
				options_pointer++;
			}

			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in hydros section ("
					+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(6);
			};

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_hydro >= MAX_HYDROS)
			{
				LogManager::getSingleton().logMessage("hydros limit reached ("+StringConverter::toString(MAX_HYDROS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//            printf("beam : %i %i\n", id1, id2);

			if(hydroInertia && startDelay != 0 && stopDelay != 0)
				hydroInertia->setCmdKeyDelay(free_hydro,startDelay,stopDelay,String (startFunction), String (stopFunction));
			else if(hydroInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
				hydroInertia->setCmdKeyDelay(free_hydro,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));


			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale);
			hydro[free_hydro]=pos;free_hydro++;
			beams[pos].Lhydro=beams[pos].L;
			beams[pos].hydroRatio=ratio;
			beams[pos].animOption = 0.0f;


			// now 'parse' the options
			options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'i':	// invisible
						beams[pos].type = BEAM_INVISIBLE_HYDRO;
						break;
					case 'n':	// normal
						beams[pos].type = BEAM_HYDRO;
						beams[pos].hydroFlags |= HYDRO_FLAG_DIR;
						break;
					case 's': // speed changing hydro
						beams[pos].hydroFlags |= HYDRO_FLAG_SPEED;
						break;
					case 'a':
						beams[pos].hydroFlags |= HYDRO_FLAG_AILERON;
						break;
					case 'r':
						beams[pos].hydroFlags |= HYDRO_FLAG_RUDDER;
						break;
					case 'e':
						beams[pos].hydroFlags |= HYDRO_FLAG_ELEVATOR;
						break;
					case 'u':
						beams[pos].hydroFlags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_ELEVATOR);
						break;
					case 'v':
						beams[pos].hydroFlags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_ELEVATOR);
						break;
					case 'x':
						beams[pos].hydroFlags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_RUDDER);
						break;
					case 'y':
						beams[pos].hydroFlags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_RUDDER);
						break;
					case 'g':
						beams[pos].hydroFlags |= (HYDRO_FLAG_ELEVATOR | HYDRO_FLAG_RUDDER);
						break;
					case 'h':
						beams[pos].hydroFlags |= (HYDRO_FLAG_REV_ELEVATOR | HYDRO_FLAG_RUDDER);
						break;
				}
				options_pointer++;
				// if you use the i flag on its own, add the direction to it
				if(beams[pos].type == BEAM_INVISIBLE_HYDRO && !beams[pos].hydroFlags)
					beams[pos].hydroFlags |= HYDRO_FLAG_DIR;
			}
		}
		else if (mode==66)
		{
			//parse animators
			int id1, id2;
			float ratio;
			Ogre::vector<Ogre::String>::type options = Ogre::StringUtil::split(String(line), ",");
			if (options.size() < 4)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Animator) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			// the required values
			id1 = StringConverter::parseInt(options[0]);
			id2 = StringConverter::parseInt(options[1]);
			ratio = StringConverter::parseReal(options[2]);
			String optionStr = options[3];
			Ogre::vector<Ogre::String>::type optionArgs = Ogre::StringUtil::split(optionStr, "|");

			int htype=BEAM_HYDRO;

			// detect invisible beams
			for(unsigned int i=0;i<optionArgs.size();i++)
			{
				String arg = optionArgs[i];
				StringUtil::trim(arg);
				if(arg == "inv")
					htype=BEAM_INVISIBLE_HYDRO;
			}

			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in animators section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(6);
			}

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_hydro >= MAX_HYDROS)
			{
				LogManager::getSingleton().logMessage("hydros limit reached (via animators) ("+StringConverter::toString(MAX_HYDROS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(hydroInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
				hydroInertia->setCmdKeyDelay(free_hydro,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));

			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale);
			hydro[free_hydro]=pos;
			free_hydro++;
			beams[pos].Lhydro=beams[pos].L;
			beams[pos].hydroRatio=ratio;

			// parse the rest
			for(unsigned int i=0;i<optionArgs.size();i++)
			{
				String arg = optionArgs[i];
				StringUtil::trim(arg);
				beam_t *beam = &beams[pos];
				if      (arg == "vis")            { beam->type = BEAM_HYDRO; }
				else if (arg == "inv")            { htype=BEAM_INVISIBLE_HYDRO; }
				else if (arg == "airspeed")       { beam->animFlags |= (ANIM_FLAG_AIRSPEED); }
				else if (arg == "vvi")            { beam->animFlags |= (ANIM_FLAG_VVI); }
				else if (arg == "altimeter100k")  { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 1.0f; }
				else if (arg == "altimeter10k")   { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 2.0f; }
				else if (arg == "altimeter1k")    { beam->animFlags |= (ANIM_FLAG_ALTIMETER); beam->animOption = 3.0f; }
				else if (arg == "aoa")            { beam->animFlags |= (ANIM_FLAG_AOA); }
				else if (arg == "flap")           { beam->animFlags |= (ANIM_FLAG_FLAP); }
				else if (arg == "airbrake")       { beam->animFlags |= (ANIM_FLAG_AIRBRAKE); }
				else if (arg == "roll")           { beam->animFlags |= (ANIM_FLAG_ROLL); }
				else if (arg == "pitch")          { beam->animFlags |= (ANIM_FLAG_PITCH); }
				else if (arg == "throttle1")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 1.0f; }
				else if (arg == "throttle2")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 2.0f; }
				else if (arg == "throttle3")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 3.0f; }
				else if (arg == "throttle4")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 4.0f; }
				else if (arg == "throttle5")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 5.0f; }
				else if (arg == "throttle6")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 6.0f; }
				else if (arg == "throttle7")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 7.0f; }
				else if (arg == "throttle8")      { beam->animFlags |= (ANIM_FLAG_THROTTLE); beam->animOption = 8.0f; }
				else if (arg == "rpm1")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 1.0f; }
				else if (arg == "rpm2")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 2.0f; }
				else if (arg == "rpm3")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 3.0f; }
				else if (arg == "rpm4")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 4.0f; }
				else if (arg == "rpm5")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 5.0f; }
				else if (arg == "rpm6")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 6.0f; }
				else if (arg == "rpm7")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 7.0f; }
				else if (arg == "rpm8")           { beam->animFlags |= (ANIM_FLAG_RPM); beam->animOption = 8.0f; }
				else if (arg == "aerotorq1")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 1.0f; }
				else if (arg == "aerotorq2")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 2.0f; }
				else if (arg == "aerotorq3")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 3.0f; }
				else if (arg == "aerotorq4")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 4.0f; }
				else if (arg == "aerotorq5")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 5.0f; }
				else if (arg == "aerotorq6")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 6.0f; }
				else if (arg == "aerotorq7")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 7.0f; }
				else if (arg == "aerotorq8")      { beam->animFlags |= (ANIM_FLAG_AETORQUE); beam->animOption = 8.0f; }
				else if (arg == "aeropit1")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 1.0f; }
				else if (arg == "aeropit2")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 2.0f; }
				else if (arg == "aeropit3")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 3.0f; }
				else if (arg == "aeropit4")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 4.0f; }
				else if (arg == "aeropit5")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 5.0f; }
				else if (arg == "aeropit6")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 6.0f; }
				else if (arg == "aeropit7")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 7.0f; }
				else if (arg == "aeropit8")       { beam->animFlags |= (ANIM_FLAG_AEPITCH); beam->animOption = 8.0f; }
				else if (arg == "aerostatus1")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 1.0f; }
				else if (arg == "aerostatus2")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 2.0f; }
				else if (arg == "aerostatus3")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 3.0f; }
				else if (arg == "aerostatus4")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 4.0f; }
				else if (arg == "aerostatus5")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 5.0f; }
				else if (arg == "aerostatus6")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 6.0f; }
				else if (arg == "aerostatus7")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 7.0f; }
				else if (arg == "aerostatus8")    { beam->animFlags |= (ANIM_FLAG_AESTATUS); beam->animOption = 8.0f; }
				else if (arg == "brakes")         { beam->animFlags |= (ANIM_FLAG_BRAKE); }
				else if (arg == "accel")          { beam->animFlags |= (ANIM_FLAG_ACCEL); }
				else if (arg == "clutch")         { beam->animFlags |= (ANIM_FLAG_CLUTCH); }
				else if (arg == "speedo")         { beam->animFlags |= (ANIM_FLAG_SPEEDO); }
				else if (arg == "tacho")          { beam->animFlags |= (ANIM_FLAG_TACHO); }
				else if (arg == "turbo")          { beam->animFlags |= (ANIM_FLAG_TURBO); }
				else if (arg == "parking")        { beam->animFlags |= (ANIM_FLAG_PBRAKE); }
				else if (arg == "shifterman1")    { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 1.0f; }
				else if (arg == "shifterman2")    { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 2.0f; }
				else if (arg == "seqential")      { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 3.0f; }
				else if (arg == "shifterlin")     { beam->animFlags |= (ANIM_FLAG_SHIFTER); beam->animOption= 4.0f; }
				else if (arg == "torque")         { beam->animFlags |= (ANIM_FLAG_TORQUE); }
				else if (arg == "throttleboat")   { beam->animFlags |= (ANIM_FLAG_BTHROTTLE); }
				else if (arg == "rudderboat")     { beam->animFlags |= (ANIM_FLAG_BRUDDER); }

				if (beam->animFlags == 0)
					LogManager::getSingleton().logMessage("Error parsing File (animators) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Failed to identify source.");
				else
					LogManager::getSingleton().logMessage("Animator source set: with flag " +StringConverter::toString(beams[pos].animFlags));
			}

		}

		else if (mode==6)
		{
			//parse wheels
			float radius, width, mass, spring, damp;
			char texf[256];
			char texb[256];
			int rays, node1, node2, snode, braked, propulsed, torquenode;
			int result = sscanf(line,"%f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %s %s",&radius,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp, texf, texb);
			if (result < 14 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Wheel) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			addWheel(manager, parent, radius,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, texf, texb);
		}
		else if (mode==31)
		{
			//parse wheels2
			char texf[256];
			char texb[256];
			float radius, radius2, width, mass, spring, damp, spring2, damp2;
			int rays, node1, node2, snode, braked, propulsed, torquenode;
			int result = sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %f, %f, %s %s",&radius,&radius2,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp,&spring2,&damp2, texf, texb);
			if (result < 17 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Wheel2) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (enable_wheel2)
				addWheel2(manager, parent, radius,radius2,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, spring2, damp2, texf, texb);
			else
				addWheel(manager, parent, radius2,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring2, damp2, texf, texb);
		}
		else if (mode==42)
		{
			//parse meshwheels
			char meshw[256];
			char texb[256];
			float radius, rimradius, width, mass, spring, damp;
			char side;
			int rays, node1, node2, snode, braked, propulsed, torquenode;
			int result = sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %c, %s %s",&radius,&rimradius,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp, &side, meshw, texb);
			if (result < 16 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (MeshWheel) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			addWheel(manager, parent, radius,width,rays,node1,node2,snode,braked,propulsed, torquenode, mass, spring, damp, meshw, texb, true, rimradius, side!='r');
		}
		else if (mode==7)
		{
			//parse globals
			int result = sscanf(line,"%f, %f, %s",&truckmass, &loadmass, texname);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Globals) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//
			LogManager::getSingleton().logMessage("BEAM: line: '"+String(line)+"'");
			LogManager::getSingleton().logMessage("BEAM: texname: '"+String(texname)+"'");


			// check for skins
			if(usedSkin && usedSkin->hasReplacementForMaterial(texname))
			{
				// yay, we use a skin :D
				strncpy(texname, usedSkin->getReplacementForMaterial(texname).c_str(), 1024);
			}

			//we clone the material
			char clonetex[256];
			sprintf(clonetex, "%s-%s",texname,truckname);
			MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().getByName(texname));
			if(mat.getPointer() == 0)
			{
				LogManager::getSingleton().logMessage("Material '" + String(texname) + "' used in Section 'globals' not found! We will try to use the material 'tracks/black' instead." + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				mat=(MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/black"));
				if(mat.getPointer() == 0)
				{
					LogManager::getSingleton().logMessage("Material not found! Try to ensure that tracks/black exists and retry.");
					exit(124);
				}
			}
			mat->clone(clonetex);
			strcpy(texname, clonetex);
		}
		else if (mode==8)
		{
			//parse cameras
			int nodepos, nodedir, dir;
			int result = sscanf(line,"%i, %i, %i",&nodepos,&nodedir,&dir);
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Camera) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			addCamera(nodepos, nodedir, dir);
		}
		else if (mode==9)
		{
			//parse engine
			if(driveable == MACHINE)
				continue;

			driveable=TRUCK;
			Ogre::vector<Ogre::String>::type tmp = Ogre::StringUtil::split(line, ", ");
			if (tmp.size() < 7) {
				LogManager::getSingleton().logMessage("Error parsing File (Engine) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			float minrpm = Ogre::StringConverter::parseReal(tmp[0]);
			tmp.erase(tmp.begin());
			float maxrpm = Ogre::StringConverter::parseReal(tmp[0]);
			tmp.erase(tmp.begin());
			float torque = Ogre::StringConverter::parseReal(tmp[0]);
			tmp.erase(tmp.begin());
			float dratio = Ogre::StringConverter::parseReal(tmp[0]);
			tmp.erase(tmp.begin());

			std::vector<float> gears;

			for(Ogre::vector< Ogre::String >::type::iterator it = tmp.begin(); it != tmp.end(); it++)
			{
				float tmpf = Ogre::StringConverter::parseReal((*it), -1.0);
				if( tmpf <= 0.0f ) break;
				gears.push_back(tmpf);
			}
			if (gears.size() < 5) // 5 -2 = 3, 2 extra gears that don't count, one for reverse and one for neutral
			{
				LogManager::getSingleton().logMessage("Trucks with less than 3 gears are not supported! " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//if (audio) audio->setupEngine();

			engine = new BeamEngine(minrpm, maxrpm, torque, gears, dratio, trucknum);

			// are there any startup shifter settings?
			Ogre::String gearboxMode = SETTINGS.getSetting("GearboxMode");
			if (gearboxMode == "Manual shift - Auto clutch")
				engine->setAutoMode(SEMIAUTO);
			else if (gearboxMode == "Fully Manual: sequential shift")
				engine->setAutoMode(MANUAL);
			else if (gearboxMode == "Fully Manual: stick shift")
				engine->setAutoMode(MANUAL_STICK);
			else if (gearboxMode == "Fully Manual: stick shift with ranges")
				engine->setAutoMode(MANUAL_RANGES);
			
			//engine->start();
		}

		else if (mode==10)
		{
			//parse texcoords
			int id;
			float x, y;
			int result = sscanf(line,"%i, %f, %f", &id, &x, &y);
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (TexCoords) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_texcoord >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("texcoords limit reached ("+StringConverter::toString(MAX_TEXCOORDS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			texcoords[free_texcoord]=Vector3(id, x, y);
			free_texcoord++;
		}

		else if (mode==11)
		{
			//parse cab
			char type='n';
			int id1, id2, id3;
			int result = sscanf(line,"%i, %i, %i, %c", &id1, &id2, &id3,&type);
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Cab) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(free_cab >= MAX_CABS)
			{
				LogManager::getSingleton().logMessage("cabs limit reached ("+StringConverter::toString(MAX_CABS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			cabs[free_cab*3]=id1;
			cabs[free_cab*3+1]=id2;
			cabs[free_cab*3+2]=id3;
			if(free_collcab >= MAX_CABS)
			{
				LogManager::getSingleton().logMessage("unable to create cabs: cabs limit reached ("+StringConverter::toString(MAX_CABS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (type=='c') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=0; free_collcab++;};
			if (type=='p') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=1; free_collcab++;};
			if (type=='u') {collcabs[free_collcab]=free_cab; collcabstype[free_collcab]=2; free_collcab++;};
			if (type=='b') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=BUOY_NORMAL; free_buoycab++;   if (!buoyance) buoyance=new Buoyance(water);};
			if (type=='r') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=BUOY_DRAGONLY; free_buoycab++; if (!buoyance) buoyance=new Buoyance(water);};
			if (type=='s') {buoycabs[free_buoycab]=free_cab; collcabstype[free_collcab]=0; buoycabtypes[free_buoycab]=BUOY_DRAGLESS; free_buoycab++; if (!buoyance) buoyance=new Buoyance(water);};
			if (type=='D' || type == 'F' || type == 'S')
			{

				if(free_collcab >= MAX_CABS || free_buoycab >= MAX_CABS)
				{
					LogManager::getSingleton().logMessage("unable to create buoycabs: cabs limit reached ("+StringConverter::toString(MAX_CABS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				collcabs[free_collcab]=free_cab;
				collcabstype[free_collcab]=0;
				if(type == 'F') collcabstype[free_collcab]=1;
				if(type == 'S') collcabstype[free_collcab]=2;
				free_collcab++;
				buoycabs[free_buoycab]=free_cab; buoycabtypes[free_buoycab]=BUOY_NORMAL; free_buoycab++; if (!buoyance) buoyance=new Buoyance(water);
			}
			free_cab++;
		}

		else if (mode==12 || mode==120)
		{
			//parse commands
			int id1, id2,keys,keyl;
			float rateShort, rateLong, shortl, longl;
			char options[250]="";
			char descr[200] = "";
			hascommands=1;
			Real startDelay=0;
			Real stopDelay=0;
			char startFunction[50]="";
			char stopFunction[50]="";
			int result = 0;
			if(mode == 12)
			{
				char opt='n';
				result = sscanf(line,"%i, %i, %f, %f, %f, %i, %i, %c, %s %f, %f, %s %s", &id1, &id2, &rateShort, &shortl, &longl, &keys, &keyl, &opt, descr, &startDelay, &stopDelay, startFunction, stopFunction);
				if (result < 7 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Command) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				options[0] = opt;
				options[1] = 0;
				rateLong = rateShort;
			}
			else if(mode == 120)
			{
				result = sscanf(line,"%i, %i, %f, %f, %f, %f, %i, %i, %s %s %f,%f,%s %s", &id1, &id2, &rateShort, &rateLong, &shortl, &longl, &keys, &keyl, options, descr, &startDelay, &stopDelay, startFunction, stopFunction);
				if (result < 8 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Command) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//LogManager::getSingleton().logMessage("added command2: " + String(options)+ ", " + String(descr));
				//LogManager::getSingleton().logMessage(String(line));
			}

			int htype=BEAM_HYDRO;

			char *options_pointer = options;
			while (*options_pointer != 0) {
				if(*options_pointer=='i') {
					htype=BEAM_INVISIBLE_HYDRO;
					break;
				}
				options_pointer++;
			}

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("cannot create command: beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale, -1, -1, -1, 1, default_beam_diameter);
			// now 'parse' the options
			options_pointer = options;
			while (*options_pointer != 0)
			{
				switch (*options_pointer)
				{
					case 'r':
						beams[pos].bounded=ROPE;
						break;
					case 'c':
						if(beams[pos].isOnePressMode>0)
						{
							LogManager::getSingleton().logMessage("Command cannot be one-pressed and self centering at the same time!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						beams[pos].iscentering=true;
						break;
					case 'p':
						if(beams[pos].iscentering)
						{
							LogManager::getSingleton().logMessage("Command cannot be one-pressed and self centering at the same time!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						if(beams[pos].isOnePressMode>0)
						{
							LogManager::getSingleton().logMessage("Command already has a one-pressed mode! All after the first are ignored!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						beams[pos].isOnePressMode=1;
						break;
					case 'o':
						if(beams[pos].iscentering)
						{
							LogManager::getSingleton().logMessage("Command cannot be one-pressed and self centering at the same time!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						if(beams[pos].isOnePressMode>0)
						{
							LogManager::getSingleton().logMessage("Command already has a one-pressed mode! All after the first are ignored!" + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							break;
						}
						beams[pos].isOnePressMode=2;
						break;
					case 'f':
						beams[pos].isforcerestricted=true;
						break;
				}
				options_pointer++;
			}

			beams[pos].Lhydro=beams[pos].L;

			//add short key
			commandkey[keys].beams.push_back(-pos);
			char *descr_pointer = descr;
			//replace '_' with ' '
			while (*descr_pointer!=0) {if (*descr_pointer=='_') *descr_pointer=' ';descr_pointer++;};

			if(strlen(descr) != 0)
				commandkey[keys].description = String(descr);
			else if (strlen(descr) == 0 && commandkey[keys].description.size() == 0)
				commandkey[keys].description = "";

			//add long key
			commandkey[keyl].beams.push_back(pos);
			if(strlen(descr) != 0)
				commandkey[keyl].description = String(descr);
			else if (strlen(descr) == 0 && commandkey[keyl].description.size() == 0)
				commandkey[keyl].description = "";

			LogManager::getSingleton().logMessage("added command: short=" + StringConverter::toString(keys)+ ", long=" + StringConverter::toString(keyl) + ", descr=" + (descr));
			beams[pos].commandRatioShort=rateShort;
			beams[pos].commandRatioLong=rateLong;
			beams[pos].commandShort=shortl;
			beams[pos].commandLong=longl;

			// set the middle of the command, so its not required to recalculate this everytime ...
			if(beams[pos].commandLong > beams[pos].commandShort)
				beams[pos].centerLength = (beams[pos].commandLong-beams[pos].commandShort)/2 + beams[pos].commandShort;
			else
				beams[pos].centerLength = (beams[pos].commandShort-beams[pos].commandLong)/2 + beams[pos].commandLong;

			if(cmdInertia && startDelay > 0 && stopDelay > 0)
			{
				cmdInertia->setCmdKeyDelay(keys,startDelay,stopDelay,String (startFunction),String (stopFunction));
				cmdInertia->setCmdKeyDelay(keyl,startDelay,stopDelay,String (startFunction),String (stopFunction));
			}
			else if(cmdInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
			{
				cmdInertia->setCmdKeyDelay(keys,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
				cmdInertia->setCmdKeyDelay(keyl,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
			}

		}
		else if (mode==13)
		{
			//parse contacters
			int id1;
			int result = sscanf(line,"%i", &id1);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Contacters) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_contacter >= MAX_CONTACTERS)
			{
				LogManager::getSingleton().logMessage("contacters limit reached ("+StringConverter::toString(MAX_CONTACTERS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			contacters[free_contacter].nodeid=id1;
			contacters[free_contacter].contacted=0;
			contacters[free_contacter].opticontact=0;
			nodes[id1].iIsSkin=true;
			free_contacter++;;
		}
		else if (mode==14)
		{
			//parse ropes
			int id1=0, id2=0, key=0, group=0;
			int result = sscanf(line,"%i, %i, %i, %i", &id1, &id2, &key, &group);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Ropes) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//add beam
			if (id1>=free_node || id2>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in ropes section ("
					+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
				exit(7);
			};

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("cannot create rope: beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			int pos=add_beam(&nodes[id1], &nodes[id2], manager, parent, BEAM_NORMAL, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale);
			beams[pos].bounded=ROPE;

			rope_t r;
			//register rope
			r.beam=&beams[pos];
			r.lockedto=0;
			r.group=group;
			ropes.push_back(r);

			nodes[id1].iIsSkin=true;
			nodes[id2].iIsSkin=true;
		}
		else if (mode==15)
		{
			//parse ropables
			int id1=0, group=-1, multilock=0;
			int result = sscanf(line,"%i, %i, %i", &id1, &group, &multilock);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Ropeable) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (id1>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in ropables section ("+StringConverter::toString(id1)+")");
				exit(7);
			}

			ropable_t r;
			r.node = &nodes[id1];
			r.group = group;
			r.used = 0;
			r.multilock = (bool)(multilock == 1);
			ropables.push_back(r);

			nodes[id1].iIsSkin=true;
		}
		else if (mode==16)
		{
			//parse ties
			int id1=0, group=-1;
			float maxl, rate, shortl, longl, maxstress;
			char option='n';
			maxstress=100000.0f;
			hascommands=1;
			int result = sscanf(line,"%i, %f, %f, %f, %f, %c, %f, %i", &id1, &maxl, &rate, &shortl, &longl, &option, &maxstress, &group);
			if (result < 5 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Ties) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("cannot create tie: beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			int htype=BEAM_HYDRO;
			if (option=='i') htype=BEAM_INVISIBLE_HYDRO;
			int pos=add_beam(&nodes[id1], &nodes[0], manager, parent, htype, default_break * default_break_scale, default_spring * default_spring_scale, default_damp * default_damp_scale);
			beams[pos].L=maxl;
			beams[pos].refL=maxl;
			beams[pos].Lhydro=maxl;
			beams[pos].bounded=ROPE;
			beams[pos].disabled=true;
			beams[pos].mSceneNode->detachAllObjects();
			beams[pos].commandRatioShort=rate;
			beams[pos].commandRatioLong=rate;
			beams[pos].commandShort=shortl;
			beams[pos].commandLong=longl;
			beams[pos].maxtiestress=maxstress;

			//register tie
			tie_t t;
			t.group = group;
			t.tying=false;
			t.tied=false;
			t.beam=&beams[pos];
			ties.push_back(t);
		}

		else if (mode==17)
		{
			//help material
			strcpy(helpmat,line);
			hashelp=1;
		}
		else if (mode==18)
		{
			//cinecam
			float x,y,z;
			int n1, n2, n3, n4, n5, n6, n7, n8;
			float spring=8000.0;
			float damp=800.0;
			int result = sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %i, %f, %f", &x,&y,&z,&n1,&n2,&n3,&n4,&n5,&n6,&n7,&n8, &spring, &damp);
			if (result < 11 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Cinecam) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_beam >= MAX_BEAMS)
			{
				LogManager::getSingleton().logMessage("cannot create cinecam: beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_node >= MAX_NODES)
			{
				LogManager::getSingleton().logMessage("cannot create cinecam: nodes limit reached ("+StringConverter::toString(MAX_NODES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			//add node
			cinecameranodepos[freecinecamera]=free_node;
			Vector3 npos=Vector3(px, py, pz)+rot*Vector3(x,y,z);
			init_node(cinecameranodepos[freecinecamera], npos.x, npos.y, npos.z);
//				init_node(cinecameranodepos[freecinecamera], px+x*cos(ry)+z*sin(ry), py+y , pz+x*cos(ry+3.14159/2.0)+z*sin(ry+3.14159/2.0));
			nodes[free_node].contactless = 1;
			free_node++;

			//add beams
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n1], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n2], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n3], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n4], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n5], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n6], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n7], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);
			add_beam(&nodes[cinecameranodepos[freecinecamera]], &nodes[n8], manager, parent, BEAM_INVISIBLE, default_break, spring, damp);

			if(flaresMode>=2 && !cablight)
			{
				// create cabin light :)
				char flarename[256];
				sprintf(flarename, "cabinglight-%s", truckname);
				cablight=manager->createLight(flarename);
				cablight->setType(Light::LT_POINT);
				cablight->setDiffuseColour( ColourValue(0.4, 0.4, 0.3));
				cablight->setSpecularColour( ColourValue(0.4, 0.4, 0.3));
				cablight->setAttenuation(20, 1, 0, 0);
				cablight->setCastShadows(false);
				cablight->setVisible(true);
				cablightNode = manager->getRootSceneNode()->createChildSceneNode();
				deletion_sceneNodes.push_back(cablightNode);
				if(cablight)
					cablightNode->attachObject(cablight);
				cablightNode->setVisible(false);
			}

			freecinecamera++;
		}

		else if (mode==19 || mode==65)
		{
			if(flaresMode==0)
				continue;
			//parse flares
			int ref=-1, nx=0, ny=0, controlnumber=-1, blinkdelay=-2;
			float ox=0, oy=0, oz=1, size=-2;
			char type='f';
			char matname[255]="";
			int result=-1;
			if(mode == 19)
			{
				// original flares
				result = sscanf(line,"%i, %i, %i, %f, %f, %c, %i, %i, %f %s", &ref, &nx, &ny, &ox, &oy, &type, &controlnumber, &blinkdelay, &size, matname);
				if (result < 5 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
			} else if(mode == 65)
			{
				// flares 2
				result = sscanf(line,"%i, %i, %i, %f, %f, %f, %c, %i, %i, %f %s", &ref, &nx, &ny, &ox, &oy, &oz, &type, &controlnumber, &blinkdelay, &size, matname);
				if (result < 5 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}

			}

			// check validity
			// b = brake light
			// f = front light
			// l = left blink light
			// r = right blink light
			// R = reverse light
			// u = user controlled light (i.e. fog light) (use controlnumber later)
			if (type != 'f' && type != 'b' && type != 'l' && type != 'r' && type != 'R' && type != 'u')
				type = 'f';

			// backwards compatibility
			if(blinkdelay == -2 && (type == 'l' || type == 'r'))
				// default blink
				blinkdelay = -1;
			else if(blinkdelay == -2 && !(type == 'l' || type == 'r'))
				//default no blink
				blinkdelay = 0;
			if(size == -2 && type == 'f')
				size = 1;
			else if ((size == -2 && type != 'f') || size == -1)
				size = 0.5;

			//LogManager::getSingleton().logMessage(StringConverter::toString(controlnumber));
			if(controlnumber < -1 || controlnumber > 500)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Controlnumber must be between -1 and 500! trying to continue ...");
				continue;
			}
			//LogManager::getSingleton().logMessage(StringConverter::toString(blinkdelay));
			if(blinkdelay < -1 || blinkdelay > 60000)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Flares) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Blinkdelay must be between 0 and 60000! trying to continue ...");
				continue;
			}
			flare_t f;
			f.type = type;
			f.controlnumber = controlnumber;
			f.controltoggle_status = false;
			if(blinkdelay == -1)
				f.blinkdelay = 0.5f;
			else
				f.blinkdelay = (float)blinkdelay / 1000.0f;
			f.blinkdelay_curr=0.0f;
			f.blinkdelay_state=false;
			f.noderef=ref;
			f.nodex=nx;
			f.nodey=ny;
			f.offsetx=ox;
			f.offsety=oy;
			f.offsetz=oz;
			f.size=size;
			f.snode = manager->getRootSceneNode()->createChildSceneNode();
			char flarename[256];
			sprintf(flarename, "flare-%s-%i", truckname, free_flare);
			f.bbs=manager->createBillboardSet(flarename,1);
			f.bbs->createBillboard(0,0,0);
			bool usingDefaultMaterial=true;
			if (f.bbs && (!strncmp(matname, "default", 250) || strnlen(matname, 250) == 0))
			{
				if (type == 'b')
					f.bbs->setMaterialName("tracks/brakeflare");
				else if (type == 'l' || type == 'r')
					f.bbs->setMaterialName("tracks/blinkflare");
				else // (type == 'f' || type == 'R')
					f.bbs->setMaterialName("tracks/flare");
			} else
			{
				usingDefaultMaterial=false;
				if(f.bbs)
					f.bbs->setMaterialName(String(matname));
			}
			if(f.bbs)
				f.snode->attachObject(f.bbs);
			f.isVisible=true;
			f.light=NULL;
			//LogManager::getSingleton().logMessage("Blinkdelay2: " + StringConverter::toString(blinkdelay));
			if (type == 'f' && usingDefaultMaterial && flaresMode >=2 && size > 0.001)
			{
				// front light
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1, 1, 1));
				f.light->setSpecularColour( ColourValue(1, 1, 1));
				f.light->setAttenuation(400, 0.9, 0, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if (type == 'f' && !usingDefaultMaterial && flaresMode >=4 && size > 0.001)
			{
				// this is a quick fix for the red backlight when frontlight is switched on
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1.0, 0, 0));
				f.light->setSpecularColour( ColourValue(1.0, 0, 0));
				f.light->setAttenuation(10.0, 1.0, 0, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if (type == 'R' && flaresMode >= 4 && size > 0.001)
			{
				// brake light
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour(ColourValue(1, 1, 1));
				f.light->setSpecularColour(ColourValue(1, 1, 1));
				f.light->setAttenuation(20.0, 1, 0, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if (type == 'b' && flaresMode >= 4 && size > 0.001)
			{
				// brake light
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1.0, 0, 0));
				f.light->setSpecularColour( ColourValue(1.0, 0, 0));
				f.light->setAttenuation(10.0, 1.0, 0, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if ((type == 'l' || type == 'r') && flaresMode >= 4 && size > 0.001)
			{
				// blink light
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1, 1, 0));
				f.light->setSpecularColour( ColourValue(1, 1, 0));
				f.light->setAttenuation(10.0, 1, 1, 0);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}
			else if ((type == 'u') && flaresMode >= 4 && size > 0.001)
			{
				// user light always white (TODO: improve this)
				f.light=manager->createLight(flarename);
				f.light->setType(Light::LT_SPOTLIGHT);
				f.light->setDiffuseColour( ColourValue(1, 1, 1));
				f.light->setSpecularColour( ColourValue(1, 1, 1));
				f.light->setAttenuation(50.0, 1.0, 1, 0.2);
				f.light->setSpotlightRange( Degree(35), Degree(45) );
				f.light->setCastShadows(false);
			}

			// update custom light array
			if(type == 'u' && netCustomLightArray_counter < 4)
			{
				netCustomLightArray[netCustomLightArray_counter] = free_flare;
				netCustomLightArray_counter++;
			}
			flares.push_back(f);
			free_flare++;
		}
		else if (mode==20)
		{
			//parse props
			int ref, nx, ny;
			float ox, oy, oz;
			float rx, ry, rz;
			char meshname[256];
			int result = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname);
			if (result < 10 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Prop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_prop >= MAX_PROPS)
			{
				LogManager::getSingleton().logMessage("props limit reached ("+StringConverter::toString(MAX_PROPS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			/* Initialize prop memory to avoid invalid pointers. */
			memset(&props[free_prop], 0, sizeof props[free_prop]);
			props[free_prop].noderef=ref;
			props[free_prop].nodex=nx;
			props[free_prop].nodey=ny;
			props[free_prop].offsetx=ox;
			props[free_prop].offsety=oy;
			props[free_prop].offsetz=oz;
			props[free_prop].orgoffsetX=ox;
			props[free_prop].orgoffsetY=oy;
			props[free_prop].orgoffsetZ=oz;
			props[free_prop].rotaX=rx;
			props[free_prop].rotaY=ry;
			props[free_prop].rotaZ=rz;
			props[free_prop].rot=Quaternion(Degree(rz), Vector3::UNIT_Z);
			props[free_prop].rot=props[free_prop].rot*Quaternion(Degree(ry), Vector3::UNIT_Y);
			props[free_prop].rot=props[free_prop].rot*Quaternion(Degree(rx), Vector3::UNIT_X);
			props[free_prop].wheel=0;
			props[free_prop].mirror=0;
			props[free_prop].pale=0;
			props[free_prop].spinner=0;
			props[free_prop].wheelrotdegree=160.0;
			//set no animation by default
			props[free_prop].animFlags[0]=0;
			props[free_prop].animMode[0]=0;
			props[free_prop].animKey[0]=-1;
			props[free_prop].animKeyState[0]=-1.0f;
			String meshnameString = String(meshname);
			std::string::size_type loc = meshnameString.find("leftmirror", 0);
			if( loc != std::string::npos ) props[free_prop].mirror=1;

			loc = meshnameString.find("rightmirror", 0);
			if( loc != std::string::npos ) props[free_prop].mirror=-1;

			loc = meshnameString.find("dashboard", 0);
			if( loc != std::string::npos )
			{
				char dirwheelmeshname[256];
				float dwx=0, dwy=0, dwz=0;
				Real rotdegrees=160;
				Vector3 stdpos = Vector3(-0.67, -0.61,0.24);
				if (!strncmp("dashboard-rh", meshname, 12))
					stdpos=Vector3(0.67, -0.61,0.24);
				String diwmeshname = "dirwheel.mesh";
				int result2 = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s %s %f, %f, %f, %f", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname, dirwheelmeshname, &dwx, &dwy, &dwz, &rotdegrees);
				if(result2 != result && result2 >= 14)
				{
					stdpos = Vector3(dwx, dwy, dwz);
					diwmeshname = String(dirwheelmeshname);
				}
				if(result2 != result && result2 >= 15) props[free_prop].wheelrotdegree=rotdegrees;
				props[free_prop].wheelpos=stdpos;

				// create the meshs scenenode
				props[free_prop].wheel = manager->getRootSceneNode()->createChildSceneNode();
				// now create the mesh
				MeshObject *mo = new MeshObject(manager, diwmeshname, "", props[free_prop].wheel, enable_background_loading);
				mo->setSimpleMaterialColour(ColourValue(0, 0.5, 0.5));
				mo->setSkin(usedSkin);
				mo->setMaterialFunctionMapper(materialFunctionMapper);
			}

			// create the meshs scenenode
			props[free_prop].snode = manager->getRootSceneNode()->createChildSceneNode();
			// now create the mesh
			MeshObject *mo = new MeshObject(manager, meshname, "", props[free_prop].snode, enable_background_loading);
			mo->setSimpleMaterialColour(ColourValue(1, 1, 0));
			mo->setSkin(usedSkin);
			mo->setMaterialFunctionMapper(materialFunctionMapper);

			//hack for the spinprops
			if (!strncmp("spinprop", meshname, 8))
			{
				props[free_prop].spinner=1;
				mo->setCastShadows(false);
				props[free_prop].snode->setVisible(false);
			}
			if (!strncmp("pale", meshname, 4))
			{
				props[free_prop].pale=1;
			}
			//detect driver seat, used to position the driver and make the seat translucent at times
			if (!strncmp("seat", meshname, 4) && !driversseatfound)
			{
				driversseatfound=true;
				mo->setMaterialName("driversseat");
				driverSeat = &props[free_prop];
			}
			else if (!strncmp("seat2", meshname, 5) && !driversseatfound)
			{
				driversseatfound=true;
				driverSeat = &props[free_prop];
			}
			props[free_prop].beacontype='n';
			if (!strncmp("beacon", meshname, 6) && flaresMode>0)
			{
				ColourValue color = ColourValue(1.0, 0.5, 0.0);
				String matname = "tracks/beaconflare";
				char beaconmaterial[256];
				float br=0, bg=0, bb=0;
				int result2 = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s %s %f, %f, %f", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname, beaconmaterial, &br, &bg, &bb);
				if(result2 != result && result2 >= 14)
				{
					color = ColourValue(br, bg, bb);
					matname = String(beaconmaterial);
				}

				props[free_prop].bpos[0]=2.0*3.14*((Real)rand()/(Real)RAND_MAX);
				props[free_prop].brate[0]=4.0*3.14+((Real)rand()/(Real)RAND_MAX)-0.5;
				props[free_prop].beacontype='b';
				props[free_prop].bbs[0]=0;
				//the light
				props[free_prop].light[0]=manager->createLight(); //propname);
				props[free_prop].light[0]->setType(Light::LT_SPOTLIGHT);
				props[free_prop].light[0]->setDiffuseColour(color);
				props[free_prop].light[0]->setSpecularColour(color);
				props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
				props[free_prop].light[0]->setSpotlightRange( Degree(35), Degree(45) );
				props[free_prop].light[0]->setCastShadows(false);
				props[free_prop].light[0]->setVisible(false);
				//the flare billboard
				props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
				props[free_prop].bbs[0]=manager->createBillboardSet(1); //(propname,1);
				props[free_prop].bbs[0]->createBillboard(0,0,0);
				if(props[free_prop].bbs[0])
					props[free_prop].bbs[0]->setMaterialName(matname);
				if(props[free_prop].bbs[0])
					props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
				props[free_prop].bbsnode[0]->setVisible(false);
			}
			if (!strncmp("redbeacon", meshname, 9) && flaresMode>0)
			{
				props[free_prop].bpos[0]=0.0;
				props[free_prop].brate[0]=1.0;
				props[free_prop].beacontype='r';
				props[free_prop].bbs[0]=0;
				//the light
				props[free_prop].light[0]=manager->createLight();//propname);
				props[free_prop].light[0]->setType(Light::LT_POINT);
				props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 0.0, 0.0));
				props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 0.0, 0.0));
				props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
				props[free_prop].light[0]->setCastShadows(false);
				props[free_prop].light[0]->setVisible(false);
				//the flare billboard
				props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
				props[free_prop].bbs[0]=manager->createBillboardSet(1); //propname,1);
				props[free_prop].bbs[0]->createBillboard(0,0,0);
				if(props[free_prop].bbs[0])
					props[free_prop].bbs[0]->setMaterialName("tracks/redbeaconflare");
				if(props[free_prop].bbs[0])
					props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
				props[free_prop].bbsnode[0]->setVisible(false);
				props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
			}
			if (!strncmp("lightbar", meshname, 6) && flaresMode>0)
			{
				int k;
				ispolice=true;
				props[free_prop].beacontype='p';
				for (k=0; k<4; k++)
				{
					props[free_prop].bpos[k]=2.0*3.14*((Real)rand()/(Real)RAND_MAX);
					props[free_prop].brate[k]=4.0*3.14+((Real)rand()/(Real)RAND_MAX)-0.5;
					props[free_prop].bbs[k]=0;
					//the light
					//char rpname[256];
					//sprintf(rpname,"%s-%i", propname, k);
					props[free_prop].light[k]=manager->createLight(); //rpname);
					props[free_prop].light[k]->setType(Light::LT_SPOTLIGHT);
					if (k>1)
					{
						props[free_prop].light[k]->setDiffuseColour( ColourValue(1.0, 0.0, 0.0));
						props[free_prop].light[k]->setSpecularColour( ColourValue(1.0, 0.0, 0.0));
					}
					else
					{
						props[free_prop].light[k]->setDiffuseColour( ColourValue(0.0, 0.5, 1.0));
						props[free_prop].light[k]->setSpecularColour( ColourValue(0.0, 0.5, 1.0));
					}
					props[free_prop].light[k]->setAttenuation(50.0, 1.0, 0.3, 0.0);
					props[free_prop].light[k]->setSpotlightRange( Degree(35), Degree(45) );
					props[free_prop].light[k]->setCastShadows(false);
					props[free_prop].light[k]->setVisible(false);
					//the flare billboard
					props[free_prop].bbsnode[k] = manager->getRootSceneNode()->createChildSceneNode();
					props[free_prop].bbs[k]=manager->createBillboardSet(1); //rpname,1);
					props[free_prop].bbs[k]->createBillboard(0,0,0);
					if(props[free_prop].bbs[k])
					{
						if (k>1)
							props[free_prop].bbs[k]->setMaterialName("tracks/brightredflare");
						else
							props[free_prop].bbs[k]->setMaterialName("tracks/brightblueflare");
						if(props[free_prop].bbs[k])
							props[free_prop].bbsnode[k]->attachObject(props[free_prop].bbs[k]);
					}
					props[free_prop].bbsnode[k]->setVisible(false);
				}
			}

			//set no animation by default
			props[free_prop].animFlags[0]=0;
			props[free_prop].animMode[0]=0;

			free_prop++;
		}
		else if (mode==21)
		{
			//parse globeams
			int result = sscanf(line,"%f, %f, %f, %s", &default_deform,&default_break,&default_beam_diameter, default_beam_material);
			if (result < 1 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (globeams) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			fadeDist=1000.0;
		}
		else if (mode==22)
		{
			//parse wings
			int nds[8];
			float txes[8];
			char type;
			float cratio, mind, maxd, liftcoef = 1.0f;
			char afname[256];
			int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %f, %f, %f, %f, %f, %c, %f, %f, %f, %s %f",
				&nds[0],
				&nds[1],
				&nds[2],
				&nds[3],
				&nds[4],
				&nds[5],
				&nds[6],
				&nds[7],
				&txes[0],
				&txes[1],
				&txes[2],
				&txes[3],
				&txes[4],
				&txes[5],
				&txes[6],
				&txes[7],
				&type,
				&cratio,
				&mind,
				&maxd,
				afname,
				&liftcoef
				);

			//visuals
			if (result < 13 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Wing) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_wing >= MAX_WINGS)
			{
				LogManager::getSingleton().logMessage("wings limit reached ("+StringConverter::toString(MAX_WINGS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			char wname[256];
			sprintf(wname, "wing-%s-%i",truckname, free_wing);
			char wnamei[256];
			sprintf(wnamei, "wingobj-%s-%i",truckname, free_wing);
			if (liftcoef != 1.0f) LogManager::getSingleton().logMessage("Wing liftforce coefficent: " + StringConverter::toString(liftcoef));
			wings[free_wing].fa=new FlexAirfoil(manager, wname, nodes, nds[0], nds[1], nds[2], nds[3], nds[4], nds[5], nds[6], nds[7], texname, Vector2(txes[0], txes[1]), Vector2(txes[2], txes[3]), Vector2(txes[4], txes[5]), Vector2(txes[6], txes[7]), type, cratio, mind, maxd, afname, liftcoef, aeroengines, state!=NETWORKED);
			Entity *ec=0;
			try
			{
				ec = manager->createEntity(wnamei, wname);
			}catch(...)
			{
				LogManager::getSingleton().logMessage("error loading mesh: "+String(wname));
				continue;
			}
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0.5, 1, 0));
			if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
			if(usedSkin) usedSkin->replaceMeshMaterials(ec);
			wings[free_wing].cnode = manager->getRootSceneNode()->createChildSceneNode();
			if(ec)
				wings[free_wing].cnode->attachObject(ec);
			//induced drag
			if (wingstart==-1) {wingstart=free_wing;wingarea=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);}
			else
			{
				if (nds[1]!=wings[free_wing-1].fa->nfld)
				{
					//discontinuity
					//inform wing segments
					float span=(nodes[wings[wingstart].fa->nfrd].RelPosition-nodes[wings[free_wing-1].fa->nfld].RelPosition).length();
					//					float chord=(nodes[wings[wingstart].fa->nfrd].Position-nodes[wings[wingstart].fa->nbrd].Position).length();
					LogManager::getSingleton().logMessage("BEAM: Full Wing "+StringConverter::toString(wingstart)+"-"+StringConverter::toString(free_wing-1)+" SPAN="+StringConverter::toString(span)+" AREA="+StringConverter::toString(wingarea));
					wings[wingstart].fa->enableInducedDrag(span,wingarea, false);
					wings[free_wing-1].fa->enableInducedDrag(span,wingarea, true);
					//we want also to add positional lights for first wing
					if (!hasposlights && flaresMode>0)
					{

						if(free_prop+4 >= MAX_PROPS)
						{
							LogManager::getSingleton().logMessage("cannot create wing props: props limit reached ("+StringConverter::toString(MAX_PROPS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
							continue;
						}
						//Left green
						leftlight=wings[free_wing-1].fa->nfld;
						props[free_prop].noderef=wings[free_wing-1].fa->nfld;
						props[free_prop].nodex=wings[free_wing-1].fa->nflu;
						props[free_prop].nodey=wings[free_wing-1].fa->nfld; //ignored
						props[free_prop].offsetx=0.5;
						props[free_prop].offsety=0.0;
						props[free_prop].offsetz=0.0;
						props[free_prop].rot=Quaternion::IDENTITY;
						props[free_prop].wheel=0;
						props[free_prop].wheelrotdegree=0.0;
						props[free_prop].mirror=0;
						props[free_prop].pale=0;
						props[free_prop].spinner=0;
						props[free_prop].snode=NULL; //no visible prop
						props[free_prop].bpos[0]=0.0;
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='L';
						//no light
						props[free_prop].light[0]=0;
						//the flare billboard
						char propname[256];
						sprintf(propname, "prop-%s-%i", truckname, free_prop);
						props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if(props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/greenflare");
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						}
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(0.5, 0.5);
						props[free_prop].animFlags[0]=0;
						props[free_prop].animMode[0]=0;
						free_prop++;
						//Left flash
						props[free_prop].noderef=wings[free_wing-1].fa->nbld;
						props[free_prop].nodex=wings[free_wing-1].fa->nblu;
						props[free_prop].nodey=wings[free_wing-1].fa->nbld; //ignored
						props[free_prop].offsetx=0.5;
						props[free_prop].offsety=0.0;
						props[free_prop].offsetz=0.0;
						props[free_prop].rot=Quaternion::IDENTITY;
						props[free_prop].wheel=0;
						props[free_prop].wheelrotdegree=0.0;
						props[free_prop].mirror=0;
						props[free_prop].pale=0;
						props[free_prop].spinner=0;
						props[free_prop].snode=NULL; //no visible prop
						props[free_prop].bpos[0]=0.5; //alt
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='w';
						//light
						sprintf(propname, "prop-%s-%i", truckname, free_prop);
						props[free_prop].light[0]=manager->createLight(propname);
						props[free_prop].light[0]->setType(Light::LT_POINT);
						props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 1.0, 1.0));
						props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 1.0, 1.0));
						props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
						props[free_prop].light[0]->setCastShadows(false);
						props[free_prop].light[0]->setVisible(false);
						//the flare billboard
						props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if(props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/flare");
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						}
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
						free_prop++;
						//Right red
						rightlight=wings[wingstart].fa->nfrd;
						props[free_prop].noderef=wings[wingstart].fa->nfrd;
						props[free_prop].nodex=wings[wingstart].fa->nfru;
						props[free_prop].nodey=wings[wingstart].fa->nfrd; //ignored
						props[free_prop].offsetx=0.5;
						props[free_prop].offsety=0.0;
						props[free_prop].offsetz=0.0;
						props[free_prop].rot=Quaternion::IDENTITY;
						props[free_prop].wheel=0;
						props[free_prop].wheelrotdegree=0.0;
						props[free_prop].mirror=0;
						props[free_prop].pale=0;
						props[free_prop].spinner=0;
						props[free_prop].snode=NULL; //no visible prop
						props[free_prop].bpos[0]=0.0;
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='R';
						//no light
						props[free_prop].light[0]=0;
						//the flare billboard
						sprintf(propname, "prop-%s-%i", truckname, free_prop);
						props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if(props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/redflare");
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						}
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(0.5, 0.5);
						props[free_prop].animFlags[0]=0;
						props[free_prop].animMode[0]=0;
						free_prop++;
						//Right flash
						props[free_prop].noderef=wings[wingstart].fa->nbrd;
						props[free_prop].nodex=wings[wingstart].fa->nbru;
						props[free_prop].nodey=wings[wingstart].fa->nbrd; //ignored
						props[free_prop].offsetx=0.5;
						props[free_prop].offsety=0.0;
						props[free_prop].offsetz=0.0;
						props[free_prop].rot=Quaternion::IDENTITY;
						props[free_prop].wheel=0;
						props[free_prop].wheelrotdegree=0.0;
						props[free_prop].mirror=0;
						props[free_prop].pale=0;
						props[free_prop].spinner=0;
						props[free_prop].snode=NULL; //no visible prop
						props[free_prop].bpos[0]=0.5; //alt
						props[free_prop].brate[0]=1.0;
						props[free_prop].beacontype='w';
						//light
						sprintf(propname, "prop-%s-%i", truckname, free_prop);
						props[free_prop].light[0]=manager->createLight(propname);
						props[free_prop].light[0]->setType(Light::LT_POINT);
						props[free_prop].light[0]->setDiffuseColour( ColourValue(1.0, 1.0, 1.0));
						props[free_prop].light[0]->setSpecularColour( ColourValue(1.0, 1.0, 1.0));
						props[free_prop].light[0]->setAttenuation(50.0, 1.0, 0.3, 0.0);
						props[free_prop].light[0]->setCastShadows(false);
						props[free_prop].light[0]->setVisible(false);
						//the flare billboard
						props[free_prop].bbsnode[0] = manager->getRootSceneNode()->createChildSceneNode();
						props[free_prop].bbs[0]=manager->createBillboardSet(propname,1);
						props[free_prop].bbs[0]->createBillboard(0,0,0);
						if(props[free_prop].bbs[0])
						{
							props[free_prop].bbs[0]->setMaterialName("tracks/flare");
							props[free_prop].bbsnode[0]->attachObject(props[free_prop].bbs[0]);
						}
						props[free_prop].bbsnode[0]->setVisible(false);
						props[free_prop].bbs[0]->setDefaultDimensions(1.0, 1.0);
						props[free_prop].animFlags[0]=0;
						props[free_prop].animMode[0]=0;
						free_prop++;
						hasposlights=true;
					}
					wingstart=free_wing;
					wingarea=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);
				}
				else wingarea+=warea(nodes[wings[free_wing].fa->nfld].AbsPosition, nodes[wings[free_wing].fa->nfrd].AbsPosition, nodes[wings[free_wing].fa->nbld].AbsPosition, nodes[wings[free_wing].fa->nbrd].AbsPosition);
			}

			free_wing++;
		}
		else if (mode==23 || mode==35 || mode==36) //turboprops, turboprops2, pistonprops
		{
			//parse turboprops
			int ref,back,p1,p2,p3,p4;
			int couplenode=-1;
			float pitch=-10;
			bool isturboprops=true;
			float power;
			char propfoil[256];
			if (mode==23)
			{
				int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &power, propfoil);
				if (result < 8 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Turboprop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
			}
			if (mode==35)
			{
				int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &couplenode, &power, propfoil);
				if (result < 9 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Turboprop2) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
			}
			if (mode==36)
			{
				int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %f, %f, %s", &ref, &back, &p1, &p2, &p3, &p4, &couplenode, &power, &pitch, propfoil);
				if (result < 10 || result == EOF) {
					LogManager::getSingleton().logMessage("Error parsing File (Pistonprop) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				isturboprops=false;
			}

			if(free_aeroengine >= MAX_AEROENGINES)
			{
				LogManager::getSingleton().logMessage("airoengine limit reached ("+StringConverter::toString(MAX_AEROENGINES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			char propname[256];
			sprintf(propname, "turboprop-%s-%i", truckname, free_aeroengine);
			Turboprop *tp=new Turboprop(manager, propname, nodes, ref, back,p1,p2,p3,p4, couplenode, power, propfoil, free_aeroengine, trucknum, disable_smoke, !isturboprops, pitch, heathaze);
			aeroengines[free_aeroengine]=tp;
			driveable=AIRPLANE;
			if (!autopilot && state != NETWORKED)
				autopilot=new Autopilot(hfinder, water, trucknum);
			//if (audio) audio->setupAeroengines(audiotype);
			//setup visual
			int i;
			float pscale=(nodes[ref].RelPosition-nodes[p1].RelPosition).length()/2.25;
			for (i=0; i<free_prop; i++)
			{
				if (props[i].pale && props[i].noderef==ref)
				{
					//setup size
					props[i].snode->scale(pscale,pscale,pscale);
					tp->addPale(props[i].snode);
				}
				if (props[i].spinner && props[i].noderef==ref)
				{
					props[i].snode->scale(pscale,pscale,pscale);
					tp->addSpinner(props[i].snode);
				}
			}
			free_aeroengine++;
		}
		else if (mode==24)
		{
			//parse fusedrag
			int front,back;
			float width;
			char fusefoil[256];
			int result = sscanf(line,"%i, %i, %f, %s", &front,&back,&width, fusefoil);
			if (result < 4 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Fusedrag) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			fuseAirfoil=new Airfoil(fusefoil);
			fuseFront=&nodes[front];
			fuseBack=&nodes[front];
			fuseWidth=width;
		}
		else if (mode==25)
		{
			//parse engoption
			float inertia;
			char type;
			float clutch = -1.0f, shifttime = -1.0f, clutchtime = -1.0f, postshifttime = -1.0f;
			int result = sscanf(line,"%f, %c, %f, %f, %f, %f", &inertia, &type, &clutch, &shifttime, &clutchtime, &postshifttime);
			if (result < 1 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (Engoption) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (engine) engine->setOptions(inertia, type, clutch, shifttime, clutchtime, postshifttime);
		}
		else if (mode==26)
		{
			// parse brakes
			int result = sscanf(line,"%f, %f", &brakeforce, &hbrakeforce);
			// Read in footbrake force and handbrake force. If handbrakeforce is not present, set it to the default value 2*footbrake force to preserve older functionality
			if (result == 1)
				hbrakeforce = 2.0f * brakeforce;
		}
		else if (mode==27)
		{
			//parse rotators
			int axis1, axis2,keys,keyl;
			int p1[4], p2[4];
			float rate;
			hascommands=1;
			Real startDelay=0;
			Real stopDelay=0;
			char startFunction[50]="";
			char stopFunction[50]="";
			int result = sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %f, %i, %i, %f, %f, %s %s", &axis1, &axis2, &p1[0], &p1[1], &p1[2], &p1[3], &p2[0], &p2[1], &p2[2], &p2[3], &rate, &keys, &keyl, &startDelay, &stopDelay, startFunction, stopFunction);
			if (result < 13 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Rotators) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_rotator >= MAX_ROTATORS)
			{
				LogManager::getSingleton().logMessage("rotators limit reached ("+StringConverter::toString(MAX_ROTATORS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			rotators[free_rotator].angle=0;
			rotators[free_rotator].rate=rate;
			rotators[free_rotator].axis1=axis1;
			rotators[free_rotator].axis2=axis2;
			int i;
			for (i=0; i<4; i++)
			{
				rotators[free_rotator].nodes1[i]=p1[i];
				rotators[free_rotator].nodes2[i]=p2[i];
			}
			//add short key
			commandkey[keys].rotators.push_back(-(free_rotator+1));
			commandkey[keys].description = "Rotate Left";
			//add long key
			commandkey[keyl].rotators.push_back(free_rotator+1);
			commandkey[keyl].description = "Rotate Right";

			if(rotaInertia && startDelay > 0 && stopDelay > 0)
			{
				rotaInertia->setCmdKeyDelay(keys,startDelay,stopDelay,String (startFunction),String (stopFunction));
				rotaInertia->setCmdKeyDelay(keyl,startDelay,stopDelay,String (startFunction),String (stopFunction));
			}
			else if(rotaInertia && (inertia_startDelay > 0 || inertia_stopDelay > 0))
			{
				rotaInertia->setCmdKeyDelay(keys,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
				rotaInertia->setCmdKeyDelay(keyl,inertia_startDelay,inertia_stopDelay, String (inertia_default_startFunction), String (inertia_default_stopFunction));
			}

			free_rotator++;
		}
		else if (mode==28)
		{
			//parse screwprops
			int ref,back,up;
			float power;
			int result = sscanf(line,"%i, %i, %i, %f", &ref,&back,&up, &power);
			if (result < 4 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Screwprops) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//if (audio) audio->setupBoat(truckmass);

			if(free_screwprop >= MAX_SCREWPROPS)
			{
				LogManager::getSingleton().logMessage("screwprops limit reached ("+StringConverter::toString(MAX_SCREWPROPS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			screwprops[free_screwprop]=new Screwprop(nodes, ref, back, up, power, water, trucknum);
			driveable=BOAT;
			free_screwprop++;
		}
		else if (mode==32)
		{
			// guisettings
			char keyword[255];
			char value[255];
			int result = sscanf(line,"%s %s", keyword, value);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (guisettings) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if(!strncmp(keyword, "interactiveOverviewMap", 255) && strnlen(value, 255) > 0)
			{
				dynamicMapMode = 0;
				if(!strncmp(value, "off", 255))
					dynamicMapMode = 0;
				else if(!strncmp(value, "simple", 255))
					dynamicMapMode = 1;
				else if(!strncmp(value, "zoom", 255))
					dynamicMapMode = 2;
			}
			if(!strncmp(keyword, "debugBeams", 255) && strnlen(value, 255) > 0)
			{
				debugVisuals = 0;
				if(!strncmp(value, "off", 255))
					debugVisuals = 0;
				else if(!strncmp(value, "node-numbers", 255))
					debugVisuals = 1;
				else if(!strncmp(value, "beam-numbers", 255))
					debugVisuals = 2;
				else if(!strncmp(value, "node-and-beam-numbers", 255))
					debugVisuals = 3;
				else if(!strncmp(value, "node-mass", 255))
					debugVisuals = 4;
				else if(!strncmp(value, "node-locked", 255))
					debugVisuals = 5;
				else if(!strncmp(value, "beam-compression", 255))
					debugVisuals = 6;
				else if(!strncmp(value, "beam-broken", 255))
					debugVisuals = 7;
				else if(!strncmp(value, "beam-stress", 255))
					debugVisuals = 8;
				else if(!strncmp(value, "beam-strength", 255))
					debugVisuals = 9;
				else if(!strncmp(value, "beam-hydros", 255))
					debugVisuals = 10;
				else if(!strncmp(value, "beam-commands", 255))
					debugVisuals = 11;
			}
			if(!strncmp(keyword, "tachoMaterial", 255) && strnlen(value, 255) > 0)
			{
				tachomat = String(value);
			}
			else if(!strncmp(keyword, "speedoMaterial", 255) && strnlen(value, 255) > 0)
			{
				speedomat = String(value);
			}
			else if(!strncmp(keyword, "helpMaterial", 255) && strnlen(value, 255) > 0)
			{
				strncpy(helpmat, value, 254);
			}
			else if(!strncmp(keyword, "speedoMax", 255) && strnlen(value, 255) > 0)
			{
				float tmp = StringConverter::parseReal(String(value));
				if(tmp > 10 && tmp < 32000)
					speedoMax = tmp;
			}
			else if(!strncmp(keyword, "useMaxRPM", 255) && strnlen(value, 255) > 0)
			{
				int use = StringConverter::parseInt(String(value));
				useMaxRPMforGUI = (use == 1);
			}

		}
		else if (mode==33)
		{
			//parse minimass
			//sets the minimum node mass
			//usefull for very light vehicles with lots of nodes (e.g. small airplanes)
			sscanf(line,"%f", &minimass);
		}
		else if (mode==34)
		{
			// parse exhausts
			if (disable_smoke)
				continue;
			int id1, id2;
			float factor;
			char material[50] = "";
			int result = sscanf(line,"%i, %i, %f %s", &id1, &id2, &factor, material);
			// catch some errors
			if (result < 4 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			exhaust_t e;
			e.emitterNode = id1;
			e.directionNode = id2;
			e.isOldFormat = false;
			e.smokeNode = parent->createChildSceneNode();
			char wname[256];
			sprintf(wname, "exhaust-%d-%s", (int)exhausts.size(), truckname);
			if(strnlen(material,50) == 0 || String(material) == "default")
				strcpy(material, "tracks/Smoke");

			if(usedSkin) strncpy(material, usedSkin->getReplacementForMaterial(material).c_str(), 50);

			e.smoker = manager->createParticleSystem(wname, material);
			if (!e.smoker)
				continue;
			e.smokeNode->attachObject(e.smoker);
			e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
			nodes[id1].isHot=true;
			nodes[id2].isHot=true;
			nodes[id1].iIsSkin=true;
			nodes[id2].iIsSkin=true;
			exhausts.push_back(e);
		}
		else if (mode==37)
		{
			// command lists
			//truckScript->addCommand(line, currentScriptCommandNumber);
		}
		else if (mode==38)
		{
			//particles
			if(!cparticle_enabled)
				continue;
			// parse particle
			int id1, id2;
			char psystem[250] = "";
			int result = sscanf(line,"%i, %i, %s", &id1, &id2, psystem);
			// catch some errors
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_cparticle >= MAX_CPARTICLES)
			{
				LogManager::getSingleton().logMessage("custom particles limit reached ("+StringConverter::toString(MAX_CPARTICLES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			cparticles[free_cparticle].emitterNode = id1;
			cparticles[free_cparticle].directionNode = id2;
			cparticles[free_cparticle].snode = parent->createChildSceneNode();
			char wname[256];
			sprintf(wname, "cparticle-%i-%s", free_cparticle, truckname);
			cparticles[free_cparticle].psys = manager->createParticleSystem(wname, psystem);
			if (!cparticles[free_cparticle].psys)
				continue;
			cparticles[free_cparticle].snode->attachObject(cparticles[free_cparticle].psys);
			cparticles[free_cparticle].snode->setPosition(nodes[cparticles[free_cparticle].emitterNode].AbsPosition);
			//shut down the emitters
			cparticles[free_cparticle].active=false;
			for (int i=0; i<cparticles[free_cparticle].psys->getNumEmitters(); i++) cparticles[free_cparticle].psys->getEmitter(i)->setEnabled(false);
			free_cparticle++;
		}
		else if (mode==39) //turbojets
		{
			//parse turbojets
			int front,back,ref, rev;
			float len, fdiam, bdiam, drthrust, abthrust;
			int result = sscanf(line,"%i, %i, %i, %i, %f, %f, %f, %f, %f", &front, &back, &ref, &rev, &drthrust, &abthrust, &fdiam, &bdiam, &len);
			if (result < 9 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Turbojet) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_aeroengine >= MAX_AEROENGINES)
			{
				LogManager::getSingleton().logMessage("airoengine limit reached ("+StringConverter::toString(MAX_AEROENGINES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			char propname[256];
			sprintf(propname, "turbojet-%s-%i", truckname, free_aeroengine);
			Turbojet *tj=new Turbojet(manager, propname, free_aeroengine, trucknum, nodes, front, back, ref, drthrust, rev!=0, abthrust>0, abthrust, fdiam, bdiam, len, disable_smoke, heathaze, materialFunctionMapper, usedSkin);
			aeroengines[free_aeroengine]=tj;
			driveable=AIRPLANE;
			if (!autopilot && state != NETWORKED)
				autopilot=new Autopilot(hfinder, water, trucknum);
			//if (audio) audio->setupAeroengines(TURBOJETS);
			free_aeroengine++;
		}
		else if (mode==40)
		{
			//parse rigidifiers
			int na,nb,nc;
			float k=DEFAULT_RIGIDIFIER_SPRING;
			float d=DEFAULT_RIGIDIFIER_DAMP;
			int result = sscanf(line,"%i, %i, %i, %f, %f", &na, &nb, &nc, &k, &d);
			if (result < 3 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (rigidifier) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_rigidifier >= MAX_RIGIDIFIERS)
			{
				LogManager::getSingleton().logMessage("rigidifiers limit reached ("+StringConverter::toString(MAX_RIGIDIFIERS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			rigidifiers[free_rigidifier].a=&nodes[na];
			rigidifiers[free_rigidifier].b=&nodes[nb];
			rigidifiers[free_rigidifier].c=&nodes[nc];
			rigidifiers[free_rigidifier].k=k;
			rigidifiers[free_rigidifier].d=d;
			rigidifiers[free_rigidifier].alpha=2.0*acos((nodes[na].RelPosition-nodes[nb].RelPosition).getRotationTo(nodes[nc].RelPosition-nodes[nb].RelPosition).w);
			rigidifiers[free_rigidifier].lastalpha=rigidifiers[free_rigidifier].alpha;
			rigidifiers[free_rigidifier].beama=0;
			rigidifiers[free_rigidifier].beamc=0;
			//searching for associated beams
			for (int i=0; i<free_beam; i++)
			{
				if ((beams[i].p1==&nodes[na] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[na] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beama=&beams[i];
				if ((beams[i].p1==&nodes[nc] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[nc] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beamc=&beams[i];
			}
			free_rigidifier++;
		}
		else if (mode==41)
		{
			//parse airbrakes
			int ref, nx, ny, na;
			float ox, oy, oz;
			float tx1, tx2, tx3, tx4;
			float wd, len, liftcoef = 1.0f;
			float maxang;
			int result = sscanf(line,"%i, %i, %i, %i, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", &ref, &nx, &ny, &na, &ox, &oy, &oz, &wd, &len, &maxang, &tx1, &tx2, &tx3, &tx4, &liftcoef);
			if (result < 14 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Airbrakes) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}

			if(free_airbrake >= MAX_AIRBRAKES)
			{
				LogManager::getSingleton().logMessage("airbrakes limit reached ("+StringConverter::toString(MAX_AIRBRAKES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			if (liftcoef != 1.0f)
				LogManager::getSingleton().logMessage("Airbrakes force coefficent: " + StringConverter::toString(liftcoef));

			airbrakes[free_airbrake]=new Airbrake(manager, truckname, free_airbrake, &nodes[ref], &nodes[nx], &nodes[ny], &nodes[na], Vector3(ox,oy,oz), wd, len, maxang, texname, tx1,tx2,tx3,tx4,liftcoef);
			free_airbrake++;
		}
		else if (mode==43)
		{
			//parse flexbodies
			int ref, nx, ny;
			float ox, oy, oz;
			float rx, ry, rz;
			char meshname[256];
			int result = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s", &ref, &nx, &ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname);
			if (result < 10 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (Flexbodies) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			Vector3 offset=Vector3(ox, oy, oz);
			Quaternion rot=Quaternion(Degree(rz), Vector3::UNIT_Z);
			rot=rot*Quaternion(Degree(ry), Vector3::UNIT_Y);
			rot=rot*Quaternion(Degree(rx), Vector3::UNIT_X);
			char uname[256];
			sprintf(uname, "flexbody-%s-%i", truckname, free_flexbody);
			//read an extra line!
			ds->readLine(line, 1023);
			linecounter++;
			if (strncmp(line, "forset", 6))
			{
				LogManager::getSingleton().logMessage("Error parsing File (Flexbodies/forset) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". No forset statement after a flexbody. trying to continue ...");
				continue;
			}

			if(free_flexbody >= MAX_FLEXBODIES)
			{
				LogManager::getSingleton().logMessage("flexbodies limit reached ("+StringConverter::toString(MAX_FLEXBODIES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			flexbodies[free_flexbody]=new FlexBody(manager, nodes, free_node, meshname, uname, ref, nx, ny, offset, rot, line+6, materialFunctionMapper, usedSkin);
			free_flexbody++;
		}
		else if (mode==44)
		{
			//parse hookgroups
			int id1=0, group=-1, lockNodes=1;
			int result = sscanf(line,"%i, %i, %i", &id1, &group, &lockNodes);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (hookgroup) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Too less arguments. trying to continue ...");
				continue;
			}
			if (id1>=free_node)
			{
				LogManager::getSingleton().logMessage("Error: unknown node number in hookgroup section ("+StringConverter::toString(id1)+")");
				exit(7);
			};
			hook_t h;
			h.hookNode=&nodes[id1];
			h.group=group;
			h.locked=UNLOCKED;
			h.lockNode=0;
			h.lockTruck=0;
			h.lockNodes=(bool)(lockNodes != 0);
			hooks.push_back(h);

		}
		else if (mode==46)
		{
			// parse materialflarebindings
			int flareid;
			char material[255]="";
			memset(material, 0, 255);
			int result = sscanf(line,"%d, %s", &flareid, material);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (materialbindings) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			String materialName = String(material);
			String newMaterialName = materialName + "_mfb_" + String(truckname);
			MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
			if(mat.isNull())
			{
				LogManager::getSingleton().logMessage("Error in materialbindings: material " + materialName + " was not found! " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//clone the material
			MaterialPtr newmat = mat->clone(newMaterialName);
			//create structes and add
			materialmapping_t t;
			t.originalmaterial = materialName;
			t.material = newMaterialName;
			t.type=0;
			if(materialFunctionMapper)
				materialFunctionMapper->addMaterial(flareid, t);
		}
		else if (mode==47)
		{
			//parse soundsources
			int ref;
			char script[256];
			int result = sscanf(line,"%i, %s", &ref, script);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (soundsource) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
#ifdef USE_OPENAL
			if(ssm) addSoundSource(ssm->createInstance(script, trucknum, NULL), ref);
#endif //OPENAL
		}
		else if (mode==48)
		{
			// parse envmap
			// we do nothing of this for the moment
		}
		else if (mode==49)
		{
			// parse managedmaterials
			char material[255];
			material[0]=0;
			char type[255];
			type[0]=0;
			int result = sscanf(line,"%s %s", material, type);
			if (result < 2 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			//first, check if work has already been done
			MaterialPtr tstmat=(MaterialPtr)(MaterialManager::getSingleton().getByName(material));
			if (!tstmat.isNull())
			{
				//material already exists, probably because the vehicle was already spawned previously
				LogManager::getSingleton().logMessage("Warning: managed material '" + String(material) +"' already exists");
				continue;
			}

			if (!strcmp(type, "flexmesh_standard"))
			{
				char maintex[255];
				maintex[0]=0;
				char dmgtex[255];
				dmgtex[0]=0;
				char spectex[255];
				spectex[0]=0;
				result = sscanf(line,"%s %s %s %s %s", material, type, maintex, dmgtex, spectex);
				if (result < 3 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//different cases
				//caution, this is hardwired against the managed.material file
				if (strlen(dmgtex)==0 || dmgtex[0]=='-')
				{
					//no damage texture
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/simple"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_standard/simple' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						if(managedmaterials_doublesided)
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular, but no damage
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/specularonly"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_standard/specularonly' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						if(managedmaterials_doublesided)
						{
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						}
						dstmat->compile();
					}
				}
				else
				{
					//damage texture
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/damageonly"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_standard/damageonly' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(dmgtex);
						if(managedmaterials_doublesided)
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular, and damage
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_standard/speculardamage"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_standard/speculardamage' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName(dmgtex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						if(managedmaterials_doublesided)
						{
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						}
						dstmat->compile();
					}
				}
			}
			else if (!strcmp(type, "flexmesh_transparent"))
			{
				char maintex[255];
				maintex[0]=0;
				char dmgtex[255];
				dmgtex[0]=0;
				char spectex[255];
				spectex[0]=0;
				result = sscanf(line,"%s %s %s %s %s", material, type, maintex, dmgtex, spectex);
				if (result < 3 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//different cases
				//caution, this is hardwired against the managed.material file
				if (strlen(dmgtex)==0 || dmgtex[0]=='-')
				{
					//no damage texture
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/simple"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_transparent/simple' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						if(managedmaterials_doublesided)
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular, but no damage
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/specularonly"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_transparent/specularonly' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						if(managedmaterials_doublesided)
						{
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						}
						dstmat->compile();
					}
				}
				else
				{
					//damage texture
					if (strlen(spectex)==0 || spectex[0]=='-')
					{
						//no specular texture
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/damageonly"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_transparent/damageonly' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(dmgtex);
						if(managedmaterials_doublesided)
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->compile();
					}
					else
					{
						//specular, and damage
						MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/flexmesh_transparent/speculardamage"));
						if(srcmat.isNull())
						{
							LogManager::getSingleton().logMessage("Material 'managed/flexmesh_transparent/speculardamage' missing!");
							continue;
						}

						MaterialPtr dstmat=srcmat->clone(material);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
						dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName(dmgtex);
						dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
						if(managedmaterials_doublesided)
						{
							dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
							dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
						}
						dstmat->compile();
					}
				}
			}
			else if (!strcmp(type, "mesh_standard"))
			{
				char maintex[255];
				maintex[0]=0;
				char spectex[255];
				spectex[0]=0;
				result = sscanf(line,"%s %s %s %s", material, type, maintex, spectex);
				if (result < 3 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//different cases
				//caution, this is hardwired against the managed.material file
				if (strlen(spectex)==0 || spectex[0]=='-')
				{
					//no specular texture
					MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_standard/simple"));
					if(srcmat.isNull())
					{
						LogManager::getSingleton().logMessage("Material 'managed/mesh_standard/simple' missing!");
						continue;
					}

					MaterialPtr dstmat=srcmat->clone(material);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
					if(managedmaterials_doublesided)
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
					dstmat->compile();
				}
				else
				{
					//specular
					MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_standard/specular"));
					if(srcmat.isNull())
					{
						LogManager::getSingleton().logMessage("Material 'managed/mesh_standard/specular' missing!");
						continue;
					}

					MaterialPtr dstmat=srcmat->clone(material);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
					dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
					if(managedmaterials_doublesided)
					{
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
					}
					dstmat->compile();
				}
			}
			else if (!strcmp(type, "mesh_transparent"))
			{
				char maintex[255];
				maintex[0]=0;
				char spectex[255];
				spectex[0]=0;
				result = sscanf(line,"%s %s %s %s", material, type, maintex, spectex);
				if (result < 3 || result == EOF)
				{
					LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
					continue;
				}
				//different cases
				//caution, this is hardwired against the managed.material file
				if (strlen(spectex)==0 || spectex[0]=='-')
				{
					//no specular texture
					MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_transparent/simple"));
					if(srcmat.isNull())
					{
						LogManager::getSingleton().logMessage("Material 'managed/mesh_transparent/simple' missing!");
						continue;
					}

					MaterialPtr dstmat=srcmat->clone(material);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
					if(managedmaterials_doublesided)
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
					dstmat->compile();
				}
				else
				{
					//specular
					MaterialPtr srcmat=(MaterialPtr)(MaterialManager::getSingleton().getByName("managed/mesh_transparent/specular"));
					if(srcmat.isNull())
					{
						LogManager::getSingleton().logMessage("Material 'managed/mesh_transparent/specular' missing!");
						continue;
					}

					MaterialPtr dstmat=srcmat->clone(material);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(maintex);
					dstmat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName(spectex);
					dstmat->getTechnique(0)->getPass(1)->getTextureUnitState(0)->setTextureName(spectex);
					if(managedmaterials_doublesided)
					{
						dstmat->getTechnique(0)->getPass(0)->setCullingMode(Ogre::CULL_NONE);
						dstmat->getTechnique(0)->getPass(1)->setCullingMode(Ogre::CULL_NONE);
					}
					dstmat->compile();
				}
			}
			else
			{
				LogManager::getSingleton().logMessage("Error parsing File (managedmaterials) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Unknown effect. trying to continue ...");
				continue;
			}

		}
		else if (mode==50)
		{
			// parse sectionconfig
			// not used here ...
			mode=savedmode;
		}
		else if (mode==51)
		{
			// parse section
			int version=0;
			char sectionName[10][256];
			for(int i=0;i<10;i++) memset(sectionName, 0, 255); // clear
			if(strnlen(line,9)<8) continue;
			int result = sscanf(line+7,"%d %s %s %s %s %s %s %s %s %s %s", &version, sectionName[0], sectionName[1], sectionName[2], sectionName[3], sectionName[4], sectionName[5], sectionName[6], sectionName[7], sectionName[8], sectionName[9]);
			if (result < 2 || result == EOF) {
				LogManager::getSingleton().logMessage("Error parsing File (section) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			bool found = false;
			for(int i=0;i<10;i++)
			{
				for(std::vector<String>::iterator it=truckconfig.begin(); it!=truckconfig.end(); it++)
				{
					if(sectionName[i] == *it)
					{
						found = true;
						break;
					}
				}
			}
			if(found)
				continue;
			else
				// wait for end_section otherwise
				mode=52;
		}
		/* mode 52 is reserved */
		else if (mode==53)
		{
			// parse torquecurve
			if (engine && engine->getTorqueCurve())
				engine->getTorqueCurve()->processLine(String(line));
		} else if (mode==54)
		{
			//parse advanced drag
			float drag;
			int result = sscanf(line,"%f", &drag);
			if (result < 4 || result == EOF)
			{
				LogManager::getSingleton().logMessage("Error parsing File (advdrag) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
				continue;
			}
			advanced_total_drag=drag;
			advanced_drag=true;
		}
		else if (mode==55)
		{
			// parse axle section
			// search for wheel

			if(!free_wheel)
			{
				LogManager::getSingleton().logMessage("AXLE ERROR: the axle section must come AFTER some wheels");
				continue;
			}

			int wheel_node[2][2] = {{0, 0}, {0, 0}};
			Ogre::vector<Ogre::String>::type options = Ogre::StringUtil::split(line, ",");
			Ogre::vector<Ogre::String>::type::iterator cur = options.begin();

			for(; cur != options.end(); ++cur)
			{
				Ogre::StringUtil::trim(*cur);

				LogManager::getSingleton().logMessage("AXLE: Parsing property: [" + *cur + "]" );

				switch(cur->at(0))
				{
				// wheels
				case 'w':
					// dirty repetitive method, could stand to be cleaned up
					if( cur->at(1) == '1')
					{
						int result = sscanf(cur->c_str(), "w1(%d %d)", &wheel_node[0][0], &wheel_node[0][1]);

						if (result < 2 )
						{
							LogManager::getSingleton().logMessage("AXLE: line did not contain enough points: " + *cur);
							continue;
						}
					}
					else if( cur->at(1) == '2')
					{
						int result = sscanf(cur->c_str(), "w2(%d %d)", &wheel_node[1][0], &wheel_node[1][1]);

						if (result < 2 )
						{
							LogManager::getSingleton().logMessage("AXLE: line did not contain enough points: " + *cur);
							continue;
						}
					}
					break;
				case 'd':
				{
						char diffs[10] = {0};
						int results = sscanf(cur->c_str(), "d(%9s)", diffs);
						if(results == 0 ) break;
						for(int i = 0; i < 10; ++i)
						{
							switch(diffs[i])
							{
							case 'l': axles[free_axle].addDiffType(LOCKED_DIFF); break;
							case 'o': axles[free_axle].addDiffType(OPEN_DIFF); break;
							//case 'v': axles[free_axle].addDiffType(VISCOUS_DIFF); break;
							case 's': axles[free_axle].addDiffType(SPLIT_DIFF); break;
							//case 'm': axles[free_axle].addDiffType(LIMITED_SLIP_DIFF); break;
							}
						}
					break;
				}
				case 's':
					LogManager::getSingleton().logMessage("AXLE: selection property not yet available");
					break;
				case 'r':
					LogManager::getSingleton().logMessage("AXLE: Gear ratio property not yet available");
					break;
				default:
					LogManager::getSingleton().logMessage("AXLE: malformed property: " + *cur);
					break;
				}

			}

			//
			for( int i = 0; i < free_wheel &&  (axles[free_axle].wheel_1 < 0 || axles[free_axle].wheel_2 < 0); ++i)
			{
				if( ( wheels[i].refnode0->id == wheel_node[0][0] || wheels[i].refnode0->id == wheel_node[0][1]) &&
					( wheels[i].refnode1->id == wheel_node[0][0] || wheels[i].refnode1->id == wheel_node[0][1]))
				{
					axles[free_axle].wheel_1 = i;
				}
				if( ( wheels[i].refnode0->id == wheel_node[1][0] || wheels[i].refnode0->id == wheel_node[1][1]) &&
					( wheels[i].refnode1->id == wheel_node[1][0] || wheels[i].refnode1->id == wheel_node[1][1]))
				{
				axles[free_axle].wheel_2 = i;
				}
			}


			if( axles[free_axle].wheel_1 < 0 || axles[free_axle].wheel_2 < 0 )
			{
				// if one or the other is null
				if( axles[free_axle].wheel_1 < 0)
				{
					LogManager::getSingleton().logMessage("AXLE: could not find wheel 1 nodes: " +
						StringConverter::toString(wheel_node[0][0]) + " " +
						StringConverter::toString(wheel_node[0][1]) );
				}
				if( axles[free_axle].wheel_2 < 0)
				{
					LogManager::getSingleton().logMessage("AXLE: could not find wheel 2 nodes: " +
					StringConverter::toString(wheel_node[1][0]) + " " +
					StringConverter::toString(wheel_node[1][1]) );
				}
				continue;
			}

			// test if any differentials have been defined
			if( axles[free_axle].availableDiffs().empty() )
			{
				LogManager::getSingleton().logMessage("AXLE: nodiffs defined, defaulting to Open and Locked");
				axles[free_axle].addDiffType(OPEN_DIFF);
				axles[free_axle].addDiffType(LOCKED_DIFF);
			}

			LogManager::getSingleton().logMessage("AXLE: Created: w1(" + StringConverter::toString(wheel_node[0][0]) + ") " +
				StringConverter::toString(wheel_node[0][1]) + ", w2(" + StringConverter::toString(wheel_node[1][0]) + " " +
				StringConverter::toString(wheel_node[1][1]) + ")");
			++free_axle;
		}

		else if (mode==63) parseRailGroupLine(line);
		else if (mode==64) parseSlideNodeLine(line);

	};
	if(!loading_finished) {
		LogManager::getSingleton().logMessage("Reached end of file "+ String(fname)+ ". No 'end' was found! Did you forgot it? Trying to continue...");
	}

	// ds closes automatically, so do not close it explicitly here:
	//ds->close();
//cameras workaround
	{
		for (int i=0; i<freecamera; i++)
		{
//LogManager::getSingleton().logMessage("Camera dir="+StringConverter::toString(nodes[cameranodedir[i]].RelPosition-nodes[cameranodepos[i]].RelPosition)+" roll="+StringConverter::toString(nodes[cameranoderoll[i]].RelPosition-nodes[cameranodepos[i]].RelPosition));
			revroll[i]=(nodes[cameranodedir[i]].RelPosition-nodes[cameranodepos[i]].RelPosition).crossProduct(nodes[cameranoderoll[i]].RelPosition-nodes[cameranodepos[i]].RelPosition).y>0;
			if (revroll[i]) LogManager::getSingleton().logMessage("Warning: camera definition is probably invalid and has been corrected. It should be center, back, left");
		}
	}
//		fclose(fd);
	//wing closure
	if (wingstart!=-1)
	{
		if (autopilot) autopilot->setInertialReferences(&nodes[leftlight], &nodes[rightlight], fuseBack);
		//inform wing segments
		float span=(nodes[wings[wingstart].fa->nfrd].RelPosition-nodes[wings[free_wing-1].fa->nfld].RelPosition).length();
		//		float chord=(nodes[wings[wingstart].fa->nfrd].Position-nodes[wings[wingstart].fa->nbrd].Position).length();
		LogManager::getSingleton().logMessage("BEAM: Full Wing "+StringConverter::toString(wingstart)+"-"+StringConverter::toString(free_wing-1)+" SPAN="+StringConverter::toString(span)+" AREA="+StringConverter::toString(wingarea));
		wings[wingstart].fa->enableInducedDrag(span,wingarea, false);
		wings[free_wing-1].fa->enableInducedDrag(span,wingarea, true);
		//wash calculator
		wash_calculator(rot);
	}
	//add the cab visual
	LogManager::getSingleton().logMessage("BEAM: creating cab");
	if (free_texcoord>0 && free_cab>0)
	{
		//closure
		subtexcoords[free_sub]=free_texcoord;
		subcabs[free_sub]=free_cab;
		char wname[256];
		sprintf(wname, "cab-%s",truckname);
		char wnamei[256];
		sprintf(wnamei, "cabobj-%s",truckname);
		//the cab materials are as follow:
		//texname: base texture with emissive(2 pass) or without emissive if none available(1 pass), alpha cutting
		//texname-trans: transparency texture (1 pass)
		//texname-back: backface texture: black+alpha cutting (1 pass)
		//texname-noem: base texture without emissive (1 pass), alpha cutting

		//material passes must be:
		//0: normal texture
		//1: transparent (windows)
		//2: emissive
		/*strcpy(texname, "testtex");
		char transmatname[256];
		sprintf(transmatname, "%s-trans", texname);
		char backmatname[256];
		sprintf(backmatname, "%s-back", texname);
		hasEmissivePass=1;*/

		MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().getByName(texname));
		if(mat.isNull())
		{
			LogManager::getSingleton().logMessage("Material '"+String(texname)+"' missing!");
			exit(123);
		}

		//-trans
		char transmatname[256];
		sprintf(transmatname, "%s-trans", texname);
		MaterialPtr transmat=mat->clone(transmatname);
		if (mat->getTechnique(0)->getNumPasses()>1) transmat->getTechnique(0)->removePass(1);
		transmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(CMPF_LESS_EQUAL, 128);
		transmat->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
		if(transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
			transmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(TFO_NONE);
		transmat->compile();

		//-back
		char backmatname[256];
		sprintf(backmatname, "%s-back", texname);
		MaterialPtr backmat=mat->clone(backmatname);
		if (mat->getTechnique(0)->getNumPasses()>1) backmat->getTechnique(0)->removePass(1);
		if(transmat->getTechnique(0)->getPass(0)->getNumTextureUnitStates()>0)
			backmat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setColourOperationEx(LBX_SOURCE1, LBS_MANUAL, LBS_MANUAL, ColourValue(0,0,0),ColourValue(0,0,0));
		if(shadowOptimizations)
			backmat->setReceiveShadows(false);
		//just in case
		//backmat->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
		//backmat->getTechnique(0)->getPass(0)->setAlphaRejectSettings(CMPF_GREATER, 128);
		backmat->compile();

		//-noem and -noem-trans
		if (mat->getTechnique(0)->getNumPasses()>1)
		{
			hasEmissivePass=1;
			char clomatname[256];
			sprintf(clomatname, "%s-noem", texname);
			MaterialPtr clomat=mat->clone(clomatname);
			clomat->getTechnique(0)->removePass(1);
			clomat->compile();
		}

		//base texture is not modified
		//	mat->compile();


		LogManager::getSingleton().logMessage("BEAM: creating mesh");
		cabMesh=new FlexObj(manager, nodes, free_texcoord, texcoords, free_cab, cabs, free_sub, subtexcoords, subcabs, texname, wname, subisback, backmatname, transmatname);
		LogManager::getSingleton().logMessage("BEAM: creating entity");

		LogManager::getSingleton().logMessage("BEAM: creating cabnode");
		cabNode = manager->getRootSceneNode()->createChildSceneNode();
		Entity *ec = 0;
		try
		{
			LogManager::getSingleton().logMessage("BEAM: loading cab");
			ec = manager->createEntity(wnamei, wname);
			//		ec->setRenderQueueGroup(RENDER_QUEUE_6);
			LogManager::getSingleton().logMessage("BEAM: attaching cab");
			if(ec)
				cabNode->attachObject(ec);
		}catch(...)
		{
			LogManager::getSingleton().logMessage("error loading mesh: "+String(wname));
		}
		MaterialFunctionMapper::replaceSimpleMeshMaterials(ec, ColourValue(0.5, 1, 0.5));
		if(materialFunctionMapper) materialFunctionMapper->replaceMeshMaterials(ec);
		if(usedSkin) usedSkin->replaceMeshMaterials(ec);
	};
	LogManager::getSingleton().logMessage("BEAM: cab ok");
	//	mWindow->setDebugText("Beam number:"+ StringConverter::toString(free_beam));

	//place correctly
	if (!hasfixes)
	{
		//check if oversized
		calcBox();
		//px=px-(maxx-minx)/2.0;
		px-=(maxx+minx)/2.0-px;
		//pz=pz-(maxz-minz)/2.0;
		pz-=(maxz+minz)/2.0-pz;
		float miny=-9999.0;
		if (spawnbox) miny=spawnbox->relo_y+spawnbox->center.y+0.01;
		if(freePositioned)
			resetPosition(Vector3(px, py, pz), true);
		else
			resetPosition(px, pz, true, miny);

		if (spawnbox)
		{
			bool inside=true;
			for (int i=0; i<free_node; i++) inside=inside && collisions->isInside(nodes[i].AbsPosition, spawnbox, 0.2f);
			if (!inside)
			{
				Vector3 gpos=Vector3(px, 0, pz);
				gpos-=rot*Vector3((spawnbox->hi_x-spawnbox->lo_x+maxx-minx)*0.6, 0, 0);
				resetPosition(gpos.x, gpos.z, true, miny);
			}
		}
	}
	//compute final mass
	calc_masses2(truckmass);
	//setup default sounds
	if (!disable_default_sounds) setupDefaultSoundSources();
	//compute collision box
	calcBox();

	//compute node connectivity graph
	calcNodeConnectivityGraph();

	//update contacter nodes
	updateContacterNodes();

	// print some truck memory stats
	int mem = 0, memr = 0, tmpmem = 0;
	LogManager::getSingleton().logMessage("BEAM: memory stats following");

	tmpmem = free_beam * sizeof(beam_t); mem += tmpmem;
	memr += MAX_BEAMS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: beam memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_beam) + " x " + StringConverter::toString(sizeof(beam_t)) + " B) / " + StringConverter::toString(MAX_BEAMS * sizeof(beam_t)));

	tmpmem = free_node * sizeof(node_t); mem += tmpmem;
	memr += MAX_NODES * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: node memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_node) + " x " + StringConverter::toString(sizeof(node_t)) + " B) / " + StringConverter::toString(MAX_NODES * sizeof(node_t)));

	tmpmem = free_shock * sizeof(shock_t); mem += tmpmem;
	memr += MAX_SHOCKS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: shock memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_shock) + " x " + StringConverter::toString(sizeof(shock_t)) + " B) / " + StringConverter::toString(MAX_SHOCKS * sizeof(shock_t)));

	tmpmem = free_prop * sizeof(prop_t); mem += tmpmem;
	memr += MAX_PROPS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: prop memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_prop) + " x " + StringConverter::toString(sizeof(prop_t)) + " B) / " + StringConverter::toString(MAX_PROPS * sizeof(prop_t)));

	tmpmem = free_wheel * sizeof(wheel_t); mem += tmpmem;
	memr += MAX_WHEELS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: wheel memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_wheel) + " x " + StringConverter::toString(sizeof(wheel_t)) + " B) / " + StringConverter::toString(MAX_WHEELS * sizeof(wheel_t)));

	tmpmem = free_rigidifier * sizeof(rigidifier_t); mem += tmpmem;
	memr += MAX_RIGIDIFIERS * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: rigidifier memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_rigidifier) + " x " + StringConverter::toString(sizeof(rigidifier_t)) + " B) / " + StringConverter::toString(MAX_RIGIDIFIERS * sizeof(rigidifier_t)));

	tmpmem = free_flare * sizeof(flare_t); mem += tmpmem;
	memr += free_flare * sizeof(beam_t);
	LogManager::getSingleton().logMessage("BEAM: flare memory: " + StringConverter::toString(tmpmem) + " B (" + StringConverter::toString(free_flare) + " x " + StringConverter::toString(sizeof(flare_t)) + " B)");

	LogManager::getSingleton().logMessage("BEAM: truck memory used: " + StringConverter::toString(mem)  + " B (" + StringConverter::toString(mem/1024)  + " kB)");
	LogManager::getSingleton().logMessage("BEAM: truck memory allocated: " + StringConverter::toString(memr)  + " B (" + StringConverter::toString(memr/1024)  + " kB)");
	return 0;
}
