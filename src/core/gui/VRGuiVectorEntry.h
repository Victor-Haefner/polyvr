#ifndef VRGUIVECTORENTRY_H_INCLUDED
#define VRGUIVECTORENTRY_H_INCLUDED

#include <string>
#include <gdk/gdkevents.h>
#include <sigc++/functors/slot.h>
#include <OpenSG/OSGVector.h>

namespace Gtk {
    class Entry;
    class Label;
}

using namespace std;

class VRGuiVectorEntry {
    private:
        Gtk::Entry* ex = 0;
        Gtk::Entry* ey = 0;
        Gtk::Entry* ez = 0;
        Gtk::Label* lbl = 0;

        static bool proxy(GdkEventFocus* focus, sigc::slot<void, OSG::Vec3f&> sig, Gtk::Entry* ex, Gtk::Entry* ey, Gtk::Entry* ez);
        static bool proxy2D(GdkEventFocus* focus, sigc::slot<void, OSG::Vec2f&> sig, Gtk::Entry* ex, Gtk::Entry* ey);

    public:
        VRGuiVectorEntry();

        void init(string placeholder, string label,  sigc::slot<void, OSG::Vec3f&> sig);
        void init2D(string placeholder, string label,  sigc::slot<void, OSG::Vec2f&> sig);

        void set(OSG::Vec3f v);
        void set(OSG::Vec2f v);

        void setFontColor(OSG::Vec3f c);
};

#endif // VRGUIVECTORENTRY_H_INCLUDED
