#ifndef VRGUIMONITOR_H_INCLUDED
#define VRGUIMONITOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <gtkmm/drawingarea.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiMonitor {
    private:
        Gtk::DrawingArea* da;

        bool draw(GdkEventExpose* e);

    public:
        VRGuiMonitor();
};

OSG_END_NAMESPACE

#endif // VRGUIMONITOR_H_INCLUDED
