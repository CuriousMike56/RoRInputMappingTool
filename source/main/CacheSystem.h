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
// created on 21th of May 2008 by Thomas

#ifndef __CacheSystem_H__
#define __CacheSystem_H__

#include <Ogre.h>
#include <vector>
#include <map>
#include "sha1.h"
#include "RoRPrerequisites.h"
#include "rormemory.h"

#define CACHE_FILE "mods.cache"
#define CACHE_FILE_FORMAT "6"

// 60*60*24 = one day
#define CACHE_FILE_FRESHNESS 86400

#define CACHE CacheSystem::Instance()

using namespace Ogre;

typedef struct
{
	Ogre::String title;					// Category title
	int number;							// Category number
} Category_Entry;

class Cache_Entry
{
public:
	Ogre::String minitype;				// type of preview picture, either png or dds
	Ogre::String fname;					// filename
	Ogre::String fname_without_uid;		// filename
	Ogre::String dname;					// name parsed from the file
	
	int categoryid;						// category id
	Ogre::String categoryname;			// category name
	
	int addtimestamp;					// timestamp when this file was added to the cache
	
	Ogre::String uniqueid;				// file's unique id
	int version;						// file's version
	
	Ogre::String fext;					// file's extension
	Ogre::String type;					// Resource Type, FileSystem or Zip
	Ogre::String dirname;				// mostly, archive name
	Ogre::String hash;					// file's hash
	bool resourceLoaded;				// loaded?
	int number;							// mod number
	Ogre::String filetime;				// NOT OS INDEPENDENT filetime
	bool changedornew;					// is it added or changed during this runtime?
	bool deleted;						// is this mod deleted?
	int usagecounter;					// how much it was used already
	std::vector<authorinfo *> authors;	// authors
	Ogre::String filecachename;			// preview image filename

	// following all TRUCK detail information:
	Ogre::String description;
	Ogre::String tags;
	int fileformatversion;
	bool hasSubmeshs;
	int nodecount;
	int beamcount;
	int shockcount;
	int fixescount;
	int hydroscount;
	int wheelcount;
	int propwheelcount;
	int commandscount;
	int flarescount;
	int propscount;
	int wingscount;
	int turbopropscount;
	int turbojetcount;
	int rotatorscount;
	int exhaustscount;
	int flexbodiescount;
	int materialflarebindingscount;
	int soundsourcescount;
	int managedmaterialscount;

	float truckmass;
	float loadmass;
	float minrpm;
	float maxrpm;
	float torque;
	bool customtach;
	bool custom_particles;
	bool forwardcommands;
	bool importcommands;
	bool rollon;
	bool rescuer;

	int driveable;
	int numgears;
	char enginetype;
	std::vector<Ogre::String> sectionconfigs;
	std::set<Ogre::String> materials;

	// default constructor resets the data.
	Cache_Entry() : 
		minitype(""), 
		fname(""), 
		fname_without_uid(""), 
		dname(""), 
		categoryid(0), 
		categoryname(""), 
		addtimestamp(0), 
		uniqueid(""), 
		version(0), 
		fext(""),  
		type(""), 
		dirname(""), 
		hash(""), 
		resourceLoaded(false), 
		number(0),
		filetime(""), 
		changedornew(false), 
		deleted(false), 
		usagecounter(0),
		//authors
		filecachename(""), 
		description(""), 
		tags(""), 
		fileformatversion(0), 
		hasSubmeshs(false), 
		nodecount(0), 
		beamcount(0), 
		shockcount(0), 
		fixescount(0), 
		hydroscount(0), 
		wheelcount(0), 
		propwheelcount(0), 
		commandscount(0), 
		flarescount(0), 
		propscount(0), 
		wingscount(0), 
		turbopropscount(0), 
		turbojetcount(0), 
		rotatorscount(0), 
		exhaustscount(0), 
		flexbodiescount(0), 
		materialflarebindingscount(0), 
		soundsourcescount(0), 
		managedmaterialscount(0), 
		truckmass(0), 
		loadmass(0), 
		minrpm(0), 
		maxrpm(0), 
		torque(0), 
		customtach(false), 
		custom_particles(false), 
		forwardcommands(false), 
		importcommands(false), 
		rollon(false), 
		rescuer(false), 
		driveable(0), 
		numgears(0), 
		enginetype('t'),
		materials()
	{
		// driveable = 0 = NOT_DRIVEABLE
		// enginetype = t = truck is default
	}
#ifdef USE_ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

};

class CacheSystem : public MemoryAllocatedObject
{
public:
	static CacheSystem &Instance();
	

#ifdef USE_ANGELSCRIPT
	// we have to add this to be able to use the class as reference inside scripts
	void addRef(){};
	void release(){};
#endif

	void startup(SceneManager *smgr, bool forcecheck=false);
	void loadAllZips();
	
	static Ogre::String stripUIDfromString(Ogre::String uidstr);
	static Ogre::String getUIDfromString(Ogre::String uidstr);
	static bool stringHasUID(Ogre::String uidstr);
	
	void loadAllZipsInResourceGroup(Ogre::String group);

	bool checkResourceLoaded(Cache_Entry t);
	bool checkResourceLoaded(Ogre::String &filename);
	bool checkResourceLoaded(Ogre::String &filename, Ogre::String &group);
	Cache_Entry getResourceInfo(Ogre::String &filename);
	Ogre::String addMeshMaterials(Cache_Entry &entry, Ogre::Entity *e);
	std::map<int, Category_Entry> *getCategories();
	std::vector<Cache_Entry> *getEntries();

	int getCategoryUsage(int category);
	Cache_Entry *getEntry(int modid);

	int getTimeStamp();

	//these location MUST include a path separator at the end!
	void setLocation(String cachepath, String configpath);
	
	Ogre::String getSkinSource(Ogre::String filename);

	// this is for stats only, maybe protect it by getter later
	int changedFiles, newFiles, deletedFiles;
	bool fileExists(Ogre::String);

	void loadSingleZip(Ogre::String zippath, int cfactor, bool unload=true, bool ownGroup=true);
	void loadSingleDirectory(String dirname, String group, bool alreadyLoaded=true);

	static bool resourceExistsInAllGroups(Ogre::String filename);
protected:
	CacheSystem();
	~CacheSystem();
	CacheSystem(const CacheSystem&);
	CacheSystem& operator= (const CacheSystem&);
	static CacheSystem* myInstance;
	String location;
	String configlocation;
	Ogre::SceneManager *smgr;


	Ogre::String currentSHA1;	// stores sha1 over the content
	int rgcounter;				// resource group counter, used to track the resource groups created
	int modcounter;	// counter the number of mods



	// the extensions we track in the cache system
	std::vector<Ogre::String> known_extensions;

	int addUniqueString(std::set<Ogre::String> &list, Ogre::String str);

	// map for skins and zips
	std::map<Ogre::String, Ogre::String> skins_map;

	// parses all files loaded with a certain extension
	void parseFilesAllRG(Ogre::String ext);
	void parseFilesOneRG(Ogre::String ext, Ogre::String rg);
	void parseKnownFilesOneRG(Ogre::String rg);
	void parseKnownFilesAllRG();
	void parseKnownFilesOneRGDirectory(Ogre::String rg, Ogre::String dir);

	void checkForNewKnownFiles();


	void addFile(Ogre::FileInfo f, String ext);	// adds a file to entries
	void addFile(Ogre::String filename, Ogre::String archiveType, Ogre::String archiveDirectory, Ogre::String ext);

	// reads all advanced infromation out of the entry's file
	void fillTerrainDetailInfo(Cache_Entry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);
	void fillTruckDetailInfo(Cache_Entry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);

	int isCacheValid();			// validate cache
	void unloadUselessResourceGroups();		// unload unused resources afer cache generation
	Ogre::String filenamesSHA1();	// generates the hash over the whole content
	bool loadCache();			// loads cache config file, new format
	String getCacheConfigFilename(bool full);	// returns filename of the cache file
	int incrementalCacheUpdate();	// tries to update parts of the Cache only

	void generateFileCache(Cache_Entry &entry, Ogre::String directory=Ogre::String());	// generates a new cache
	void deleteFileCache(char *filename); // removed files from cache
	void writeGeneratedCache();
	void writeStreamCache();
	
	// adds a zip to the cache
	void loadSingleZip(Ogre::FileInfo f, bool unload=true, bool ownGroup=true);
	void loadSingleZip(Cache_Entry e, bool unload=true, bool ownGroup=true);

	Ogre::String detectFilesMiniType(Ogre::String filename);
	void removeFileFromFileCache(std::vector<Cache_Entry>::iterator it);
	void generateCache(bool forcefull=false);
	Ogre::String formatEntry(int counter, Cache_Entry t);
	Ogre::String formatInnerEntry(int counter, Cache_Entry t);
	Ogre::String formatSkinEntry(int counter, Skin *skin);
	void updateSingleTruckEntryCache(int number, Cache_Entry t);
	void parseModAttribute(const Ogre::String& line, Cache_Entry& t);
	void logBadTruckAttrib(const Ogre::String& line, Cache_Entry& t);

	// this holds all files
	std::vector<Cache_Entry> entries;

	std::map<Ogre::String, Ogre::String> zipHashes;

	// categories
	std::map<int, Category_Entry> categories;
	std::map<int, int> category_usage;
	std::set<Ogre::String> zipCacheList;
	void readCategoryTitles();


	// helpers
	char *replacesSpaces(char *str);
	char *restoreSpaces(char *str);
	
	Ogre::String fileTime(Ogre::String);

	Ogre::String getRealPath(Ogre::String path);
	Ogre::String getVirtualPath(Ogre::String path);

	Ogre::String normalizeText(Ogre::String text);
	Ogre::String deNormalizeText(Ogre::String text);
	
	void checkForNewFiles(Ogre::String ext);

	void checkForNewContent();

	void checkForNewZipsInResourceGroup(Ogre::String group);
	void checkForNewDirectoriesInResourceGroup(String group);

	void generateZipList();
	bool isZipUsedInEntries(Ogre::String filename);
	bool isFileInEntries(Ogre::String filename);

	bool isDirectoryUsedInEntries(Ogre::String directory);

	void loadAllDirectoriesInResourceGroup(String group);

};


#endif
