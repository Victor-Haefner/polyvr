#ifndef VRGTKWINDOW_H_INCLUDED
#define VRGTKWINDOW_H_INCLUDED

#include "VRWindow.h"

#include <gdk/gdkevents.h>
#include <gdkmm/rectangle.h>
#include <gtk/gtkstyle.h>
#include <sigc++/connection.h>

#include <OpenSG/OSGPassiveWindow.h>

namespace Gtk{
    class DrawingArea;
}

OSG_BEGIN_NAMESPACE;
using namespace std;


class VRGtkWindow : public VRWindow {
    private:
        Gtk::DrawingArea* drawArea = 0;
        GtkWidget* widget = 0;
        PassiveWindowMTRecPtr win;
        bool initialExpose = true;

        vector<sigc::connection> signals;

        bool on_scroll(GdkEventScroll* e);
        void on_realize();
        bool on_expose(GdkEventExpose* e);
        void on_resize(Gdk::Rectangle& a);
        bool on_button(GdkEventButton* e);
        bool on_motion(GdkEventMotion* e);
        bool on_key(GdkEventKey* e);

    public:
        VRGtkWindow(Gtk::DrawingArea* glarea);
        ~VRGtkWindow();

        static VRGtkWindowPtr create(Gtk::DrawingArea* da);
        VRGtkWindowPtr ptr();

        PassiveWindowMTRecPtr getOSGWindow();
        void render(bool fromThread = false);
        void clear(Color3f c);

        void setCursor(string c);

        void save(XMLElementPtr node);
        void load(XMLElementPtr node);
};

OSG_END_NAMESPACE;

#endif // VRGTKWINDOW_H_INCLUDED
