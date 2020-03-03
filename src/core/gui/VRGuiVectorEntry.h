#ifndef VRGUIVECTORENTRY_H_INCLUDED
#define VRGUIVECTORENTRY_H_INCLUDED

#include <string>
#include <gdk/gdkevents.h>
#include <sigc++/functors/slot.h>
#include "core/math/OSGMathFwd.h"

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

        static bool proxy(GdkEventFocus* focus, sigc::slot<void, OSG::Vec3d&> sig, Gtk::Entry* ex, Gtk::Entry* ey, Gtk::Entry* ez);
        static bool proxy2D(GdkEventFocus* focus, sigc::slot<void, OSG::Vec2d&> sig, Gtk::Entry* ex, Gtk::Entry* ey);

    public:
        VRGuiVectorEntry();

        void init(string placeholder, string label,  sigc::slot<void, OSG::Vec3d&> sig);
        void init2D(string placeholder, string label,  sigc::slot<void, OSG::Vec2d&> sig);

        void set(OSG::Vec3d v);
        void set(OSG::Vec2d v);

        void setFontColor(OSG::Vec3d c);
};

#endif // VRGUIVECTORENTRY_H_INCLUDED
