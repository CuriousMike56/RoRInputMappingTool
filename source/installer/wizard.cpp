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

#include "wizard.h"
#include "cevent.h"
#include "installerlog.h"
// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#include <wx/filename.h>
#include <wx/dataobj.h>
#include <wx/cmdline.h>
#include <wx/dir.h>
#include <wx/thread.h>
#include <wx/event.h>
#include <wx/fs_inet.h>
#include <wx/html/htmlwin.h>
#include <wx/settings.h>
#include "wthread.h"
#include "cevent.h"
#include "installerlog.h"

// for all others, include the necessary headers
#ifndef WX_PRECOMP
	#include "wx/frame.h"
	#include "wx/stattext.h"
	#include "wx/log.h"
	#include "wx/app.h"
	#include "wx/checkbox.h"
	#include "wx/checklst.h"
	#include "wx/msgdlg.h"
	#include "wx/radiobox.h"
	#include "wx/menu.h"
	#include "wx/sizer.h"
	#include "wx/textctrl.h"
	#include "wx/button.h"
	#include "wx/dirdlg.h"
	#include "wx/filename.h"
	#include "wx/dir.h"
	#include "wx/choice.h"
	#include "wx/gauge.h"
	#include "wx/timer.h"
	#include "wx/scrolwin.h"
#endif

#include "wx/wizard.h"
#include "wxStrel.h"
#include "wx/filename.h"
#include <wx/clipbrd.h>

#include "ConfigManager.h"

#include "welcome.xpm"
#include "licence.xpm"
#include "dest.xpm"
#include "streams.xpm"
#include "action.xpm"
#include "download.xpm"
#include "finished.xpm"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include <shlobj.h> // for the special path functions
	#include "symlink.h"
	#include <shellapi.h> // for executing the binaries
#endif //OGRE_PLATFORM


BEGIN_EVENT_TABLE(PathPage, wxWizardPageSimple)
    EVT_BUTTON(ID_BROWSE, PathPage::OnBrowse)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(DownloadPage, wxWizardPageSimple)
	EVT_TIMER(ID_TIMER, DownloadPage::OnTimer)
	EVT_MYSTATUS(wxID_ANY, DownloadPage::OnStatusUpdate )
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(StreamsPage, wxWizardPageSimple)
	//EVT_TIMER(ID_TIMER, DownloadPage::OnTimer)
	EVT_RADIOBOX(wxID_ANY, StreamsPage::OnBetaChange )
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MyWizard, wxWizard)
	EVT_WIZARD_PAGE_CHANGING(ID_WIZARD, MyWizard::OnPageChanging)
END_EVENT_TABLE()


IMPLEMENT_APP(MyApp)


// some helper
// from wxWidetgs wiki: http://wiki.wxwidgets.org/Calling_The_Default_Browser_In_WxHtmlWindow
class HtmlWindow: public wxHtmlWindow
{
public:
	HtmlWindow(wxWindow *parent, wxWindowID id = -1,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = wxHW_SCROLLBAR_NEVER|wxHW_NO_SELECTION|wxBORDER_SUNKEN, const wxString& name = _T("htmlWindow"));
	void OnLinkClicked(const wxHtmlLinkInfo& link);
};

HtmlWindow::HtmlWindow(wxWindow *parent, wxWindowID id, const wxPoint& pos,
	const wxSize& size, long style, const wxString& name)
: wxHtmlWindow(parent, id, pos, size, style, name)
{
  this->SetBorders(1);
}

void HtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
	wxString linkhref = link.GetHref();
    if(!wxLaunchDefaultBrowser(linkhref))
          // failed to launch externally, so open internally
          wxHtmlWindow::OnLinkClicked(link);
}


// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
#if wxCHECK_VERSION(2, 9, 0)
     { wxCMD_LINE_SWITCH, ("h"), ("help"),      ("displays help on the command line parameters"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
     { wxCMD_LINE_SWITCH, ("u"), ("update"),    ("update mode"), wxCMD_LINE_VAL_NONE },
     { wxCMD_LINE_SWITCH, ("r"), ("uninstall"), ("uninstall mode"), wxCMD_LINE_VAL_NONE },
     { wxCMD_LINE_SWITCH, ("i"), ("install"),   ("install mode"), wxCMD_LINE_VAL_NONE  },
     { wxCMD_LINE_SWITCH, ("g"), ("upgrade"),   ("upgrade mode"), wxCMD_LINE_VAL_NONE  },
     { wxCMD_LINE_SWITCH, ("n"), ("noupdate"),  ("ignore available updates"), wxCMD_LINE_VAL_NONE  },
     { wxCMD_LINE_SWITCH, ("d"), ("hash"),      ("put installer hash into clipboard"), wxCMD_LINE_VAL_NONE  },
#else
     { wxCMD_LINE_SWITCH, wxT("h"), wxT("help"),      wxT("displays help on the command line parameters"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
     { wxCMD_LINE_SWITCH, wxT("u"), wxT("update"),    wxT("update mode"), wxCMD_LINE_VAL_NONE },
     { wxCMD_LINE_SWITCH, wxT("r"), wxT("uninstall"), wxT("uninstall mode"), wxCMD_LINE_VAL_NONE },
     { wxCMD_LINE_SWITCH, wxT("i"), wxT("install"),   wxT("install mode"), wxCMD_LINE_VAL_NONE  },
     { wxCMD_LINE_SWITCH, wxT("g"), wxT("upgrade"),   wxT("upgrade mode"), wxCMD_LINE_VAL_NONE  },
#endif //wxCHECK_VERSION
	 { wxCMD_LINE_NONE }
};

// `Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
	autoUpdateEnabled=true;

    // call default behaviour (mandatory)
    if (!wxApp::OnInit())
        return false;

	MyWizard wizard(startupMode, NULL, autoUpdateEnabled);

	wizard.RunWizard(wizard.GetFirstPage());

	// we're done
	exit(0);

	// this crashes the app:
	return true;
}

void MyApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc (g_cmdLineDesc);
    // must refuse '/' as parameter starter or cannot use "/path" style paths
    parser.SetSwitchChars (wxT("-"));
}

bool MyApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	startupMode = IMODE_NONE;
    if(parser.Found(wxT("u"))) startupMode = IMODE_UPDATE;
    if(parser.Found(wxT("r"))) startupMode = IMODE_UNINSTALL;
    if(parser.Found(wxT("i"))) startupMode = IMODE_INSTALL;
    if(parser.Found(wxT("g"))) startupMode = IMODE_UPGRADE;

	// ignore auto-update function
    if(parser.Found(wxT("n"))) autoUpdateEnabled=false;
	

#if wxCHECK_VERSION(2, 9, 0)
	// special mode: put our hash into the clipboard
	if(parser.Found(wxT("d")))
	{
		if (wxTheClipboard->Open())
		{
			wxString txt = getExecutablePath() + wxT(" ") + conv(ConfigManager::getOwnHash()) + wxT(" ") + wxT(__TIME__) + wxT(" ") + wxT(__DATE__);
			wxTheClipboard->SetData(new wxTextDataObject(txt));
			wxTheClipboard->Flush();
			wxTheClipboard->Close();
		}
		exit(0);
	}
#endif
    return true;
}


wxString MyApp::getExecutablePath()
{
    static bool found = false;
    static wxString path;

    if (found)
        return path;
    else
    {
#ifdef __WXMSW__

        TCHAR buf[512];
        *buf = '\0';
        GetModuleFileName(NULL, buf, 511);
        path = buf;

#elif defined(__WXMAC__)

        ProcessInfoRec processinfo;
        ProcessSerialNumber procno ;
        FSSpec fsSpec;

        procno.highLongOfPSN = NULL ;
        procno.lowLongOfPSN = kCurrentProcess ;
        processinfo.processInfoLength = sizeof(ProcessInfoRec);
        processinfo.processName = NULL;
        processinfo.processAppSpec = &fsSpec;

        GetProcessInformation( &procno , &processinfo ) ;
        path = wxMacFSSpec2MacFilename(&fsSpec);
#else
        wxString argv0 = wxTheApp->argv[0];

        if (wxIsAbsolutePath(argv0))
            path = argv0;
        else
        {
            wxPathList pathlist;
            pathlist.AddEnvList(wxT("PATH"));
            path = pathlist.FindAbsoluteValidPath(argv0);
        }

        wxFileName filename(path);
        filename.Normalize();
        path = filename.GetFullPath();
#endif
        found = true;
        return path;
    }
}

// ----------------------------------------------------------------------------
// MyWizard
// ----------------------------------------------------------------------------
void path_descend(char* path)
{
	// WINDOWS ONLY
	char dirsep='\\';
	char* pt1=path;
	while(*pt1)
	{
		// normalize
		if(*pt1 == '/') *pt1 = dirsep;
		pt1++;
	}
	char* pt=path+strlen(path)-1;
	if (pt>=path && *pt==dirsep) pt--;
	while (pt>=path && *pt!=dirsep) pt--;
	if (pt>=path) *(pt+1)=0;
}

MyWizard::MyWizard(int startupMode, wxFrame *frame, bool _autoUpdateEnabled, bool useSizer)
        : wxWizard(frame,ID_WIZARD,_T("Rigs of Rods Installation Assistant"),
                   wxBitmap(licence_xpm),wxDefaultPosition,
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), startupMode(startupMode),
				   autoUpdateEnabled(_autoUpdateEnabled)
{
	// first thing to do: remove old installer file if possible
	if(boost::filesystem::exists("installer.exe.old"))
		boost::filesystem::remove("installer.exe.old");
	// now continue with normal startup

	new ConfigManager();
	CONFIG->setStartupMode(startupMode);

	// create log
	boost::filesystem::path iPath = boost::filesystem::path(conv(CONFIG->getInstallationPath()));
	boost::filesystem::path lPath = iPath / std::string("wizard.log");
	new InstallerLog(lPath);
	LOG("installer log created");

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#if 0 
	// this is disabled since we check for the newest installer anyways

	// detect wether the user should run the other installer
	if(!CONFIG->isFirstInstall())
	{
		// find the current working directory of this running app
		bool error=false;
		char program_path[1024]="";
		char install_path[1024]="";
		memset(program_path, 0, 1024);
		memset(install_path, 0, 1024);

		// get our path
		wxString myPath = MyApp::getExecutablePath();
		strncpy(program_path, myPath.c_str(), myPath.size());
		GetShortPathNameA(program_path, program_path, 512);
		path_descend(program_path);

		// find the installation path
		boost::filesystem::path installPath = boost::filesystem::path(conv(CONFIG->getInstallationPath()) + std::string("installer.exe"));
		strncpy(install_path, installPath.string().c_str(), installPath.string().size());
		GetShortPathNameA(install_path, install_path, 512);
		path_descend(install_path);

		// compare both
		if(!error && strcmp(program_path, install_path) && boost::filesystem::exists(installPath))
		{
			wxMessageBox(_T("Please try to use the installer that you downloaded during the installation:\n"+conv(installPath.string()+"\n\nPlease do not use the installer you just started, as it can get out of date.")), _T("Warning"), wxICON_WARNING | wxOK);
			//exit(1);
		}
	}
#endif //0
#endif // OGRE_PLATFORM

	// check if there is a newer installer available
	if(autoUpdateEnabled)
		CONFIG->checkForNewInstaller();

    PresentationPage *presentation = new PresentationPage(this);
	LicencePage *licence = new LicencePage(this);
	PathPage *path = new PathPage(this);
	StreamsPage *streams = new StreamsPage(this);
	StreamsContentPage *streamsContent = new StreamsContentPage(this);
    ActionPage *action = new ActionPage(this, licence, path, streams);
	DownloadPage *download = new DownloadPage(this);
	LastPage *last = new LastPage(this);
	streams->setPages(path, streamsContent);
	streamsContent->setPrevPage(streams);


	m_page1 = presentation;

	if (!CONFIG->isLicenceAccepted())
	{
		wxWizardPageSimple::Chain(presentation, licence);
		licence->SetNext(action);
		action->SetPrev(licence);
	}
	else
	{
		if(startupMode == IMODE_UPDATE)
		{
			presentation->SetNext(streams);
			streams->SetPrev(presentation);
			CONFIG->setAction(2); // 2 = update
		} else
		{
			presentation->SetNext(action);
			action->SetPrev(presentation);
		}
	}
	path->SetPrev(action);
	wxWizardPageSimple::Chain(path, streams);
	wxWizardPageSimple::Chain(streams, streamsContent);
	wxWizardPageSimple::Chain(streamsContent, download);
	wxWizardPageSimple::Chain(download, last);

    if ( useSizer )
    {
        // allow the wizard to size itself around the pages
        GetPageAreaSizer()->Add(presentation);
    }
}

void MyWizard::OnPageChanging(wxWizardEvent &event)
{
	wxWizardPage *wp=event.GetPage();
	EnterLeavePage *elp=dynamic_cast<EnterLeavePage*>(wp);
	if (elp)
	{
		if (event.GetDirection())
		{
			//forward
			bool b=elp->OnLeave(true);
			if (!b) {event.Veto();return;}
			wxWizardPage *nwp=wp->GetNext();
			EnterLeavePage *nelp=dynamic_cast<EnterLeavePage*>(nwp);
			if (nelp)
			{
				bool b=nelp->OnEnter(true);
				if (!b) event.Veto();
			}
		}
		else
		{
			//backward
			bool b=elp->OnLeave(false);
			if (!b) {event.Veto();return;}
			wxWizardPage *nwp=wp->GetPrev();
			EnterLeavePage *nelp=dynamic_cast<EnterLeavePage*>(nwp);
			if (nelp)
			{
				bool b=nelp->OnEnter(false);
				if (!b) event.Veto();
			}
		}
	}
}


// utils for the wizard
inline void setControlEnable(wxWizard *wiz, int id, bool enable)
{
	wxWindow *win = wiz->FindWindowById(id);
	if(win) win->Enable(enable);
}

// ----------------------------------------------------------------------------
// Wizard pages
// ----------------------------------------------------------------------------
//// PresentationPage
PresentationPage::PresentationPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
	m_bitmap = wxBitmap(welcome_xpm);
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *tst;
	//GetParent()->SetBackgroundColour(*wxWHITE);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Welcome to the online installer of Rigs of Rods\n")), 0, wxALL, 5);
	wxFont dfont=tst->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+4);
	tst->SetFont(dfont);
	tst->Wrap(TXTWRAP);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("This program will help you install or update Rigs of Rods on your computer.\n")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Before installing, make sure Rigs of Rods is not running, and that your internet connection is available.\n")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("If you are using a firewall, please allow this program to access the Internet.\n")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Click on Next to continue.\n")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);

	SetSizer(mainSizer);
	mainSizer->Fit(this);
}

//// LicencePage
LicencePage::LicencePage(wxWizard *parent) : wxWizardPageSimple(parent)
{
	m_bitmap = wxBitmap(licence_xpm);
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *tst;
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Licence\n")), 0, wxALL, 5);
	wxFont dfont=tst->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+4);
	tst->SetFont(dfont);
	tst->Wrap(TXTWRAP);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Please review the terms of the following licence before installing:\n")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);
	mainSizer->Add(new wxTextCtrl(this, wxID_ANY, _T("Rigs of Rods Copyright (C) 2008 Pierre-Michel Ricordel\n\n")
													_T("This program is a freeware which is offered to you at no cost.\n")
													_T("You can redistribute the installer pack as long as you do not modify it in any way.\n\n")
													_T("This program is distributed in the hope that it will entertain you, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n")
													_T("By using this program, you agree that in the case you create and publish a content for it (for example a vehicle), you accept that the author can include it in the official distribution, however you still retain the copyright of your work.")
													,wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY)
													, 1, wxALL|wxEXPAND , 5);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("If you agree to this licence, click Next. If you do not agree, you cannot install Rigs of Rods.\n")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);

	SetSizer(mainSizer);
	mainSizer->Fit(this);
}

//// ActionPage
ActionPage::ActionPage(wxWizard *parent, wxWizardPage* prev, wxWizardPage* fselect, wxWizardPage* download) : wxWizardPageSimple(parent)
{
	m_prev=prev;
	m_fselect=fselect;
	m_download=download;
	bool firstInstall = CONFIG->isFirstInstall();
	m_bitmap = wxBitmap(action_xpm);
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *tst;
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("What do you want to do?\n")), 0, wxALL, 5);
	wxFont dfont=tst->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+4);
	tst->SetFont(dfont);
	tst->Wrap(TXTWRAP);
	if(firstInstall)
	{
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("This is the first time you install Rigs of Rods, choose \"Install\" to download all the files of the game.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
	} else
	{
		wxString installPath = CONFIG->getInstallationPath();
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("The game is currently installed in:\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, installPath), 0, wxALL, 10);
		tst->Wrap(TXTWRAP);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+2);
		tst->SetFont(dfont);

		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Choose \"Update\" to update Rigs of Rods to the latest version with faster download time than downloading the entire game.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("If you choose \"Uninstall\", Rigs of Rods will be deleted from your hard drive.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
	}
	wxString choices[4];
	choices[0]=_T("Install");
	choices[1]=_T("Upgrade older Installation");
	choices[2]=_T("Update");
	choices[3]=_T("Uninstall");
	arb=new wxRadioBox(this, wxID_ANY, _T("Actions"), wxDefaultPosition, wxDefaultSize, 4, choices, 1, wxRA_SPECIFY_COLS);
	if (firstInstall)
	{
		arb->Enable(2, false);
		arb->Enable(3, false);
		arb->SetSelection(0);
		if(CONFIG->getStartupMode() == IMODE_UPGRADE)
			arb->SetSelection(1);
	}
	else
	{
		arb->Enable(0, false);
		arb->Enable(1, false);
		arb->SetSelection(2);
		if(CONFIG->getStartupMode() == IMODE_UNINSTALL)
			arb->SetSelection(3);
	}
	mainSizer->Add(arb, 0, wxALL, 5);

	SetSizer(mainSizer);
	mainSizer->Fit(this);

}

void ActionPage::SetPrev(wxWizardPage *prev)
{
	m_prev = prev;
}

wxWizardPage *ActionPage::GetPrev() const
{
	return m_prev;
}

wxWizardPage *ActionPage::GetNext() const
{
	if (arb->GetSelection()==0)
	{
		return m_fselect;
	} else if (arb->GetSelection()==1)
	{
		return m_fselect;
	} else if (arb->GetSelection()==3)
	{
		int res = wxMessageBox(_T("Do you really want to uninstall Rigs of Rods?"), _T("Uninstall?"), wxICON_QUESTION | wxYES_NO);
		if(res == wxYES)
		{
			res = wxMessageBox(_T("Do you want to remove all Rigs of Rods content in the User Folder (all downloaded mods, etc)?"), _T("Uninstall User Content?"), wxICON_QUESTION | wxYES_NO);
			res = CONFIG->uninstall((res == wxYES));
			if(!res)
			{
				wxMessageBox(_T("Rigs of Rods was uninstalled successfully. Thanks for using!"), _T("Uninstalled!"), wxICON_INFORMATION | wxOK);
				exit(0);
			}
		}
		wxMessageBox(_T("Uninstall aborted!"), _T("uninstall?"), wxICON_ERROR | wxOK);
		exit(0);

		// we should never be here
		return m_fselect;
	} else
	{
		return m_download;
	}
}

//output validation
bool ActionPage::OnLeave(bool forward)
{
	if (forward && arb->GetSelection()==1)
		wxMessageBox(_T("Please select the old installation directory."), _T("Upgrade installation"), wxICON_INFORMATION | wxOK);

	if (forward) CONFIG->setAction(arb->GetSelection());
	return true;
}

//// PathPage
PathPage::PathPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
	m_bitmap = wxBitmap(dest_xpm);
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *tst;
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Source and Destination\n")), 0, wxALL, 5);
	wxFont dfont=tst->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+4);
	tst->SetFont(dfont);
	tst->Wrap(TXTWRAP);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Download from:")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);

	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("\nChoose the installation directory:")), 0, wxTOP|wxLEFT|wxRIGHT, 5);
	tst->Wrap(TXTWRAP);

	wxString path = wxT("");
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxString dir;
	if (SHGetSpecialFolderPath(0, wxStringBuffer(dir, MAX_PATH), CSIDL_PROGRAM_FILES, FALSE))
	{
		path = dir + wxT("\\Rigs of Rods");
	}
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	char homedir[255] = "";
	strncpy(homedir, getenv ("HOME"), 250);
	char tmp[255]="";
	snprintf(tmp, 255, "%s/rigsofrods/", homedir);
	path = conv(tmp);
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	path = _T("/opt/games/rigsofrods/");
#endif // OGRE_PLATFORM
	mainSizer->Add(sel=new wxTextCtrl(this, wxID_ANY, path, wxDefaultPosition,wxDefaultSize, 0) , 0, wxALL|wxEXPAND , 5);
	mainSizer->Add(brobut=new wxButton(this, ID_BROWSE, _T("Browse...")), 0, wxBOTTOM|wxLEFT|wxRIGHT , 5);

	//mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("You should have at least 100MB of free disk space on this drive to install Rigs of Rods.\n")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);

	dirdial=new wxDirDialog(this, _T("Choose a directory"), sel->GetValue());

	SetSizer(mainSizer);
	mainSizer->Fit(this);
}

void PathPage::OnBrowse(wxCommandEvent&WXUNUSED(evt))
{
	dirdial->SetPath(sel->GetValue());
	int res=dirdial->ShowModal();
	if (res==wxID_OK) sel->SetValue(dirdial->GetPath());
}

//output validation
bool PathPage::OnLeave(bool forward)
{
	if (forward)
	{
		wxString path=sel->GetValue();
		if (!wxFileName(path).IsAbsolute()) //relative paths are dangerous!
		{
			wxMessageBox(_T("This path is invalid"), _T("Invalid path"), wxICON_ERROR | wxOK, this);
			return false;
		}
		if (!wxDir::Exists(path))
		{
			if (!wxFileName::Mkdir(path))
			{
				wxMessageBox(_T("This path could not be created"), _T("Invalid path"), wxICON_ERROR | wxOK, this);
				return false;
			}
		}
		CONFIG->setInstallPath(path);
	}
	return true;
}


//// StreamsPage
StreamsPage::StreamsPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
	streamset=false;
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *tst;
	m_bitmap = wxBitmap(streams_xpm);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Streams selection\n")), 0, wxALL, 0);
	wxFont dfont=tst->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+4);
	tst->SetFont(dfont);
	tst->Wrap(TXTWRAP);

	wxString choices[4];
	choices[0]=_T("Stable");
	choices[1]=_T("Latest");
	betaOption=new wxRadioBox(this, wxID_ANY, _T("Version"), wxDefaultPosition, wxDefaultSize, 2, choices, 1, wxRA_SPECIFY_COLS);
	mainSizer->Add(betaOption);
	bool use_stable = CONFIG->getPersistentConfig(wxT("installer.use_stable")) == wxT("yes");
	betaOption->SetSelection(1);
	if(use_stable)
		betaOption->SetSelection(0);

	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Choose which binary pack you want to download:\n")), 0, wxALL, 0);
	tst->Wrap(TXTWRAP);

	mainSizer->Add(scrw=new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL), 1, wxGROW, 0);
	scrwsz=new wxBoxSizer(wxVERTICAL);
	scrw->SetSizer(scrwsz);
	//scrw->SetBackgroundColour(*wxWHITE);
	scrw->SetScrollbars(0, STREL_HEIGHT+3, 100, 30);

	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Click Next to select the content.")), 0, wxALL, 0);
	tst->Wrap(TXTWRAP);

	SetSizer(mainSizer);
	scrwsz->Fit(scrw);
	mainSizer->Fit(this);

}
void StreamsPage::OnBetaChange(wxCommandEvent &evt)
{
	if(betaOption->GetSelection() == 0)
	{
		// stable
		CONFIG->setPersistentConfig(wxT("installer.use_stable"), wxT("yes"));
		OnEnter(true);
	} else
	{
		// beta
		CONFIG->setPersistentConfig(wxT("installer.use_stable"), wxT("no"));
		OnEnter(true);
	}
}

bool StreamsPage::OnEnter(bool forward)
{
	int res1 = CONFIG->getCurrentVersionInfo();
	if(res1)
	{
		wxMessageBox(_T("The program could not connect to the central Streams server in order to receive the latest version information.\nThe service may be temporarily unavailable, or you have no network connection."), _T("Network error"), wxICON_ERROR | wxOK, this);
		exit(1);
	}

	int res = CONFIG->getOnlineStreams();
	if(res)
	{
		wxMessageBox(_T("The program could not connect to the central Streams server.\nThe service may be temporarily unavailable, or you have no network connection."), _T("Network error"), wxICON_ERROR | wxOK, this);
		exit(1);
	}

	wxSizerItemList::compatibility_iterator node = scrwsz->GetChildren().GetFirst();
	while (node)
	{
		wxSizerItem *item = node->GetData();
		item->Show(false);
		node = node->GetNext();
	}
	//scrwsz->Clear();
	//add the streams
	CONFIG->loadStreamSubscription();
	std::vector < stream_desc_t > *streams = CONFIG->getStreamset();
	if(!streams->size())
	{
		wxMessageDialog *dlg = new wxMessageDialog(this, wxT("Error while downloading the streams index. Please retry later."), wxT("Error"),wxOK|wxICON_ERROR);
		dlg->ShowModal();
		exit(1);
	}
	bool use_stable = betaOption->GetSelection() == 0; // 0 == stable
	int counter=0;
	for(std::vector < stream_desc_t >::iterator it=streams->begin(); it!=streams->end(); it++)
	{
		if(it->hidden) continue; // hide hidden entries ;)
		bool disable = false;
		if(!it->binary && !it->resource) disable=true; // just show binaries and resources
		if((!it->stable || it->beta )&& use_stable) disable=true; // hide non-stable in stable mode
		if((it->stable || !it->beta )&& !use_stable) disable=true; // hide stable in beta mode
		if(disable)	continue;

		if(it->forcecheck)
			it->checked = true;

		counter++;
		wxStrel *wst=new wxStrel(scrw, &(*it));
		wxSizerItem *si = scrwsz->Add(wst, 0, wxALL|wxEXPAND,0);
		si->SetUserData((wxObject *)wst);
	}
	scrwsz->Fit(scrw);
	if(!counter && use_stable)
		wxMessageBox(wxT("No stable version currently available via the installer. Please select the latest option or download the latest stable version from \nhttp://sourceforge.net/projects/rigsofrods/"), wxT("No stable stream"));

	return true;
}

bool StreamsPage::OnLeave(bool forward)
{
	if (forward)
	{
		//store the user settings
		wxSizerItemList::compatibility_iterator node = scrwsz->GetChildren().GetFirst();
		while (node)
		{
			wxSizerItem *item = node->GetData();
			if(item->IsShown())
			{
				wxStrel *wst = (wxStrel *)item->GetUserData();
				if(wst)
					CONFIG->setStreamSelection(wst->getDesc(), wst->getSelection());
			} else
			{
				wxStrel *wst = (wxStrel *)item->GetUserData();
				if(wst)
					CONFIG->setStreamSelection(wst->getDesc(), false);
			}
			node = node->GetNext();
		}
	}

	return true;
}

wxWizardPage *StreamsPage::GetPrev() const
{
	if(CONFIG->getAction() == 0 || CONFIG->getAction() == 1)
		return fpath;
	return faction;
}

void StreamsPage::setPages(wxWizardPage* _fpath, wxWizardPage* _faction)
{
	fpath=_fpath;
	faction=_faction;
}

//// StreamsContentPage
StreamsContentPage::StreamsContentPage(wxWizard *parent) : wxWizardPageSimple(parent)
{
	streamset=false;
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *tst;
	m_bitmap = wxBitmap(streams_xpm);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Content Streams selection\n")), 0, wxALL, 0);
	wxFont dfont=tst->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+4);
	tst->SetFont(dfont);
	tst->Wrap(TXTWRAP);

	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Choose which content packs you want to download:\n")), 0, wxALL, 0);
	tst->Wrap(TXTWRAP);

	mainSizer->Add(scrw=new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL), 1, wxGROW, 0);
	scrwsz=new wxBoxSizer(wxVERTICAL);
	scrw->SetSizer(scrwsz);
	//scrw->SetBackgroundColour(*wxWHITE);
	scrw->SetScrollbars(0, STREL_HEIGHT+3, 100, 30);

	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Click Next to begin the download.")), 0, wxALL, 0);
	tst->Wrap(TXTWRAP);

	SetSizer(mainSizer);
	//scrwsz->Fit(scrw);
	mainSizer->Fit(this);

}

bool StreamsContentPage::OnEnter(bool forward)
{
	if (streamset) return true; //reloading streams do not work for the moment (GUI problem)
	int res = CONFIG->getOnlineStreams();
	if(res)
	{
		wxMessageBox(_T("The program could not connect to the central Streams server.\nThe service may be temporarily unavailable, or you have no network connection."), _T("Network error"), wxICON_ERROR | wxOK, this);
	}
	scrwsz->Clear(true);
	//add the streams
	std::vector < stream_desc_t > *streams = CONFIG->getStreamset();
	if(!streams->size())
	{
		wxMessageDialog *dlg = new wxMessageDialog(this, wxT("Error while downloading the streams index. Please retry later."), wxT("Error"),wxOK|wxICON_ERROR);
		dlg->ShowModal();
		exit(1);
	}
	bool use_stable = CONFIG->getPersistentConfig(wxT("installer.use_stable")) == wxT("yes");
	for(std::vector < stream_desc_t >::iterator it=streams->begin(); it!=streams->end(); it++)
	{
		if(it->hidden || !it->content) continue; // hide hidden entries and non-content things
		bool disable=false;
		if(it->beta && use_stable) disable=true; // hide non-stable in stable mode
		if(disable) continue;

		if(it->forcecheck)
			it->checked = true;

		wxStrel *wst=new wxStrel(scrw, &(*it));
		wxSizerItem *si = scrwsz->Add(wst, 0, wxALL|wxEXPAND,0);
		si->SetUserData((wxObject *)wst);
	}
	scrwsz->Fit(scrw);
	streamset=true;
	return true;
}

bool StreamsContentPage::OnLeave(bool forward)
{
	if (forward)
	{
		//store the user settings
		wxSizerItemList::compatibility_iterator node = scrwsz->GetChildren().GetFirst();
		while (node)
		{
			wxSizerItem *item = node->GetData();
			wxStrel *wst = (wxStrel *)item->GetUserData();
			if(!wst) continue;
			// save the selection in the registry for the next time.
			CONFIG->setStreamSelection(wst->getDesc(), wst->getSelection());
			node = node->GetNext();
		}
		// then load the full set from the storage again and use it!
		CONFIG->loadStreamSubscription();
		CONFIG->streamSubscriptionDebug();
	}
	return true;
}

wxWizardPage *StreamsContentPage::GetPrev() const
{
	return fpage;
}

void StreamsContentPage::setPrevPage(wxWizardPage* _fpage)
{
	fpage=_fpage;
}


//// DownloadPage
DownloadPage::DownloadPage(wxWizard *parent) : wxWizardPageSimple(parent), wizard(parent)
{
	threadStarted=false;
	isDone=false;
	m_bitmap = wxBitmap(download_xpm);
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	
    mainSizer->Add(txtTitle=new wxStaticText(this, wxID_ANY, _T("Downloading")), 0, wxALL, 5);
	wxFont dfont=txtTitle->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+3);
	txtTitle->SetFont(dfont);
	txtTitle->Wrap(TXTWRAP);
     

	// status text and progress bar
	statusList = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(20, 160));
	mainSizer->Add(statusList, 0, wxALL|wxEXPAND, 0);
	mainSizer->Add(10, 10);
	progress=new wxGauge(this, wxID_ANY, 1000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_SMOOTH);
	mainSizer->Add(progress, 0, wxALL|wxEXPAND, 0);
	progress->Pulse();

 	// now the information thingy
	wxGridSizer *wxg = new wxGridSizer(3, 2, 2, 5);

    wxStaticText *txt;
	/*
	// removed for now
	// download time
	txt = new wxStaticText(this, wxID_ANY, _T("Download time: "));
	wxg->Add(txt, 0, wxALL|wxEXPAND, 0);
	txt_dltime = new wxStaticText(this, wxID_ANY, _T("n/a"));
	wxg->Add(txt_dltime, 0, wxALL|wxEXPAND, 0);
	*/

	// download time
	txt = new wxStaticText(this, wxID_ANY, _T("Remaining time: "));
	wxg->Add(txt, 0, wxALL|wxEXPAND, 0);
	txt_remaintime = new wxStaticText(this, wxID_ANY, _T("n/a"));
	wxg->Add(txt_remaintime, 0, wxALL|wxEXPAND, 0);

	// traffic
	txt = new wxStaticText(this, wxID_ANY, _T("Data transferred: "));
	wxg->Add(txt, 0, wxALL|wxEXPAND, 0);
	txt_traf = new wxStaticText(this, wxID_ANY, _T("n/a"));
	wxg->Add(txt_traf, 0, wxALL|wxEXPAND, 0);

	// speed
	txt = new wxStaticText(this, wxID_ANY, _T("Average Download Speed: "));
	wxg->Add(txt, 0, wxALL|wxEXPAND, 0);
	txt_speed = new wxStaticText(this, wxID_ANY, _T("n/a"));
	wxg->Add(txt_speed, 0, wxALL|wxEXPAND, 0);

	// Local Path
	txt = new wxStaticText(this, wxID_ANY, _T("Sync Path: "));
	wxg->Add(txt, 0, wxALL|wxEXPAND, 0);
	txt_localpath = new wxStaticText(this, wxID_ANY, _T("n/a"));
	wxg->Add(txt_localpath, 0, wxALL|wxEXPAND, 0);

	// Server used
	txt = new wxStaticText(this, wxID_ANY, _T("Server used: "));
	wxg->Add(txt, 0, wxALL|wxEXPAND, 0);
	txt_server = new wxStaticText(this, wxID_ANY, _T("only main server"));
	wxg->Add(txt_server, 0, wxALL|wxEXPAND, 0);

	// Server used
	txt = new wxStaticText(this, wxID_ANY, _T("Download Jobs: "));
	wxg->Add(txt, 0, wxALL|wxEXPAND, 0);
	txt_concurr = new wxStaticText(this, wxID_ANY, wxT("none"));
	wxg->Add(txt_concurr, 0, wxALL|wxEXPAND, 0);

	// version information
	txt = new wxStaticText(this, wxID_ANY, _T("Version Info: "));
	wxg->Add(txt, 0, wxALL|wxEXPAND, 0);
	hlink = new wxHyperlinkCtrl(this, wxID_ANY, wxT("none"), wxT(""));
	wxg->Add(hlink, 0, wxALL|wxEXPAND, 0);


	// grid end
	mainSizer->Add(wxg, 0, wxALL|wxEXPAND, 5);


	mainSizer->Add(10, 10);
    // important to be able to load web URLs
    wxFileSystem::AddHandler( new wxInternetFSHandler );
    htmlinfo = new HtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(50, 50), wxBORDER_NONE);
	mainSizer->Add(htmlinfo, 0, wxALL|wxEXPAND);
    htmlinfo->SetPage(_("."));
    timer = new wxTimer(this, ID_TIMER);
    timer->Start(10000);
    
	// FINISHED text
	txtFinish = new wxStaticText(this, wxID_ANY, _T("Finished downloading, please continue by pressing next."));
    wxFont dfont2=txtFinish->GetFont();
	dfont2.SetWeight(wxFONTWEIGHT_BOLD);
	dfont2.SetPointSize(dfont2.GetPointSize()+2);
	txtFinish->SetFont(dfont2);
	//txtFinish->Wrap(TXTWRAP);
	mainSizer->Add(txtFinish, 0, wxALL|wxEXPAND, 0);

	SetSizer(mainSizer);
	mainSizer->Fit(this);
}


void DownloadPage::startThread()
{
	if(threadStarted) return;
	threadStarted=true;
	// XXX ENABLE DEBUG
	bool debugEnabled = true;
	// XXX
	m_pThread = new WsyncThread(this, CONFIG->getInstallationPath(), *(CONFIG->getStreamset()));
	if ( m_pThread->Create() != wxTHREAD_NO_ERROR )
	{
		wxLogError(wxT("Can't create the thread!"));
		delete m_pThread;
		m_pThread = NULL;
	}
	else
	{
		if (m_pThread->Run() != wxTHREAD_NO_ERROR )
		{
			wxLogError(wxT("Can't create the thread!"));
			delete m_pThread;
			m_pThread = NULL;
		}

		// after the call to wxThread::Run(), the m_pThread pointer is "unsafe":
		// at any moment the thread may cease to exist (because it completes its work).
		// To avoid dangling pointers OnThreadExit() will set m_pThread
		// to NULL when the thread dies.
	}
}

bool DownloadPage::OnEnter(bool forward)
{
	txtFinish->Hide();
	//disable forward and backward buttons
	setControlEnable(wizard, wxID_FORWARD, false);
	setControlEnable(wizard, wxID_BACKWARD, false);

	// fill in version information
	std::string fromVersion = CONFIG->readVersionInfo();
	std::string toVersion   = CONFIG->getOnlineVersion();
	std::string versionText = std::string("updating from ") + fromVersion + std::string(" to ") + toVersion;
	if(fromVersion == "unkown")
		versionText = toVersion;
	if(fromVersion == toVersion)
		versionText = toVersion;
	std::string versionURL  = std::string(CHANGELOGURL) + toVersion;
	hlink->SetLabel(conv(versionText));
	hlink->SetURL(conv(versionURL));


	std::vector < stream_desc_t > *streams = CONFIG->getStreamset();
	if(!streams->size())
	{
		// TODO: handle this case, go back?
		return false;
	}

    htmlinfo->LoadPage(wxT("http://api.rigsofrods.com/didyouknow/"));

    startThread();
	return true;
}

bool DownloadPage::OnLeave(bool forward)
{
	if(isDone && !forward) return false; //when done, only allow to go forward
	return isDone;
}

void DownloadPage::OnStatusUpdate(MyStatusEvent &ev)
{
	switch(ev.GetId())
	{
	case MSE_STARTING:
	case MSE_UPDATE_TEXT:
		statusList->Append(ev.GetString());
		statusList->SetSelection(statusList->GetCount()-1);
		break;
	case MSE_UPDATE_PROGRESS:
		progress->SetValue(ev.GetProgress() * 1000.0f);
		break;
	case MSE_DOWNLOAD_PROGRESS:
	{
		wxString str = ev.GetString();
		if(ev.GetProgress() > 0)
		{
			for(int i=statusList->GetCount()-1;i>=0;--i)
			{
				wxString str_comp = statusList->GetString(i).SubString(0, str.size()-1);
				if(str_comp == str)
				{
					statusList->SetString(i, str + wxString::Format(wxT(" (%3.1f%% downloaded)"), ev.GetProgress() * 100.0f));
					break;
				}
			}
		}
	}
		break;
	case MSE_DOWNLOAD_DONE:
	{
		wxString str = ev.GetString();
		for(int i=statusList->GetCount()-1;i>=0;--i)
		{
			wxString str_comp = statusList->GetString(i).SubString(0, str.size()-1);
			if(str_comp == str)
			{
				statusList->SetString(i, str + wxT(" (DONE)"));
				break;
			}
		}
	}
		break;
	case MSE_ERROR:
		statusList->Append(_("error: ") + ev.GetString());
		statusList->SetSelection(statusList->GetCount()-1);
		progress->SetValue(1000);
		wxMessageBox(ev.GetString(), _("Error"), wxICON_ERROR | wxOK, this);
		break;
	case MSE_UPDATE_TIME:
		//txt_dltime->SetLabel(ev.GetString());
		break;
	case MSE_UPDATE_TIME_LEFT:
		txt_remaintime->SetLabel(ev.GetString());
		break;
	case MSE_UPDATE_SPEED:
		txt_speed->SetLabel(ev.GetString());
		break;
	case MSE_UPDATE_TRAFFIC:
		txt_traf->SetLabel(ev.GetString());
		break;
	case MSE_UPDATE_PATH:
		txt_localpath->SetLabel(ev.GetString());
		break;
	case MSE_UPDATE_SERVER:
		txt_server->SetLabel(ev.GetString());
		break;
	case MSE_UPDATE_CONCURR:
		txt_concurr->SetLabel(ev.GetString());
		break;
	case MSE_UPDATE_TITLE:
		txtTitle->SetLabel(ev.GetString());
		break;
	case MSE_DONE:
		// normal end
		statusList->Append(wxT("Finished downloading."));
		statusList->SetSelection(statusList->GetCount()-1);
		progress->SetValue(1000);
		txt_remaintime->SetLabel(wxT("finished!"));
		isDone=true;
		CONFIG->writeVersionInfo(); // write the version to the file, since we updated
		// enableforward button
		txtFinish->Show();
        htmlinfo->Hide();
		setControlEnable(wizard, wxID_FORWARD, true);
		break;
	}
}
void DownloadPage::OnTimer(wxTimerEvent& event)
{
    if(htmlinfo)
      htmlinfo->LoadPage(wxT("http://api.rigsofrods.com/didyouknow/"));
}


//// LastPage
LastPage::LastPage(wxWizard *parent) : wxWizardPageSimple(parent), wizard(parent)
{
	m_bitmap = wxBitmap(finished_xpm);
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText *tst;
	//GetParent()->SetBackgroundColour(*wxWHITE);
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Finished\n")), 0, wxALL, 5);
	wxFont dfont=tst->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+4);
	tst->SetFont(dfont);
	tst->Wrap(TXTWRAP);
	bool firstInstall = CONFIG->isFirstInstall();

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Thank you for downloading Rigs of Rods.\nWhat do you want to do now?")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);


	chk_runtime = new wxCheckBox(this, wxID_ANY, _T("Install required runtime libraries now"));
	mainSizer->Add(chk_runtime, 0, wxALL|wxALIGN_LEFT, 5);
	chk_runtime->SetValue(firstInstall);
	if(firstInstall) chk_runtime->Disable();

	chk_upgrade_configs = new wxCheckBox(this, wxID_ANY, _T("Update User Configurations (important)"));
	mainSizer->Add(chk_upgrade_configs, 0, wxALL|wxALIGN_LEFT, 5);
	if(firstInstall)
	{
		chk_upgrade_configs->SetValue(false);
		chk_upgrade_configs->Disable();
	} else
	{
		// always enable this box, thus force the users to update always
		chk_upgrade_configs->SetValue(true);
		chk_upgrade_configs->Enable();
	}

	chk_desktop = new wxCheckBox(this, wxID_ANY, _T("Create Desktop shortcuts"));
	mainSizer->Add(chk_desktop, 0, wxALL|wxALIGN_LEFT, 5);

	chk_startmenu = new wxCheckBox(this, wxID_ANY, _T("Create Start menu shortcuts"));
	mainSizer->Add(chk_startmenu, 0, wxALL|wxALIGN_LEFT, 5);

	chk_viewmanual = new wxCheckBox(this, wxID_ANY, _T("View manual"));
	mainSizer->Add(chk_viewmanual, 0, wxALL|wxALIGN_LEFT, 5);

	chk_configurator = new wxCheckBox(this, wxID_ANY, _T("Run the Configurator"));
	mainSizer->Add(chk_configurator, 0, wxALL|wxALIGN_LEFT, 5);

	chk_changelog = new wxCheckBox(this, wxID_ANY, _T("View the Changelog"));
	mainSizer->Add(chk_changelog, 0, wxALL|wxALIGN_LEFT, 5);

	chk_filetypes = new wxCheckBox(this, wxID_ANY, _T("Associate file types"));
	mainSizer->Add(chk_filetypes, 0, wxALL|wxALIGN_LEFT, 5);

#else
	// TODO: add linux options
	mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Thank you for downloading Rigs of Rods.\n")), 0, wxALL, 5);
	tst->Wrap(TXTWRAP);
#endif // OGRE_PLATFORM
	SetSizer(mainSizer);
	mainSizer->Fit(this);
}

bool LastPage::OnEnter(bool forward)
{
	setControlEnable(wizard, wxID_BACKWARD, false);
	setControlEnable(wizard, wxID_CANCEL, false);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

	bool firstInstall = CONFIG->isFirstInstall();
	chk_desktop->SetValue(true);
	if(CONFIG->getPersistentConfig(wxT("installer.create_desktop_shortcuts")) == wxT("no"))
		chk_desktop->SetValue(false);

	chk_startmenu->SetValue(true);
	if(CONFIG->getPersistentConfig(wxT("installer.create_start_menu_shortcuts")) == wxT("no"))
		chk_startmenu->SetValue(false);

	chk_viewmanual->SetValue(firstInstall);

	chk_configurator->SetValue(true);
	if(CONFIG->getPersistentConfig(wxT("installer.run_configurator")) == wxT("no"))
		chk_configurator->SetValue(false);

	chk_changelog->SetValue(true);
	if(CONFIG->getPersistentConfig(wxT("installer.view_changelog")) == wxT("no"))
		chk_changelog->SetValue(false);

	chk_filetypes->SetValue(true);
	if(CONFIG->getPersistentConfig(wxT("installer.associate_filetypes")) == wxT("no"))
		chk_filetypes->SetValue(false);

	
#endif // OGRE_PLATFORM
	return true;
}

bool LastPage::OnLeave(bool forward)
{
	if(!forward) return false;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// do last things
	CONFIG->setPersistentConfig(wxT("installer.create_desktop_shortcuts"), chk_desktop->IsChecked()?wxT("yes"):wxT("no"));
	CONFIG->setPersistentConfig(wxT("installer.create_start_menu_shortcuts"), chk_startmenu->IsChecked()?wxT("yes"):wxT("no"));
	CONFIG->setPersistentConfig(wxT("installer.run_configurator"), chk_configurator->IsChecked()?wxT("yes"):wxT("no"));
	CONFIG->setPersistentConfig(wxT("installer.view_changelog"), chk_changelog->IsChecked()?wxT("yes"):wxT("no"));
	CONFIG->setPersistentConfig(wxT("installer.associate_filetypes"), chk_filetypes->IsChecked()?wxT("yes"):wxT("no"));

	
	if(chk_desktop->IsChecked() || chk_startmenu->IsChecked())
		CONFIG->createProgramLinks(chk_desktop->IsChecked(), chk_startmenu->IsChecked());

	if(chk_runtime->IsChecked())
		CONFIG->installRuntime();

	if(chk_upgrade_configs->IsChecked())
		CONFIG->updateUserConfigs();

	if(chk_configurator->IsChecked())
		CONFIG->startConfigurator();

	if(chk_viewmanual->IsChecked())
		CONFIG->viewManual();

	if(chk_changelog->IsChecked())
		CONFIG->viewChangelog();

	if(chk_filetypes->IsChecked())
		CONFIG->associateFileTypes();

#endif // OGRE_PLATFORM
	return true;
}




