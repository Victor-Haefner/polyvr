#ifndef VRGUIMONITOR_H_INCLUDED
#define VRGUIMONITOR_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <gtkmm/drawingarea.h>
#include <map>

#include "core/utils/VRProfiler.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiMonitor {
    private:
        Glib::RefPtr<Gtk::DrawingArea> da;
        Cairo::RefPtr<Cairo::Context> cr;

        map<string, Vec3f> color_map;
        Vec3f getColor(string name);
        string toHex(Vec3f color);

        string selRow;
        int selFrameRange = 10;
        int selFrame = 1;

        VRProfiler::Frame frame;

        void draw_frame(int i, float w, float h, float x, int h0, bool fill = false);
        void draw_timeline(int N0, int N1, int DN, int w, int h, int h0, int selection);
        void draw_call(int x0, int y0, int w, int h, string name);
        void draw_text(string txt, int x, int y);
        bool draw(GdkEventExpose* e);

        bool on_button(GdkEventButton* e);

        void select_fkt();

    public:
        VRGuiMonitor();
        void selectFrame();
        void redraw();
};

OSG_END_NAMESPACE

#endif // VRGUIMONITOR_H_INCLUDED
