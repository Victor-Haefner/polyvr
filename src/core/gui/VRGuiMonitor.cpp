#include "VRGuiMonitor.h"
#include "VRGuiUtils.h"
#include <gtkmm/window.h>
#include <cairomm/context.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGuiMonitor::VRGuiMonitor() {
    VRGuiBuilder()->get_widget("profiler_area", da);
    da->signal_expose_event().connect( sigc::mem_fun(*this, &VRGuiMonitor::draw) );
}

bool VRGuiMonitor::draw(GdkEventExpose* e) {
    //Glib::RefPtr<Gdk::Drawable> win = Glib::wrap(e->window);
    Glib::RefPtr<Gdk::Drawable> win = da->get_window();
    Cairo::RefPtr<Cairo::Context> cr = win->create_cairo_context();
    int width = e->area.width;
    int height = e->area.height;

    cr->rectangle(e->area.x, e->area.y, e->area.width, e->area.height);
    cr->clip(); // only draw on exposed area

    // coordinates for the center of the window
    int xc, yc;
    xc = width / 2;
    yc = height / 2;

    cr->set_line_width(10.0);

    // draw red lines out from the center of the window
    cr->set_source_rgb(0.8, 0.0, 0.0);
    cr->move_to(0, 0);
    cr->line_to(xc, yc);
    cr->line_to(0, height);
    cr->move_to(xc, yc);
    cr->line_to(width, yc);
    cr->stroke();
    return true;
}

OSG_END_NAMESPACE;
