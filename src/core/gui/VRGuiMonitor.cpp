#include "VRGuiMonitor.h"
#include "VRGuiUtils.h"
#include "core/utils/toString.h"
#include "core/utils/VRProfiler.h"
#include <gtkmm/window.h>
#include <cairomm/context.h>
#include <pangomm/context.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGuiMonitor::VRGuiMonitor() {
    Gtk::DrawingArea* _da;
    VRGuiBuilder()->get_widget("profiler_area", _da);
    da = Glib::RefPtr<Gtk::DrawingArea>(_da);
    da->signal_expose_event().connect( sigc::mem_fun(*this, &VRGuiMonitor::draw) );
}

void VRGuiMonitor::draw_frame(int i, float w, float h, float x, int h0) {
    cr->set_line_width(1.0);
    cr->set_source_rgb(0, 0.5, 0.9);

    float w2 = w*0.5;
    cr->move_to(x+w2, h0);
    cr->line_to(x-w2, h0);
    cr->line_to(x-w2, h0+h);
    cr->line_to(x+w2, h0+h);
    cr->stroke();

    Pango::FontDescription font;
    font.set_family("Monospace");
    font.set_weight(Pango::WEIGHT_BOLD);

    Glib::RefPtr<Pango::Layout> layout = da->create_pango_layout(toString(i));
    layout->set_font_description(font);
    int tw, th;
    layout->get_pixel_size(tw, th);
    cr->move_to(x-tw*0.5, h0+h-th);
    layout->show_in_cairo_context(cr);
}

void VRGuiMonitor::draw_timeline(int N0, int N1, int DN, int w, int h, int h0) {
    cr->set_line_width(1.0);
    cr->set_source_rgb(0, 0.5, 0.9);

    int N = N1-N0;
    float d = float(w)/(N+1);
    int j = 0;
    for (int i=N0; i<N1; i+=DN) {
        draw_frame(i, d, h, d*(0.5+j-N0), h0);
        j++;
    }

    cr->rectangle(0,h0,j*d,h);
    cr->stroke();
}

bool VRGuiMonitor::draw(GdkEventExpose* e) {
    Glib::RefPtr<Gdk::Drawable> win = da->get_window();
    cr = win->create_cairo_context();
    cr->rectangle(e->area.x, e->area.y, e->area.width, e->area.height);
    cr->clip(); // only draw on exposed area

    // construction parameters
    int line_height = 20;

    // get needed lines
    int lineN = 2; // timeline 1 and 2
    /*for (auto f : VRProfiler::get()->getCalls()) {
        ;
    }*/

    /*da->set_size_request(w, line_height*lineN);

    draw_timeline(0, N, 10, 10*w, h, 0);
    draw_timeline(0, N, 1, w, h, h); // frames N0 to N1, in DN steps, w the total width and h the total height, h0 the height offset
    */
    return true;
}

OSG_END_NAMESPACE;
