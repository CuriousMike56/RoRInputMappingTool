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
#include "platform.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include "wx/msw/private.h"
	#include "wx/msw/registry.h"
	#include <shlobj.h>
#endif // OGRE_PLATFORM

#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>

#include "ConfigManager.h"
#include "wthread.h"
#include "wsyncdownload.h"
#include "utils.h"
#include "installerlog.h"
#include "SHA1.h"
#include "wizard.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include <shlobj.h> // for the special path functions
	#include "symlink.h"
	#include <shellapi.h> // for executing the binaries
#endif //OGRE_PLATFORM

#include "boost/filesystem.hpp"

//icons must be 50x50
#include "unknown.xpm"
#include "mainpack.xpm"
#include "extrapack.xpm"
#include <string>

ConfigManager *ConfigManager::instance = 0;

ConfigManager::ConfigManager() : currVersion()
{
	ConfigManager::instance = this;
	streams.clear();
	installPath = wxString();
	dlerror=0;
}

int ConfigManager::getCurrentVersionInfo()
{
	// get the most recent version information
	WsyncDownload *wsdl = new WsyncDownload();
	std::vector< std::vector< std::string > > list;
	int res = wsdl->downloadConfigFile(WSYNC_MAIN_SERVER, WSYNC_VERSION_INFO, list);
	delete(wsdl);
	if(!res && list.size()>0 && list[0].size()>0)
	{
		currVersion = list[0][0];
		return 0;
	} else
	{
		currVersion = std::string();
	}
	return 1;
}

int ConfigManager::writeVersionInfo()
{
	wxString fn = getInstallationPath() + wxT("\\version");
	FILE *f = fopen(conv(fn).c_str(), "w");
	if(!f) return -1;
	fprintf(f, "%s", currVersion.c_str());
	fclose(f);
	return 0;
}

std::string ConfigManager::readVersionInfo()
{
	wxString fn = getInstallationPath() + wxT("\\version");
	FILE *f = fopen(conv(fn).c_str(), "r");
	if(!f) return std::string("unkown");
	char tmp[256]="";
	int res = fscanf(f, "%s", tmp);
	fclose(f);
	if(res>0)
		return std::string(tmp);
	return std::string("unkown");
}

int ConfigManager::getOnlineStreams()
{
	if(streams.size() > 0) return 0; // already downloaded
	//clear list
	clearStreamset();

	// now get the available streams list
	std::vector< std::map< std::string, std::string > > olist;

	WsyncDownload *wsdl = new WsyncDownload();
	int res = wsdl->downloadAdvancedConfigFile("wsync.rigsofrods.com", "/streams.index", olist, true);
	if(res == -1)
	{
		wxMessageBox(_T("error creating tempfile for download"), _T("Error"), wxICON_ERROR | wxOK);
	} else if (res == -2)
	{
		std::string errorMsg;// = w->getLastError();
		wxMessageBox(_T("error downloading file:\n")/*+errorMsg*/, _T("Error"), wxICON_ERROR | wxOK);
	} else if (res == -3)
	{
		wxMessageBox(_T("unable to open local file for reading"), _T("Error"), wxICON_ERROR | wxOK);
	} else if (res == -4)
	{
		wxMessageBox(_T("unable to download file, content incorrect: http://wsync.rigsofrods.com/streams.index"), _T("Error"), wxICON_ERROR | wxOK);
	}
	delete wsdl;

	if(!res)
	{
		// no error so far
		if(olist.size() > 0)
		{
			for(int i=0;i<(int)olist.size();i++)
			{
				stream_desc_t s;
				s.title     = conv(olist[i]["title"]);
				s.desc      = conv(olist[i]["description"]);
				s.conflict  = conv(olist[i]["conflict"]);
				s.group     = conv(olist[i]["group"]);
				s.platform  = conv(olist[i]["platform"]);
				s.path      = conv(olist[i]["path"]);

				// OLD version of getting the version, now obsolete
				//if(olist[i]["version"].size() > 0 && currVersion.empty())
				//	currVersion = olist[i]["version"];

				s.icon      = (olist[i]["type"]=="0")?wxBitmap(mainpack_xpm):wxBitmap(extrapack_xpm);
				s.checked   = (olist[i]["checked"] == "1");
				s.forcecheck= (olist[i]["forcecheck"] == "1");
				s.hidden    = (olist[i]["hidden"] == "1");
				s.binary    = (olist[i]["binary"] == "1");
				s.resource  = (olist[i]["resource"] == "1");
				s.disabled  = (olist[i]["disabled"] == "1");
				s.beta      = (olist[i]["beta"] == "1");
				s.stable    = (olist[i]["stable"] == "1");
				s.del       = (olist[i]["delete"] == "1");
				s.content   = (olist[i]["content"] == "1");
				s.overwrite = (olist[i]["overwrite"] == "1");
				//s.overwrite = (olist[i]["overwrite"] == "1");
				conv(olist[i]["size"]).ToULong(&s.size);

				if(!s.title.size()) continue;

				// check for platform
				if(!s.platform.empty() && s.platform != wxT("all") && s.platform != wxT("ALL"))
				{
					// there is a platform restriction, so check it
					if(s.platform != wxT(INSTALLER_PLATFORM))
					{
						//printf("discarding stream '%s' because of wrong platform: %s [%s]\n", conv(s.title).c_str(), conv(s.platform).c_str(), INSTALLER_PLATFORM);
						// platform does not fit, ignore this stream

						continue;
					}
				}

				streams.push_back(s);
			}

		} else
			return 1;
	}

	if(!streams.size() && dlerror < 3)
	{
		// try to download three times...
		dlerror++;
		return getOnlineStreams();
	}


	//add default streams
	//appendStream(_T("Base game"), _T("The minimum you need to run the game."), wxBitmap(mainpack_xpm), true, true);
	//appendStream(_T("Standard media pack"), _T("The best terrains and vehicles, highly recommended!"), wxBitmap(extrapack_xpm), true, false);
	//for (int i=0; i<5; i++)
	//	appendStream(_T("Test pack"), _T("This is a test"), wxBitmap(unknown_xpm), false, false);

	loadStreamSubscription();

	return 0;

}

wxString ConfigManager::getInstallationPath()
{
	if(installPath.empty())
	{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		wxString path;
		wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods"));
		if(!pRegKey->Exists())
			return wxString();

		if(!pRegKey->HasValue(wxT("InstallPath")))
			return wxString();

		pRegKey->QueryValue(wxT("InstallPath"), path);
		path += wxT("\\");
		// check if RoR.exe exists
		bool exists = wxFileExists(path+wxT("RoR.exe"));
		if(!exists)
			return wxString();
		// existing, everything correct.
		installPath = path;
		// see http://docs.wxwidgets.org/stable/wx_wxregkey.html
		//wxMessageBox(path,wxT("Registry Value"),0);
#else
		// TODO: implement
		installPath = wxString();
#endif //OGRE_PLATFORM
	}
	return installPath;
}

int ConfigManager::uninstall(bool deleteUserFolder)
{
	// TODO: implement uninstall for non-windows versions!
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxString ipath = getInstallationPath();
	if(ipath.empty())
	{
		wxMessageBox(_T("Installation Path empty?!"), _T("Error"), wxICON_ERROR | wxOK);
		return 1;
	}

	wxString mtxt = _T("Will now:\n\n");
	mtxt += wxT("remove the main installation directory recursivly:\n") + ipath + wxT("\n\n");

	wxString userPath;
	if(deleteUserFolder)
	{
		LPWSTR wuser_path = new wchar_t[1024];
		if (SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, wuser_path)!=S_OK)
			return 3;
		GetShortPathName(wuser_path, wuser_path, 512); //this is legal
		std::string user_path_str = wstrtostr(std::wstring(wuser_path));
		user_path_str += "\\Rigs of Rods\\";
		userPath = conv(user_path_str);
		mtxt += wxT("remove the Rigs of Rods user content directory recursivly:\n") + userPath + wxT("\n\n");
	}


	// remove shortcuts
	wxString startmenuDir, desktopDir, workingDirectory = ipath, desktopLink;
	if(!SHGetSpecialFolderPath(0, wxStringBuffer(startmenuDir, MAX_PATH), CSIDL_COMMON_PROGRAMS, FALSE))
	{
		wxMessageBox(_T("Error getting Startmenu directory"), _T("Error"), wxICON_ERROR | wxOK);
		return 8;
	}
	if(!SHGetSpecialFolderPath(0, wxStringBuffer(desktopDir, MAX_PATH), CSIDL_DESKTOP, FALSE))
	{
		wxMessageBox(_T("Error getting Desktop directory"), _T("Error"), wxICON_ERROR | wxOK);
		return 9;
	}

	if(workingDirectory.size() > 3 && workingDirectory.substr(workingDirectory.size()-1,1) != wxT("\\"))
		workingDirectory += wxT("\\");

	startmenuDir += wxT("\\Rigs of Rods");
	if (!wxDir::Exists(startmenuDir))
		startmenuDir = wxString();

	desktopLink = desktopDir + wxT("\\Rigs of Rods.lnk");
	if(!wxFileName::FileExists(desktopLink))
		desktopLink = wxString();

	if(!startmenuDir.empty())
		mtxt += wxT("remove the Rigs of Rods start menu directory:\n") + startmenuDir + wxT("\n\n");
	if(!desktopLink.empty())
		mtxt += wxT("remove the Rigs of Rods Desktop Link:\n") + desktopLink + wxT("\n");

	mtxt += wxT("remove the Rigs of Rods Registry entry:\nHKEY_LOCAL_MACHINE\\Software\\RigsOfRods \n\n");
	mtxt += wxT("do you want to continue?");
	int res = wxMessageBox(mtxt, _T("Uninstall?"), wxICON_QUESTION | wxYES_NO);
	if(res != wxYES)
	{
		return 10;
	}

	// this is called upon uninstall to clean the system from meta things
	wxFileName *f = new wxFileName(ipath);
	// wxPATH_RMDIR_RECURSIVE is available in wxWidgets >= 2.9.0
#if wxCHECK_VERSION(2, 9, 0)
	bool rmres = f->Rmdir(wxPATH_RMDIR_RECURSIVE);
#else
	#error You need at least wxWidgets version 2.9.0 in order to compile the installer correctly!
#endif
	if(!rmres)
	{
		wxMessageBox("Could not remove installation directory recursively", _T("Error"), wxICON_ERROR | wxOK);
		return 2;
	}

	if(deleteUserFolder && !userPath.empty())
	{
		wxFileName *f = new wxFileName(userPath);
#if wxCHECK_VERSION(2, 9, 0)
		bool res = f->Rmdir(wxPATH_RMDIR_RECURSIVE);
#else
		#error You need at least wxWidgets version 2.9.0 in order to compile the installer correctly!
#endif
		if(!res)
		{
			wxMessageBox("Could not remove user directory recursively", _T("Error"), wxICON_ERROR | wxOK);
			return 4;
		}
	}
	// remove registry keys
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods"));
	if(pRegKey->Exists())
		pRegKey->DeleteSelf();

	pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Rigs of Rods"));
	if(pRegKey->Exists())
		pRegKey->DeleteSelf();

	// remove shortcuts
	if(!startmenuDir.empty())
#if wxCHECK_VERSION(2, 9, 0)
		wxFileName::Rmdir(startmenuDir, wxPATH_RMDIR_RECURSIVE);
#else
		#error You need at least wxWidgets version 2.9.0 in order to compile the installer correctly!
#endif

	if(!desktopLink.empty())
		wxRemoveFile(desktopLink);
#endif // OGRE_PLATFORM
	return 0;
}

void ConfigManager::associateViewerFileTypes(std::string type)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// add the icon
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_CLASSES_ROOT\\" + type + "\\DefaultIcon"));
	if(!pRegKey->Exists())
		pRegKey->Create();

	pRegKey->SetValue("", installPath + "\\RoRViewer.exe,0");

	// add the action
	pRegKey = new wxRegKey(wxT("HKEY_CLASSES_ROOT\\" + type + "\\shell\\open\\command"));
	if(!pRegKey->Exists())
		pRegKey->Create();

	pRegKey->SetValue("", installPath + "\\RoRViewer.exe \"%1\" ");
#else
	// TODO: implement
#endif //OGRE_PLATFORM
}
int ConfigManager::associateFileTypes()
{
	associateViewerFileTypes(".mesh");
	return 0;
}

void ConfigManager::saveStreamSubscription()
{
	if(!streams.size()) return;
	for(std::vector < stream_desc_t >::iterator it=streams.begin(); it!=streams.end(); it++)
	{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\streams"));
		if(!pRegKey->Exists())
			pRegKey->Create();
		pRegKey->SetValue(it->path, it->checked?wxT("yes"):wxT("no"));
#else
	// TODO: implement
#endif //OGRE_PLATFORM
	}
}

void ConfigManager::saveStreamSubscription(wxString streamPath, bool value)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\streams"));
	if(!pRegKey->Exists())
		pRegKey->Create();
	pRegKey->SetValue(streamPath, value?wxT("yes"):wxT("no"));
#else
	// TODO: implement
#endif //OGRE_PLATFORM
}

bool ConfigManager::isStreamSubscribed(wxString streamPath)
{
	if(!streams.size()) return false;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\streams"));
	if(!pRegKey->Exists())
		return false;

	wxString enabled;
	if(!pRegKey->HasValue(streamPath)) return false;
	pRegKey->QueryValue(streamPath, enabled);
	if(enabled == wxT("yes"))
		return true;
#else
	// TODO: implement
#endif //OGRE_PLATFORM
	return false;
}

void ConfigManager::clearStreamSubscription()
{
	if(!streams.size()) return;
	for(std::vector < stream_desc_t >::iterator it=streams.begin(); it!=streams.end(); it++)
		it->checked = false;
}

void ConfigManager::streamSubscriptionDebug()
{
	if(!streams.size()) return;
	LOG("==== Stream Subscriptions:\n");
	for(std::vector < stream_desc_t >::iterator it=streams.begin(); it!=streams.end(); it++)
		LOG("  %s(%s) : %s\n", conv(it->title).c_str(), conv(it->path).c_str(), it->checked?"YES":"NO");
}


void ConfigManager::loadStreamSubscription()
{
	if(!streams.size()) return;
	for(std::vector < stream_desc_t >::iterator it=streams.begin(); it!=streams.end(); it++)
	{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		if(it->forcecheck) continue; // we enforce the value of this
		wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\streams"));
		if(!pRegKey->Exists())
			return;

		wxString enabled;
		if(!pRegKey->HasValue(it->path)) continue;
		pRegKey->QueryValue(it->path, enabled);
		if(enabled == wxT("yes"))
			it->checked = true;
		else if(enabled == wxT("no"))
			it->checked = false;
#else
		// TODO: implement
#endif //OGRE_PLATFORM
	}
}


void ConfigManager::setPersistentConfig(wxString name, wxString value)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\config"));
	if(!pRegKey->Exists())
		pRegKey->Create();
	pRegKey->SetValue(name, value);
#else
	// TODO: implement
#endif //OGRE_PLATFORM
}

wxString ConfigManager::getPersistentConfig(wxString name)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods\\config"));
	if(!pRegKey->Exists())
		return wxT("");

	if(!pRegKey->HasValue(name))
		return wxT("");

	wxString result;
	pRegKey->QueryValue(name, result);
	return result;
#else
	// TODO: implement
	return wxString();
#endif //OGRE_PLATFORM
}

void ConfigManager::setInstallationPath()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxRegKey *pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\Software\\RigsOfRods"));
	if(!pRegKey->Exists())
		pRegKey->Create();
	pRegKey->SetValue(wxT("InstallPath"), installPath);

	// add theuninstall Info
	pRegKey = new wxRegKey(wxT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Rigs of Rods"));
	if(!pRegKey->Exists())
		pRegKey->Create();
	pRegKey->SetValue(wxT("DisplayName"), wxT("Rigs of Rods"));
	pRegKey->SetValue(wxT("DisplayVersion"), wxT("installer version"));
	pRegKey->SetValue(wxT("Publisher"), wxT("Rigs of Rods Team"));
	pRegKey->SetValue(wxT("UninstallString"), wxT("\"") + installPath + wxT("\\installer.exe\""));
	pRegKey->SetValue(wxT("URLInfoAbout"), wxT("http://www.rigsofrods.com"));
	pRegKey->SetValue(wxT("URLUpdateInfo"), wxT("http://www.rigsofrods.com"));
	pRegKey->SetValue(wxT("DisplayIcon"), wxT("\"") + installPath + wxT("\\ror.exe\""));
	pRegKey->SetValue(wxT("HelpLink"), wxT("http://forum.rigsofrods.com/index.php?board=10.0"));
	//pRegKey->SetValue(wxT("InstallDate"), wxT(""));
	pRegKey->SetValue(wxT("InstallLocation"), installPath);

#else
	// TODO: implement
#endif //OGRE_PLATFORM
}

bool ConfigManager::isFirstInstall()
{
	wxString path = getInstallationPath();
	installPath = path; //dont use setter, because it would write into the registry again
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	if(path.empty())
		return true;
#endif //OGRE_PLATFORM
	return !wxFileExists(path + wxT("RoR.exe"));
}

bool ConfigManager::isLicenceAccepted()
{
	return !isFirstInstall();
}

void ConfigManager::setAction(int ac)
{
	installeraction = ac;
}

int ConfigManager::getAction()
{
	return installeraction;
}

void ConfigManager::setInstallPath(wxString pth)
{
	installPath=pth;
	setInstallationPath();
}

std::vector < stream_desc_t > *ConfigManager::getStreamset()
{
	return &streams;
}

void ConfigManager::clearStreamset()
{
	streams.clear();
}

void ConfigManager::setStreamSelection(stream_desc_t* desc, bool selection)
{
	for(std::vector < stream_desc_t >::iterator it=streams.begin(); it!=streams.end(); it++)
	{
		// we use the path, since it is unique
		if(it->path == desc->path)
		{
			//if (!it->disabled)
			it->checked=selection;
			saveStreamSubscription(it->path, selection);
			return;
		}
	}
	//if we are here, an invalid desc has been provided! better safe than sorry.
}









void ConfigManager::updateUserConfigFile(std::string filename, boost::filesystem::path iPath, boost::filesystem::path uPath)
{
	boost::filesystem::path fPath = iPath / filename;
	boost::filesystem::path dfPath = uPath / filename;
	if(boost::filesystem::exists(dfPath))
	{
		WsyncDownload::tryRemoveFile(dfPath);
	}
	LOG("updating file: %s ... ", fPath.string().c_str());

	bool ok = boost::filesystem::is_regular_file(fPath);
	if(ok)
	{
		try
		{
			boost::filesystem::copy_file(fPath, dfPath);
			ok=true;
		} catch(...)
		{
			ok=false;
		}
	}

	LOG("%s\n", ok?"ok":"error");
}

void ConfigManager::updateUserConfigs()
{
	LOG("==== updating user configs ... \n");

	boost::filesystem::path iPath = boost::filesystem::path (conv(getInstallationPath()));
	iPath = iPath / std::string("skeleton") / std::string("config");

	boost::filesystem::path uPath;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	LPWSTR wuser_path = new wchar_t[1024];
	if (!SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, wuser_path)!=S_OK)
	{
		GetShortPathName(wuser_path, wuser_path, 512); //this is legal
		uPath = wstrtostr(std::wstring(wuser_path));
		uPath = uPath / std::string("Rigs of Rods") / std::string("config");;
	}
#else
	// XXX : TODO
#endif // OGRE_PLATFORM

	// check directories

	LOG("installation path: %s ... ", iPath.string().c_str());
	bool ok = boost::filesystem::is_directory(iPath);
	LOG("%s\n", ok?"ok":"error");

	LOG("user path: %s ... ", uPath.string().c_str());
	ok = boost::filesystem::is_directory(uPath);
	LOG("%s\n", ok?"ok":"error");

	updateUserConfigFile(std::string("categories.cfg"), iPath, uPath);
	updateUserConfigFile(std::string("ground_models.cfg"), iPath, uPath);
	updateUserConfigFile(std::string("torque_models.cfg"), iPath, uPath);
	updateUserConfigFile(std::string("wavefield.cfg"), iPath, uPath);
}

void ConfigManager::installRuntime()
{
	wxMessageBox(wxT("Will now install DirectX. Please click ok to continue"), _T("directx"), wxICON_INFORMATION | wxOK);
	executeBinary(wxT("dxwebsetup.exe"));
	wxMessageBox(wxT("Please wait until the DirectX installation is done and click ok to continue"), _T("directx"), wxICON_INFORMATION | wxOK);
	wxMessageBox(wxT("Will now install the Visual Studio runtime. Please click ok to continue."), _T("runtime"), wxICON_INFORMATION | wxOK);
	executeBinary(wxT("msiexec.exe"), wxT("runas"), wxT("/i \"") + CONFIG->getInstallationPath() + wxT("\\VCCRT4.msi\""), wxT("cwd"), false);
}

void ConfigManager::startConfigurator()
{
	executeBinary(wxT("rorconfig.exe"), wxT("runas"), wxT(""), CONFIG->getInstallationPath());
}

void ConfigManager::viewManual()
{
	executeBinary(wxT("Things_you_can_do_in_Rigs_of_Rods.pdf"), wxT("open"));
	executeBinary(wxT("keysheet.pdf"), wxT("open"));
}

void ConfigManager::viewChangelog()
{
	wxString changeLogURL = wxT(CHANGELOGURL) + conv(currVersion);
	wxLaunchDefaultBrowser(changeLogURL);
}

void ConfigManager::executeBinary(wxString filename, wxString action, wxString parameters, wxString cwDir, bool prependCWD)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	char path[2048]= "";
	if(prependCWD)
		filename = CONFIG->getInstallationPath() + "\\" + filename;

	// now construct struct that has the required starting info
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.cbSize = sizeof(SHELLEXECUTEINFOA);
	sei.fMask = 0;
	sei.hwnd = NULL;
	sei.lpVerb = action;
	sei.lpFile = filename;
	sei.lpParameters = parameters;
	if(cwDir == wxT("cwd"))
		sei.lpDirectory = CONFIG->getInstallationPath();
	else
		sei.lpDirectory = cwDir;

	sei.nShow = SW_NORMAL;
	ShellExecuteEx(&sei);
#endif //OGRE_PLATFORM
}

void ConfigManager::createProgramLinks(bool desktop, bool startmenu)
{
	// XXX: TODO: ADD LINUX code!
	// create shortcuts
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxString startmenuDir, desktopDir, workingDirectory;

	// XXX: ADD PROPER ERROR HANDLING
	// CSIDL_COMMON_PROGRAMS = start menu for all users
	// CSIDL_PROGRAMS = start menu for current user only
	if(!SHGetSpecialFolderPath(0, wxStringBuffer(startmenuDir, MAX_PATH), CSIDL_COMMON_PROGRAMS, FALSE))
		return;

	// same with CSIDL_COMMON_DESKTOPDIRECTORY and CSIDL_DESKTOP
	if(!SHGetSpecialFolderPath(0, wxStringBuffer(desktopDir, MAX_PATH), CSIDL_DESKTOP, FALSE))
		return;

	workingDirectory = CONFIG->getInstallationPath();
	if(workingDirectory.size() > 3 && workingDirectory.substr(workingDirectory.size()-1,1) != wxT("\\"))
	{
		workingDirectory += wxT("\\");
	}
	// ensure our directory in the start menu exists
	startmenuDir += wxT("\\Rigs of Rods");
	if (!wxDir::Exists(startmenuDir)) wxFileName::Mkdir(startmenuDir);

	// the actual linking
	if(desktop)
		createShortcut(workingDirectory + wxT("rorconfig.exe"), workingDirectory, desktopDir + wxT("\\Rigs of Rods.lnk"), wxT("start Rigs of Rods"));

	if(startmenu)
	{
		createShortcut(workingDirectory + wxT("RoR.exe"), workingDirectory, startmenuDir + wxT("\\Rigs of Rods.lnk"), wxT("start Rigs of Rods"));
		createShortcut(workingDirectory + wxT("rorconfig.exe"), workingDirectory, startmenuDir + wxT("\\Configurator.lnk"), wxT("start Rigs of Rods Configuration Program (required upon first start)"));
		createShortcut(workingDirectory + wxT("servergui.exe"), workingDirectory, startmenuDir + wxT("\\Multiplayer Server.lnk"), wxT("start Rigs of Rods multiplayer server"));
		createShortcut(workingDirectory + wxT("Things_you_can_do_in_Rigs_of_Rods.pdf"), workingDirectory, startmenuDir + wxT("\\Manual.lnk"), wxT("open the RoR Manual"));
		createShortcut(workingDirectory + wxT("keysheet.pdf"), workingDirectory, startmenuDir + wxT("\\Keysheet.lnk"), wxT("open the RoR Key Overview"));
		createShortcut(workingDirectory + wxT("installer.exe"), workingDirectory, startmenuDir + wxT("\\Installer (update or uninstall).lnk"), wxT("open the Installer with which you can update or uninstall RoR."));
	}
#endif // OGRE_PLATFORM
}

// small wrapper that converts wxString to std::string and remvoes the file if already existing
int ConfigManager::createShortcut(wxString linkTarget, wxString workingDirectory, wxString linkFile, wxString linkDescription)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	if(wxFileExists(linkFile)) wxRemoveFile(linkFile);
	if(!wxFileExists(linkTarget)) return 1;
	return createLink(conv(linkTarget), conv(workingDirectory), conv(linkFile), conv(linkDescription));
#endif //OGRE_PLATFORM
	return 1;
}

std::string ConfigManager::getOwnHash()
{
	// now hash ourself and validate our installer version, to be sure we are using the latest installer
	std::string myPath = conv(MyApp::getExecutablePath());

	CSHA1 sha1;
	bool res = sha1.HashFile(const_cast<char*>(myPath.c_str()));
	if(!res) return std::string("");
	sha1.Final();
	char resultHash[256] = "";
	sha1.ReportHash(resultHash, CSHA1::REPORT_HEX_SHORT);
	return std::string(resultHash);
}

void ConfigManager::checkForNewInstaller()
{
	std::string ourHash = getOwnHash();

    char platform_str[256]="";
#ifdef WIN32
   sprintf(platform_str, "windows");
#else
   sprintf(platform_str, "linux");
#endif

    char url_tmp[256]="";
	sprintf(url_tmp, API_CHINSTALLER, ourHash.c_str(), platform_str);

	WsyncDownload *wsdl = new WsyncDownload();
	LOG("checking for installer updates...\n");
	wsdl->setDownloadMessage(_T("checking for installer updates"));
	std::vector< std::vector< std::string > > list;
	int res = wsdl->downloadConfigFile(API_SERVER, std::string(url_tmp), list, true);
	if(!res && list.size()>0 && list[0].size()>0)
	{
		LOG("installer check update result: %s\n", list[0][0].c_str());
		if(list[0][0] == std::string("ok"))
		{
			// no updates
		} else if(list[0][0] == std::string("update") && list[0].size() > 2)
		{
			// yay, an update
			wsdl->setDownloadMessage(_T("downloading installer update"));

			// rename ourself, so we can replace ourself
			std::string myPath = conv(MyApp::getExecutablePath());
			boost::filesystem::rename(myPath, myPath+std::string(".old"));

			int res = wsdl->downloadFile(0, myPath, list[0][1], list[0][2], 0, 0, true);
			if(!res)
			{
				wxMessageBox(_T("Installer was updated, will restart the installer now!"));

				// now start the new installer and quit ourselfs
				executeBinary(conv(myPath), wxT("runas"), wxT(""), wxT("cwd"), false);
				exit(1);
			}
		}
	}
	delete(wsdl);
}