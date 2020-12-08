#ifndef VRGUIMONITOR_H_INCLUDED
#define VRGUIMONITOR_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <map>

#include "core/utils/VRProfiler.h"

struct _GtkWidget;
struct _GdkEventExpose;
struct _GdkEventButton;
struct _cairo;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiMonitor {
    private:
        _GtkWidget* darea = 0;
        _cairo* context = 0;

        map<string, Vec3d> color_map;
        Vec3d getColor(string name);
        string toHex(Vec3d color);

        string selRow;
        int selFrameRange = 10;
        int selFrame = 1;

        VRProfiler::Frame frame;

        void draw_frame(int i, float w, float h, float x, int h0, bool fill = false);
        void draw_timeline(int N0, int N1, int DN, int w, int h, int h0, int selection);
        void draw_call(int x0, int y0, int w, int h, string name);
        void draw_text(string txt, int x, int y);
        bool draw(_cairo* e);

        bool on_button(_GdkEventButton* e);

        void select_fkt();

    public:
        VRGuiMonitor();
        void selectFrame();
        void redraw();
};

OSG_END_NAMESPACE

#endif // VRGUIMONITOR_H_INCLUDED
