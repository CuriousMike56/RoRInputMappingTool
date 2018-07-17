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
#include "wx/wizard.h"
#include "cevent.h"
#include "wthread.h"

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
#include <wx/hyperlink.h>

#define TXTWRAP 400

enum {ID_BROWSE, ID_WIZARD, ID_TIMER};
enum {IMODE_NONE=0, IMODE_UPDATE, IMODE_INSTALL, IMODE_UNINSTALL, IMODE_UPGRADE};

class MyApp : public wxApp
{
public:
	// override base class virtuals
	virtual bool OnInit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
	static wxString getExecutablePath();
protected:
	int startupMode;
	bool autoUpdateEnabled;

};

class HtmlWindow;

// ----------------------------------------------------------------------------
// our wizard
// ----------------------------------------------------------------------------

class MyWizard : public wxWizard
{
public:
	MyWizard(int startupMode, wxFrame *frame, bool autoUpdateEnabled, bool useSizer = true);
	wxWizardPage *GetFirstPage() const { return m_page1; }
	void OnPageChanging(wxWizardEvent& event);

private:
	wxWizardPageSimple *m_page1;
	int startupMode;
	bool autoUpdateEnabled;
	DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// Pages
// ----------------------------------------------------------------------------

class EnterLeavePage
{
public:
	virtual bool OnEnter(bool forward) {return true;}
	virtual bool OnLeave(bool forward) {return true;}
};

class PresentationPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
	PresentationPage(wxWizard *parent);
};

class LicencePage : public wxWizardPageSimple, public EnterLeavePage
{
public:
	LicencePage(wxWizard *parent);
};

class ActionPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
	ActionPage(wxWizard *parent, wxWizardPage* prev, wxWizardPage* fselect, wxWizardPage* download);
	bool OnLeave(bool forward);
	void SetPrev(wxWizardPage *prev);
	wxWizardPage *GetPrev() const;
	wxWizardPage *GetNext() const;
private:
	wxRadioBox* arb;
	wxWizardPage *m_prev, *m_fselect, *m_download;
};

class PathPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
	PathPage(wxWizard *parent);
	void OnBrowse(wxCommandEvent&WXUNUSED(evt));
	bool OnLeave(bool forward);
private:
	wxTextCtrl* sel;
	wxChoice* src;
	wxButton* brobut;
	wxDirDialog *dirdial;
	DECLARE_EVENT_TABLE()
};

class StreamsPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
	StreamsPage(wxWizard *parent);
	bool OnEnter(bool forward);
	bool OnLeave(bool forward);
	virtual wxWizardPage *GetPrev() const;
	void setPages(wxWizardPage* _fpath, wxWizardPage* _faction);
private:
	void OnBetaChange(wxCommandEvent &evt);
	wxScrolledWindow *scrw;
	wxSizer *scrwsz;
	wxRadioBox* betaOption;
	wxWizardPage* fpath, *faction;
	bool streamset;
	DECLARE_EVENT_TABLE()
};

class StreamsContentPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
	StreamsContentPage(wxWizard *parent);
	bool OnEnter(bool forward);
	bool OnLeave(bool forward);
	virtual wxWizardPage *GetPrev() const;
	void setPrevPage(wxWizardPage* fpage);
private:
	wxScrolledWindow *scrw;
	wxSizer *scrwsz;
	wxWizardPage* fpage;
	bool streamset;
};

class DownloadPage : public wxWizardPageSimple, public EnterLeavePage
{
	friend class WsyncThread;
public:
	DownloadPage(wxWizard *parent);
	void startThread();
	bool OnEnter(bool forward);
	bool OnLeave(bool forward);
private:
	wxStaticText *txt_dltime, *txt_speed, *txt_traf, *txt_localpath, *txt_server, *txt_remaintime, *txt_concurr, *txt_updver;
	wxHyperlinkCtrl *hlink;
	wxStaticText *txtFinish;
	wxStaticText *txtTitle;
	wxListBox *statusList;
	wxGauge *progress;
        HtmlWindow *htmlinfo;
	bool threadStarted;
	bool isDone;
	wxWizard *wizard;
        wxTimer *timer;

	WsyncThread *m_pThread;

	void OnStatusUpdate(MyStatusEvent &ev);
        void OnTimer(wxTimerEvent& event);
	DECLARE_EVENT_TABLE()
};

class LastPage : public wxWizardPageSimple, public EnterLeavePage
{
protected:
	wxWizard *wizard;
	wxCheckBox *chk_runtime, *chk_configurator, *chk_desktop, *chk_startmenu, *chk_viewmanual, *chk_upgrade_configs, *chk_changelog, *chk_filetypes;
public:
	LastPage(wxWizard *parent);
	bool OnEnter(bool forward);
	bool OnLeave(bool forward);
};
