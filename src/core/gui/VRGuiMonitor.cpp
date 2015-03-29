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

void VRGuiMonitor::draw_text(string txt, int x, int y) {

    Pango::FontDescription font;
    font.set_family("Monospace");
    font.set_weight(Pango::WEIGHT_BOLD);

    Glib::RefPtr<Pango::Layout> layout = da->create_pango_layout(txt);
    layout->set_font_description(font);
    int tw, th;
    layout->get_pixel_size(tw, th);
    cr->move_to(x-tw*0.5, y-th);
    layout->show_in_cairo_context(cr);
}

void VRGuiMonitor::draw_frame(int i, float w, float h, float x, int h0, bool fill) {
    cr->set_line_width(1.0);
    if (!fill) cr->set_source_rgb(0, 0.5, 0.9);
    else cr->set_source_rgb(0.75, 0.9, 1.0);

    float w2 = w*0.5;
    cr->move_to(x+w2, h0);
    cr->line_to(x-w2, h0);
    cr->line_to(x-w2, h0+h);
    cr->line_to(x+w2, h0+h);
    if (fill) cr->fill();
    else cr->stroke();

    draw_text(toString(i), x, h0+h);
}

void VRGuiMonitor::draw_timeline(int N0, int N1, int DN, int w, int h, int h0, int selection) {
    int N = N1-N0;
    float d = float(w)/N;
    int j = 0;
    for (int i=N1; i>=N0; i-=DN) {
        if (selection == i) draw_frame(i, d, h, d*(0.5+j-N0), h0, true);
        draw_frame(i, d, h, d*(0.5+j-N0), h0);
        j++;
    }

    cr->set_line_width(1.0);
    cr->set_source_rgb(0, 0.5, 0.9);
    cr->rectangle(0,h0,j*d,h);
    cr->stroke();
}

void VRGuiMonitor::draw_call(int x0, int x1, int h, int h0, string name) {
    cr->set_line_width(1.0);
    cr->set_source_rgb(0.9, 0.5, 0);
    cr->rectangle(x0,h0,x1,h);
    cr->stroke();

    draw_text(name, x0+x1*0.5, h0+h);
}

bool VRGuiMonitor::draw(GdkEventExpose* e) {
    Glib::RefPtr<Gdk::Drawable> win = da->get_window();
    cr = win->create_cairo_context();
    cr->rectangle(e->area.x, e->area.y, e->area.width, e->area.height);
    cr->clip(); // only draw on exposed area

    auto a = da->get_allocation();

    // construction parameters
    int line_height = 20;
    int width = a.get_width();

    // get needed lines
    int lineN = 2; // timeline 1 and 2
    lineN += 1; // N threads (TODO)

    da->set_size_request(-1, line_height*lineN);

    int N = VRProfiler::get()->getHistoryLength();
    int L = 10;

    // user selection
    int selected_cluster = 10;
    int selected_frame = 2;

    draw_timeline(0, N,   L, L*width, line_height, 0,           selected_cluster);
    draw_timeline(0, N/L, 1, width,   line_height, line_height, selected_frame);

    auto frames = VRProfiler::get()->getFrames();
    VRProfiler::Frame sframe;
    int i=0;
    for (auto frame : frames) {
        // TODO:
        //  get fps per frame
        //  draw curve of fps on frames
        if (i == selected_frame) sframe = frame;
        i++;
    }

    for (auto itr : sframe.calls) {
        auto call = itr.second;
        int fl = sframe.t1 - sframe.t0;
        int t0 = call.t0 - sframe.t0;
        int t1 = call.t1 - sframe.t0;
        t0 = t0*width/fl;
        t1 = t1*width/fl;
        int l = t1-t0;
        draw_call(t0, l, line_height, line_height*2, call.name);
    }

    return true;
}

OSG_END_NAMESPACE;
