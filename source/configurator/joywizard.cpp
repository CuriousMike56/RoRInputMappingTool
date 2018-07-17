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

#include "joywizard.h"

//BEGIN_EVENT_TABLE(DownloadPage, wxWizardPageSimple)
//	//EVT_TIMER(ID_TIMER, DownloadPage::OnTimer)
//	EVT_MYSTATUS(wxID_ANY, DownloadPage::OnStatusUpdate )
//END_EVENT_TABLE()

BEGIN_EVENT_TABLE(JoystickWizard, wxWizard)
	EVT_WIZARD_PAGE_CHANGING(ID_WIZARD, JoystickWizard::OnPageChanging)
	EVT_TIMER(ID_TIMER0, AxisPage::OnTimer)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AxisPage, wxWizardPageSimple)
	EVT_TIMER(ID_TIMER0, AxisPage::OnTimer)
	EVT_BUTTON(ID_BTN_RESET, AxisPage::OnButReset)
END_EVENT_TABLE()

JoystickWizard::JoystickWizard(size_t ihandle, wxWindow *parent)
        : wxWizard(parent,ID_WIZARD,_T("Rigs of Rods Joystick Assistant"),
                   wxBitmap(joywiz_xpm),wxDefaultPosition,
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), ihandle(ihandle)
{
    PresentationPage *presentation = new PresentationPage(this);
	AxisPage *axis = new AxisPage(ihandle, ID_EVUP, this);
    wxWizardPageSimple::Chain(presentation, axis);
	this->m_page1 = presentation;
	//ihandle = getOISHandle(this);

    // allow the wizard to size itself around the pages
    GetPageAreaSizer()->Add(presentation);
}

void JoystickWizard::OnPageChanging(wxWizardEvent &event)
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