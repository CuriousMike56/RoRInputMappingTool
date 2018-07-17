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
#ifndef JOYWIZARD_H__
#define JOYWIZARD_H__

#define MAX_JOYSTICKS 10
#define MAX_JOYSTICK_SLIDERS 4
#define MAX_JOYSTICK_AXIS 32
#include "wx/wizard.h"
#include "wx/sizer.h"
#include "wx/button.h"
#include "wx/stattext.h"
#include <wx/timer.h>
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/gauge.h>
#include <wx/scrolbar.h>
#include <wx/grid.h>
#include <wx/treectrl.h>
#include <wx/tooltip.h>
#include <wx/mediactrl.h>
#include <wx/intl.h>
#include <wx/textfile.h>
#include <wx/cmdline.h>
#include <wx/html/htmlwin.h>
#include <wx/uri.h>
#include <wx/dir.h>
#include <wx/fs_inet.h>
#include <wx/scrolwin.h>
#include "statpict.h"
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/version.h>
#include "Ogre.h"
#include "InputEngine.h"
#include "utils.h"

#include "joywiz.xpm"
#define TXTWRAP 400

// event IDs
enum {
	ID_WIZARD,
	ID_TIMER0,
	ID_BTN_RESET,
};

// axistypes
enum {
	ID_EVUP,
	ID_EVDOWN,
	ID_EVLEFT,
	ID_EVRIGHT,
	ID_EVROLL_LEFT,
	ID_EVROLL_RIGHT,
	ID_EV_ACC,
	ID_EV_BRAKE,
	ID_EV_BTN1,
	ID_EV_BTN2,
	ID_EV_BTN3,
	ID_EV_BTN4,
};

// utils for the wizard
inline void setControlEnable(wxWizard *wiz, int id, bool enable)
{
	wxWindow *win = wiz->FindWindowById(id);
	if(win) win->Enable(enable);
}

class JoystickWizard : public wxWizard
{
public:
	JoystickWizard(size_t ihandle, wxWindow *parent);
	wxWizardPage *GetFirstPage() const { return m_page1; }

	void OnPageChanging(wxWizardEvent& event);
private:
	size_t ihandle;
	wxWizardPageSimple *m_page1;
	DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// pages for our wizard
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
	PresentationPage(wxWizard *parent) : wxWizardPageSimple(parent)
	{
		m_bitmap = wxBitmap(joywiz_xpm);
		wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *tst;
		//GetParent()->SetBackgroundColour(*wxWHITE);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Welcome to the Joystick Wizard\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("This program will help you to setup your input device for Rigs of Rods.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Please connect all your input devices you want to use before starting this wizard.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Click on Next to continue.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

		SetSizer(mainSizer);
		mainSizer->Fit(this);
	}
};

class AxisPage : public wxWizardPageSimple, public EnterLeavePage
{
protected:
	wxStaticText *text;
	wxStaticText *text2;
	wxTimer *timer;
	std::string lastCombo;
	int lastBtn, lastJoy;
	event_trigger_t t;
	int eventType;
	size_t ihandle;
	int selectedJoystickLast, selectedAxisLast;
	float joyMinState[MAX_JOYSTICKS][MAX_JOYSTICK_AXIS];
	float joyMaxState[MAX_JOYSTICKS][MAX_JOYSTICK_AXIS];
	float joySliderMinState[MAX_JOYSTICKS][2][MAX_JOYSTICK_SLIDERS];
	float joySliderMaxState[MAX_JOYSTICKS][2][MAX_JOYSTICK_SLIDERS];
public:
	AxisPage(size_t ihandle, int eventType, wxWizard *parent) : wxWizardPageSimple(parent), eventType(eventType), ihandle(ihandle)
    {
		timer = new wxTimer(this, ID_TIMER0);

		wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
		text = new wxStaticText(this, wxID_ANY, wxT("Please move an axis"));
		sizer->Add(text, 1, wxGROW);
		text2 = new wxStaticText(this, wxID_ANY, wxString());
		sizer->Add(text2, 1, wxGROW);

		wxButton *btn = new wxButton(this, ID_BTN_RESET, _("Reset"), wxPoint(0, 0));
		sizer->Add(btn, 1, wxGROW);
    }

	bool OnEnter(bool forward)
	{
		reset();
		bool captureMouse = false;
		try
		{
			//if(!INPUTENGINE.setup((size_t)this->GetHandle(), true, false))
			if(!INPUTENGINE.setup(ihandle, true, false))
			{
				//wxMessageDialog(this, wxString(_("Could not write to input.map file")), wxString(_("Write error")), wxOK||wxICON_ERROR).ShowModal();
				wxMessageDialog(this, wxT("Could not create input system."), wxT("error while creating input system"), wxOK||wxICON_ERROR).ShowModal();
			}
		} catch (std::exception& e)
		{
			wxMessageDialog(this, conv(e.what()), wxT("exception while creating input system"), wxOK||wxICON_ERROR, wxDefaultPosition).ShowModal();;
		}
		timer->Start(50);

		INPUTENGINE.resetKeys();
		return true;
	}

	bool OnLeave(bool forward)
	{
		timer->Stop();
		return true;
	}

	void reset()
	{
		selectedJoystickLast=-1;
		selectedAxisLast=-1;

		// reset joy min/maxs
		for(int i=0;i<MAX_JOYSTICKS;i++)
		{
			for(int x=0;x<MAX_JOYSTICK_AXIS;x++)
			{
				joyMinState[i][x]=0;
				joyMaxState[i][x]=0;
			}
			for(int x=0;x<MAX_JOYSTICK_SLIDERS;x++)
			{
				joySliderMinState[i][0][x]=0;
				joySliderMaxState[i][0][x]=0;
				joySliderMinState[i][1][x]=0;
				joySliderMaxState[i][1][x]=0;
			}
		}
		//if(eventType == ID_EVUP)
		text2->SetLabel(_("(Please move an Axis)\n"));

	}

	void update()
	{
		if(eventType == ID_EV_BTN1)
		{
			int joyBtn=-1, joyNum=-1;
			int res = INPUTENGINE.getCurrentJoyButton(joyNum, joyBtn);
			if(res && joyBtn != lastBtn && joyNum != lastJoy)
			{
				std::string str = conv(wxString::Format(_T("%d"), joyBtn));
				strcpy(t.configline, str.c_str());
				t.joystickButtonNumber = joyBtn;
				wxString s = conv(str);
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastBtn = joyBtn;
				lastJoy = joyNum;
			} else if (!res)
			{
				wxString s = _("(Please press a Joystick Button)");
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastBtn = -1;
				lastJoy = -1;
			} else if(joyBtn >= 0 && joyNum >= 0)
			{
				//closeWindow();
			}
		} else if(eventType == ID_EVUP)
		{
			std::string str = "";
			int selectedJoystick=-1, selectedAxis = -1, cd=0;
			for(int i=0;i<INPUTENGINE.getNumJoysticks();i++)
			{
				OIS::JoyStickState *j = INPUTENGINE.getCurrentJoyState(i);

				std::vector<OIS::Axis>::iterator it;
				int counter = 0, cdi=0;
				for(it=j->mAxes.begin();it!=j->mAxes.end();it++, counter++)
				{
					float value = 100*((float)it->abs/(float)OIS::JoyStick::MAX_AXIS);
					if(value < joyMinState[i][counter]) joyMinState[i][counter] = value;
					if(value > joyMaxState[i][counter]) joyMaxState[i][counter] = value;
					float delta = fabs((float)(joyMaxState[i][counter]-joyMinState[i][counter]));
					//if(value > 10 || delta > 10)
					{
						str += std::string("Joystick ") + conv(wxString::Format(_T("%d"), i)) + std::string(", Axis ") + conv(wxString::Format(_T("%02d"), counter)) + std::string(": ") + conv(wxString::Format(_T("%03d"), (int)value)) + std::string(" (DELTA:") + conv(wxString::Format(_T("%0.2f"), delta))+ std::string(")\n");
						cdi++;
					}
				}
				if(cdi)
					str += std::string(" ---- \nResult: ");
				cd+=cdi;

				// check for delta on each axis
				float maxdelta=0;
				int sAxis=-1, sJoy=-1;
				for(int c=0;c<=counter;c++)
				{
					float delta = fabs((float)(joyMaxState[i][c]-joyMinState[i][c]));
					if(delta > maxdelta && delta > 50)
					{
						selectedAxis = c;
						selectedJoystick = i;
						maxdelta = delta;
					}
				}
			}

			if(!cd) str = conv(_("(Please move an Axis)\n"));
			if(selectedJoystick>=0 && selectedAxis >= 0)
			{
				// changed?
				float delta = fabs((float)(joyMaxState[selectedJoystick][selectedAxis]-joyMinState[selectedJoystick][selectedAxis]));
				if(selectedAxis != selectedAxisLast && selectedJoystick != selectedJoystickLast)
				{
					if(selectedAxisLast != -1)
					{
						// reset max/mins
						joyMaxState[selectedJoystickLast][selectedAxisLast] = 0;
						joyMinState[selectedJoystickLast][selectedAxisLast] = 0;
					}
					selectedJoystickLast = selectedJoystick;
					selectedAxisLast = selectedAxis;
				}
				static float olddeta = 0;
				bool upper = (joyMaxState[selectedJoystick][selectedAxis] > 50);
				if(delta > 50 && delta < 120)
				{
					str += std::string("Joystick ") + conv(wxString::Format(_T("%d"), selectedJoystick)) + std::string(", Axis ") + conv(wxString::Format(_T("%d"), selectedAxis)) + \
						(upper?std::string(" UPPER"):std::string(" LOWER"));
					t.joystickAxisNumber = selectedAxis;
					t.joystickNumber = selectedJoystick;
					t.joystickAxisRegion = (upper?1:-1); // half axis
					strcpy(t.configline, (upper?"UPPER":"LOWER"));
				}
				else if(delta > 120)
				{
					str += std::string("Joystick ") + conv(wxString::Format(_T("%d"), selectedJoystick)) + std::string(", Axis ") + conv(wxString::Format(_T("%d"), selectedAxis));
					t.joystickAxisNumber = selectedAxis;
					t.joystickNumber = selectedJoystick;
					t.joystickAxisRegion = 0; // full axis
					strcpy(t.configline, "");
				}

				str += std::string("\n====\nPress RETURN or ESC to complete\nPress 'c' to reset ranges\n");
				//str = wxString::Format(_T("%f"), joyMaxState[0]) + "/" + wxString::Format(_T("%f"), joyMinState[i][0]) + " - " + wxString::Format(_T("%f"), fabs((float)(joyMaxState[0]-joyMinState[i][0])));
			}
			wxString s = conv(str);
			if(text2->GetLabel() != s)
				text2->SetLabel(s);
		} else if(eventType == ID_EVDOWN)
		{
#if 0			
			std::string str = "";
			int sliders=0;
			for(int i=0;i<INPUTENGINE.getNumJoysticks();i++)
			{
				OIS::JoyStickState *j = INPUTENGINE.getCurrentJoyState(i);

				int slidercount = INPUTENGINE.getJoyComponentCount(OIS::OIS_Slider, i);
				if(!slidercount) continue;
				sliders += slidercount;

				std::vector<OIS::Slider>::iterator it;
				int counter = 0;

				for(int s=0;s<slidercount;s++)
				{
					float value = ((t.eventtype == ET_JoystickSliderX)?j->mSliders[s].abX:j->mSliders[s].abY);

					value = 100*(value/(float)OIS::JoyStick::MAX_AXIS);
					if(value < joySliderMinState[i][counter]) joySliderMinState[i][counter] = value;
					if(value > joySliderMaxState[i][counter]) joySliderMaxState[i][counter] = value;
					//str += "Axis " + wxString::Format(_T("%d"), counter) + ": " + wxString::Format(_T("%0.2f"), value) + "\n";
				}

				// check for delta on each axis
				str = conv(_("(Please move a Slider)\n"));
				float maxdelta=0;
				int selectedSlider = -1;
				for(int c=0;c<=counter;c++)
				{
					float delta = fabs((float)(joySliderMaxState[i][c]-joySliderMinState[i][c]));
					if(delta > maxdelta && delta > 50)
					{
						selectedSlider = c;
						maxdelta = delta;
					}
				}

				if(selectedSlider >= 0)
				{
					float delta = fabs((float)(joySliderMaxState[i][selectedSlider]-joySliderMinState[i][selectedSlider]));
					static float olddeta = 0;
					bool upper = (joyMaxState[i][selectedSlider] > 50);
					if(delta > 50 && delta < 120)
					{
						str = std::string("Slider ") + \
								conv(wxString::Format(_T("%d"), selectedSlider)) + \
								(upper?std::string(" UPPER"):std::string(" LOWER")); // + " - " + wxString::Format(_T("%f"), lastJoyEventTime);
						t.joystickSliderNumber = selectedSlider;
						// todo: support this
						//t.joystickAxisRegion = (upper?1:-1); // half axis
						strcpy(t.configline, "");
					}
					else if(delta > 120)
					{
						str = std::string("Slider ") + conv(wxString::Format(_T("%d"), selectedSlider)); //+ " - " + wxString::Format(_T("%f"), lastJoyEventTime);
						t.joystickSliderNumber = selectedSlider;
						strcpy(t.configline, "");
					}
				}
				//str = wxString::Format(_T("%f"), joyMaxState[0]) + "/" + wxString::Format(_T("%f"), joyMinState[i][0]) + " - " + wxString::Format(_T("%f"), fabs((float)(joyMaxState[0]-joyMinState[i][0])));
				wxString s = conv(str);
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
			}
			if(!sliders)
			{
				// no sliders found!
			}
#endif //0
		}
	}

	void OnButReset(wxCommandEvent& event)
	{
		reset();
	}

	void OnTimer(wxTimerEvent& event)
	{
		try
		{
			INPUTENGINE.Capture();
		} catch (...) //std::exception& e)
		{
			//wxMessageDialog(this, conv(e.what()), wxT("exception while updating input system"), wxOK||wxICON_ERROR, wxDefaultPosition).ShowModal();;
		}
		update();
	}

private:
	DECLARE_EVENT_TABLE()
};

#endif //JOYWIZARD_H__
