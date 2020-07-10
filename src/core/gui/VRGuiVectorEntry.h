#ifndef VRGUIVECTORENTRY_H_INCLUDED
#define VRGUIVECTORENTRY_H_INCLUDED

#include <string>
#include <functional>
#include "core/math/OSGMathFwd.h"

struct _GtkEntry;
struct _GtkLabel;
struct _GdkEventFocus;

using namespace std;

class VRGuiVectorEntry {
    private:
        _GtkEntry* ex = 0;
        _GtkEntry* ey = 0;
        _GtkEntry* ez = 0;
        _GtkLabel* lbl = 0;

        static bool proxy(_GdkEventFocus* focus, function<void(OSG::Vec3d&)> sig, _GtkEntry* ex, _GtkEntry* ey, _GtkEntry* ez);
        static bool proxy2D(_GdkEventFocus* focus, function<void(OSG::Vec2d&)> sig, _GtkEntry* ex, _GtkEntry* ey);

    public:
        VRGuiVectorEntry();

        void init(string placeholder, string label, function<void(OSG::Vec3d&)> sig);
        void init2D(string placeholder, string label, function<void(OSG::Vec2d&)> sig);

        void set(OSG::Vec3d v);
        void set(OSG::Vec2d v);

        void setFontColor(OSG::Vec3d c);
};

#endif // VRGUIVECTORENTRY_H_INCLUDED
