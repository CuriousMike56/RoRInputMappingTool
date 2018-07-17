#ifndef WTHREAD_H__
#define WTHREAD_H__

#include <wx/thread.h>
#include <wx/event.h>
#include <string.h>
#include "Timer.h"
#include "cevent.h"
#include "ConfigManager.h" //only use for stream_desc_t, dont use functions, as its not thread safe!
#include "threadpool.h"

#include "boost/filesystem.hpp"


#define INDEXFILENAME       "files.index"

// servers
#define API_SERVER          "api.rigsofrods.com"

#ifndef REPO_SERVER
# define REPO_SERVER        "repository.rigsofrods.com"
#endif //REPO_SERVER


// functions
#define API_MIRROR          "/getwsyncmirror/"
#define API_MIRROR_NOGEO    "/getwsyncmirror/?ignoregeo"
#define API_RECORDTRAFFIC   "/reporttraffic/?mirror=%s&bytes=%d"
#define API_REPOSEARCH      "/reposearch/"
#define API_CHINSTALLER     "/checkinstallerupdate/?h=%s&platform=%s"

#define REPO_DOWNLOAD       "/files/mirror/geoselect/"

#define WSYNC_MAIN_SERVER   "wsync.rigsofrods.com"
#define WSYNC_MAIN_DIR      "/"
#define WSYNC_VERSION_INFO  "/version"

#define CHANGELOGURL        "http://wiki.rigsofrods.com/pages/Changelog#"

#define WMO_FULL     0x00000001
#define WMO_NODELETE 0x00000010

class Hashentry
{
public:
	std::string hash;
	boost::uintmax_t filesize;
	Hashentry()
	{
	}
	Hashentry(std::string hash, boost::uintmax_t filesize) : hash(hash), filesize(filesize)
	{
	}
};

class Fileentry
{
public:
	std::string filename;
	std::string stream_path;
	boost::uintmax_t filesize;
	Fileentry()
	{
	}
	Fileentry(std::string stream_path, std::string filename, boost::uintmax_t filesize) : filename(filename), stream_path(stream_path), filesize(filesize)
	{
	}
};

enum { CMD_THREAD_DONE };

typedef struct dlstatus_t
{
	int status;
	float percent;
	std::string path;
	std::string text;
	float time;
	float time_remaining;
	int speed;
	boost::uintmax_t filesize;
	boost::uintmax_t downloaded;
} dlstatus_t;

class WsyncThread : public wxThread, public wxEvtHandler
{
public:
	WsyncThread(wxEvtHandler *parent, wxString ipath, std::vector < stream_desc_t > streams);
	~WsyncThread();

	static std::string generateFileHash(boost::filesystem::path file);
protected:
	// members
	wxEvtHandler *parent; // where we send events to
	WsyncDownloadManager *dlm; // the download manager
	boost::filesystem::path ipath; // installation path, what we work on
	boost::uintmax_t predDownloadSize;
	Timer dlStartTime;
	Timer updateTimer;
	int dlNum;
	std::map<int, dlstatus_t> dlStatus;

	std::string mainserver, server, mainserverdir, serverdir;
	std::vector < stream_desc_t > streams;
	std::map < std::string, unsigned int > traffic_stats;


	// functions
	virtual ExitCode Entry(); // thread entry point

	// helper to construct event
	void updateCallback(int type, std::string txt = std::string(), float percent=-1);

	// special sync with visual event feedback
	int sync();

	int buildFileIndex(boost::filesystem::path &outfilename, boost::filesystem::path &path, boost::filesystem::path &rootpath, std::map<std::string, Hashentry> &hashMap, bool writeFile, int mode);
	void debugOutputHashMap(std::map<std::string, Hashentry> &hashMap);

	// wsync related functions
	int getSyncData();
	void recordDataUsage();
	int findMirror(bool probeForBest=false);
	
	void listFiles(const boost::filesystem::path &dir_path, std::vector<std::string> &files);
	int saveHashMapToFile(boost::filesystem::path &filename, std::map<std::string, Hashentry> &hashMap, int mode);
	int loadHashMapFromFile(boost::filesystem::path &filename, std::map<std::string, Hashentry> &hashMap, int &mode);
	static std::string findHashInHashmap(std::map<std::string, Hashentry> hashMap, std::string filename);
	static std::string findHashInHashmap(std::map < std::string, std::map < std::string, Hashentry > > hashMap, std::string filename);
	double measureDownloadSpeed(std::string server, std::string url);

	void onDownloadStatusUpdate(MyStatusEvent &ev);
	void reportProgress();
	void addJob(wxString localFile, wxString remoteDir, wxString remoteServer, wxString remoteFile, wxString hashRemoteFile, wxString localFilename, boost::uintmax_t filesize);

	std::map < std::string, Hashentry> hashMapLocal;
	std::map < std::string, std::map<std::string, Hashentry> > hashMapRemote; // stream hashmaps, first key is stream path

	DECLARE_EVENT_TABLE();
};
#endif //WTHREAD_H__
