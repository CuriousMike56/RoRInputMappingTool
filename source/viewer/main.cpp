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

#include "main.h"

#include "panel_meshprop.h"
#include "panel_meshtree.h"

#include "wxutils.h"
#include "Settings.h"

#include "display_mode.xpm"

Window3D::Window3D(wxWindow* parent, wxWindowID id) : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxTRANSPARENT_WINDOW | wxBORDER_NONE | wxNO_FULL_REPAINT_ON_RESIZE)
{
	wnd = NULL;
	cam = NULL;
	handler = NULL;
}

void Window3D::OnSize(wxSizeEvent& e)
{
	if (!wnd || !cam) return;

	// Setting new size;
	int width;
	int height;
	GetSize(&width, &height);
	wnd->resize(width, height);

	// Letting Ogre know the window has been resized;
	wnd->windowMovedOrResized();

	// Set the aspect ratio for the new size;
	cam->setAspectRatio(Ogre::Real(width) / Ogre::Real(height));
}

void Window3D::OnLeftUp(wxMouseEvent& e)
{
	if (handler) handler->OnLeftUp(e.m_x, e.m_y);
}

void Window3D::OnMouseMove(wxMouseEvent& e)
{
	if (handler) handler->OnMouseMove(e);
}

void Window3D::OnKeyDown(wxKeyEvent& e)
{
	if (handler) handler->OnKeyDown(e.ControlDown(), e.AltDown(), e.ShiftDown(), e.GetKeyCode());
}

bool RoRViewerApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	frame = new RoRViewerFrame(L"RoR Viewer");
	SetTopWindow(frame);
	frame->Show();
	
	frame->InitializeAUI();
	if (!frame->InitializeRoRViewer(meshPath))
	{
		frame->DeinitializeAUI();
		return false;
	}

	return true;
}

void RoRViewerApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.AddParam(_T("<file>"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY);
}

bool RoRViewerApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	meshPath = parser.GetParam();
	return !meshPath.empty();
}

int RoRViewerApp::OnExit(void)
{
	return 0;
}

RoRViewerFrame::RoRViewerFrame(wxString title) : wxFrame(NULL, wxID_ANY, title)
{
	msx = 0;
	msy = 0;
	msw = 0;

	viewer = NULL;

	SetMinSize(wxSize(640, 480));
	SetClientSize(640, 480);

	Show();
}

RoRViewerFrame::~RoRViewerFrame()
{
	DenitializeRoRViewer();
	DeinitializeAUI();

}
void RoRViewerFrame::OnViewToolClick(wxCommandEvent& e)
{
	switch(e.GetId())
	{
		case ID_TOOL_MODE_TEXTURE:
			viewer->GetCamera()->setPolygonMode(Ogre::PM_SOLID);
			break;
		case ID_TOOL_MODE_WIREFRAME:
			viewer->GetCamera()->setPolygonMode(Ogre::PM_WIREFRAME);
			break;
		case ID_TOOL_MODE_POINT:
			viewer->GetCamera()->setPolygonMode(Ogre::PM_POINTS);
			break;
	};
}

void RoRViewerFrame::InitializeAUI(void)
{
	aui_mgr = new wxAuiManager(this);

	// main 3D viewport
	pane_viewport = new wxAuiPaneInfo();
	pane_viewport->PaneBorder(false);
	pane_viewport->CaptionVisible(false);
	pane_viewport->Caption(L"3D Viewport");
	pane_viewport->FloatingSize(wxSize(200, 300));
	pane_viewport->MinSize(wxSize(200, 300));
	pane_viewport->Center();
	pane_viewport->Dock();
	panel_viewport = new Window3D(this, ID_VIEWPORT);
	panel_viewport->SetBackgroundColour(wxColour(L"Gray"));
	aui_mgr->AddPane(panel_viewport, *pane_viewport);
	panel_viewport->handler = this;

	// property grid
	pane_meshprop = new wxAuiPaneInfo();
	pane_meshprop->CaptionVisible(true);
	pane_meshprop->Caption(L"Properties");
	pane_meshprop->FloatingSize(wxSize(200, 150));
	pane_meshprop->MinSize(wxSize(200, 250));
	//pane_meshprop->MaxSize(wxSize(200, 150));
	pane_meshprop->BestSize(wxSize(200, 250));
	pane_meshprop->Resizable(true);
	pane_meshprop->TopDockable(false);
	pane_meshprop->BottomDockable(false);
	pane_meshprop->Left();
	pane_meshprop->Position(1);
	pane_meshprop->Dock();
	panel_meshprop = new PanelMeshProp(0, this, wxID_ANY);
	aui_mgr->AddPane(panel_meshprop, *pane_meshprop);

	// tree
	pane_meshtree = new wxAuiPaneInfo();
	pane_meshtree->CaptionVisible(true);
	pane_meshtree->Caption(L"Tree");
	pane_meshtree->FloatingSize(wxSize(200, 150));
	pane_meshtree->MinSize(wxSize(200, 100));
	//pane_meshtree->MaxSize(wxSize(200, 150));
	pane_meshtree->BestSize(wxSize(200, 200));
	pane_meshtree->Resizable(true);
	pane_meshtree->TopDockable(false);
	pane_meshtree->BottomDockable(false);
	pane_meshtree->Left();
	pane_meshtree->Position(0);
	pane_meshtree->Dock();
	panel_meshtree = new PanelMeshTree(0, this, wxID_ANY);
	panel_meshtree->setPropGrid(panel_meshprop);
	aui_mgr->AddPane(panel_meshtree, *pane_meshtree);

	// toolbars
	wxAuiToolBar *view_toolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	view_toolbar->SetToolBitmapSize(wxSize(16,16));
	wxBitmap bmpDisMode = wxBitmap(display_mode_xpm);
	view_toolbar->AddTool(ID_TOOL_MODE_TEXTURE, "Texture", bmpDisMode, "show textures", wxITEM_RADIO);
	aui_mgr->Bind(wxEVT_COMMAND_TOOL_CLICKED, &RoRViewerFrame::OnViewToolClick, this, ID_TOOL_MODE_TEXTURE);
	view_toolbar->AddTool(ID_TOOL_MODE_WIREFRAME, "Wireframe", bmpDisMode, "show wireframe", wxITEM_RADIO);
	aui_mgr->Bind(wxEVT_COMMAND_TOOL_CLICKED, &RoRViewerFrame::OnViewToolClick, this, ID_TOOL_MODE_WIREFRAME);
	view_toolbar->AddTool(ID_TOOL_MODE_POINT, "Point", bmpDisMode, "show points", wxITEM_RADIO);
	aui_mgr->Bind(wxEVT_COMMAND_TOOL_CLICKED, &RoRViewerFrame::OnViewToolClick, this, ID_TOOL_MODE_POINT);
	view_toolbar->Realize();

	wxAuiPaneInfo *pane_toolbar = new wxAuiPaneInfo();
	pane_toolbar->ToolbarPane();
	pane_toolbar->LeftDockable(false);
	pane_toolbar->RightDockable(false);
	pane_toolbar->Top();
	pane_toolbar->Row(0);
	pane_toolbar->Position(0);
	aui_mgr->AddPane(view_toolbar, *pane_toolbar);

	// done with AUI
	aui_mgr->Update();

	// try to load the old window layout
	String perspective_str = SETTINGS.getSetting("ViewerPerspective");
	if(!perspective_str.empty())
	{
		aui_mgr->LoadPerspective(wxString(perspective_str.c_str()));
	}
}

void RoRViewerFrame::DeinitializeAUI(void)
{
	// save the window layout
	string perspective_str = std::string(aui_mgr->SavePerspective().mb_str());
	SETTINGS.setSetting("ViewerPerspective",perspective_str);
	SETTINGS.saveSettings();

	aui_mgr->UnInit();
}

bool RoRViewerFrame::InitializeRoRViewer(wxString meshPath)
{
	string mstr = std::string(meshPath.mb_str());
	viewer = new RoRViewer(mstr);
	if (!viewer->Initialize(getWindowHandle(panel_viewport)))
	{
		delete viewer;
		viewer = NULL;
		return false;
	}
	panel_meshprop->setViewer(viewer);
	panel_meshtree->setViewer(viewer);

	panel_viewport->wnd = viewer->GetOgreWindow();
	panel_viewport->cam = viewer->GetCamera();

	return true;
}

void RoRViewerFrame::DenitializeRoRViewer(void)
{
	panel_viewport->handler = NULL;
	panel_viewport->wnd = NULL;

	if (viewer)
	{
		viewer->Deinitialize();
		delete viewer;
		viewer = NULL;
	}
}

void RoRViewerFrame::updatePanelData()
{
	panel_meshprop->updateData();
	panel_meshtree->updateData();
}

void RoRViewerFrame::OnIdle(wxIdleEvent& e)
{
	viewer->Update();
	updatePanelData();

	e.RequestMore();
}

void RoRViewerFrame::OnLeftUp(int x, int y)
{
}

void RoRViewerFrame::OnMouseMove(wxMouseEvent& e)
{
	int xp = msx - e.m_x;
	int yp = msy - e.m_y;
	int wp = e.m_wheelRotation;
	msx = e.m_x;
	msy = e.m_y;

	/// shit behaves badly, to be fixed!
	if (e.LeftIsDown() || wp != 0)
	{
		viewer->TurnCamera(Vector3(xp, yp, wp*0.02f));
	}
}

void RoRViewerFrame::OnKeyDown(bool ctrl, bool alt, bool shift, int key)
{
}
