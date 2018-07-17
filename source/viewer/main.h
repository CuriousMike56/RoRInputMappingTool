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

#ifndef MESHVIEWERMAIN_H__
#define MESHVIEWERMAIN_H__

#include <vector>
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/artprov.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/statusbr.h>
#include <wx/colordlg.h>
#include <wx/wrapsizer.h>
#include <wx/msgdlg.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/vscroll.h>
#include <wx/rawbmp.h>
#include <wx/dir.h>
#include <wx/spinctrl.h>
#include <wx/cmdline.h>

#include "RoRViewer.h"

// RoR includes
#include "Settings.h"

using namespace std;

class PanelMeshProp;
class PanelMeshTree;

enum
{
	ID_VIEWPORT,

	ID_TOOL_MODE_TEXTURE,
	ID_TOOL_MODE_WIREFRAME,
	ID_TOOL_MODE_POINT,

};

class Window3DHandler
{
public:
	virtual void OnLeftUp(int x, int y) = 0;
	virtual void OnMouseMove(wxMouseEvent& e) = 0;
	virtual void OnKeyDown(bool ctrl, bool alt, bool shift, int key) = 0;
};

class Window3D : public wxPanel
{
public:
	Window3D(wxWindow* parent, wxWindowID id);

	void OnSize(wxSizeEvent& e);
	void OnLeftUp(wxMouseEvent& e);
	void OnMouseMove(wxMouseEvent& e);
	void OnKeyDown(wxKeyEvent& event);
	Window3DHandler* handler;
	Ogre::RenderWindow* wnd;
	Ogre::Camera* cam;

private:
	 DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(Window3D, wxPanel)
	EVT_LEFT_UP(Window3D::OnLeftUp)
	EVT_KEY_DOWN(Window3D::OnKeyDown)
	EVT_MOTION(Window3D::OnMouseMove)
	EVT_MOUSEWHEEL(Window3D::OnMouseMove)
	EVT_SIZE(Window3D::OnSize)
	// maybe use EVT_MOUSE_EVENTS for all events
END_EVENT_TABLE()

class RoRViewerFrame : public wxFrame, Window3DHandler
{
public:
	RoRViewerFrame(wxString title);
	~RoRViewerFrame();

	void updatePanelData(void);

	void InitializeAUI(void);
	void DeinitializeAUI(void);

	bool InitializeRoRViewer(wxString meshPath);
	void DenitializeRoRViewer(void);

	void OnIdle(wxIdleEvent& event);

	void OnViewToolClick(wxCommandEvent& e);

	// Window3DHandler
	void OnLeftUp(int x, int y);
	void OnMouseMove(wxMouseEvent& e);
	void OnKeyDown(bool ctrl, bool alt, bool shift, int key);
private:
	int msx, msy, msw;

	// AUI elements
	wxAuiManager* aui_mgr;

	Window3D* panel_viewport;
	wxAuiPaneInfo* pane_viewport;

	PanelMeshProp* panel_meshprop;
	wxAuiPaneInfo* pane_meshprop;

	PanelMeshTree* panel_meshtree;
	wxAuiPaneInfo* pane_meshtree;

	RoRViewer* viewer;
    DECLARE_EVENT_TABLE()
};

class RoRViewerApp : public wxApp
{
public:
	bool OnInit(void);

	void OnInitCmdLine(wxCmdLineParser& parser);
	bool OnCmdLineParsed(wxCmdLineParser& parser);
	int OnExit(void);
private:
	wxString meshPath;
	RoRViewerFrame* frame;
};


BEGIN_EVENT_TABLE(RoRViewerFrame, wxFrame)
	EVT_IDLE(RoRViewerFrame::OnIdle)
END_EVENT_TABLE()

#ifdef USE_CONSOLE
IMPLEMENT_APP_CONSOLE(RoRViewerApp)
#else
IMPLEMENT_APP(RoRViewerApp)
#endif

#endif //MESHVIEWERMAIN_H__