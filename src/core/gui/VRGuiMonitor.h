#ifndef VRGUIMONITOR_H_INCLUDED
#define VRGUIMONITOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <gtkmm/drawingarea.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiMonitor {
    private:
        Glib::RefPtr<Gtk::DrawingArea> da;
        Cairo::RefPtr<Cairo::Context> cr;

        void draw_frame(int i, float w, float h, float x, int h0, bool fill = false);
        void draw_timeline(int N0, int N1, int DN, int w, int h, int h0, int selection);
        void draw_call(int x0, int x1, int h, int h0, string name);
        void draw_text(string txt, int x, int y);
        bool draw(GdkEventExpose* e);

    public:
        VRGuiMonitor();
};

OSG_END_NAMESPACE

#endif // VRGUIMONITOR_H_INCLUDED
