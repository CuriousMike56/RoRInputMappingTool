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
#include "wxutils.h"

#include <iostream>
#include <sstream>

// this is in a separate file to reduce the clutter in the global namespace and their conflicts

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
# ifdef __WXGTK__
#include <gdk/gdk.h>
#include <gtk/gtk.h> // just this should suffice as it should include gdk.h itself
/* Seems to be needed under Linux */
//#include <wx/gtk/win_gtk.h>
#include "wx/gtk/private/win_gtk.h"
#include <gdk/gdkx.h>
#include <GL/glx.h>

#if  wxCHECK_VERSION(2, 9, 0)
#define piz(wxwin) WX_PIZZA((wxwin)->m_wxwindow)
#define GetXWindow(wxwin) (wxwin)->m_wxwindow ? \
	    GDK_WINDOW_XWINDOW(((GtkWidget*)piz(wxwin))->window) : \
	    GDK_WINDOW_XWINDOW((wxwin)->m_widget->window)
#else
#define GetXWindow(wxwin) (wxwin)->m_wxwindow ? \
	    GDK_WINDOW_XWINDOW(GTK_PIZZA((wxwin)->m_wxwindow)->bin_window) : \
	    GDK_WINDOW_XWINDOW((wxwin)->m_widget->window)
#endif

#ifdef __WXX11__
#include "wx/x11/privx.h"
#define GetXWindow(wxwin)   ((Window)(wxwin)->GetHandle())
#endif

# endif //__WXGTK__
#endif //OGRE_PLATFORM_LINUX

std::string getWindowHandle(wxWindow *window)
{
#if WIN32
    std::stringstream sstr;
	size_t hWnd = (size_t)window->GetHandle();
    sstr << hWnd;
    std::string result = sstr.str();
	return result;
#else

    // TODO: Someone test this. you might to use "parentWindowHandle" if this
    // does not work.  Ogre 1.2 + Linux + GLX platform wants a string of the
    // format display:screen:window, which has variable types ulong:uint:ulong.
    GtkWidget* widget = window->GetHandle();
    //gtk_widget_set_double_buffered (widget, FALSE);
    gtk_widget_realize( widget );	// Mandatory. Otherwise, a segfault happens.
    //XSync(GDK_WINDOW_XDISPLAY(widget->window), False);
    
    Display* display = GDK_WINDOW_XDISPLAY( widget->window );

    
    //Window wid = GetXWindow(window);
    Window wid = GDK_WINDOW_XWINDOW( widget->window );

    std::stringstream sstr;
    /* Get the right display (DisplayString() returns ":display.screen") */
    std::string displayStr = DisplayString( display );
    displayStr = displayStr.substr( 1, ( displayStr.find( ".", 0 ) - 1 ) );

    int screenNum = DefaultScreen( display );

    /* Put all together */
    printf("using display: %s\n", displayStr.c_str());
    printf("using screen: %d\n", screenNum);
    printf("using window: 0x%x\n", wid);

    // old format:
    sstr << displayStr << ':' << screenNum << ':' << wid;
    std::string result = sstr.str();

    printf("getWindowHandle() = %s\n", result.c_str());
    return result;
#endif
}
