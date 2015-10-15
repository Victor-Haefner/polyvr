#include "VRGuiMonitor.h"
#include "VRGuiUtils.h"
#include "core/utils/toString.h"
#include <gtkmm/window.h>
#include <gtkmm/liststore.h>
#include <cairomm/context.h>
#include <pangomm/context.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiMonitor_FktColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiMonitor_FktColumns() { add(fkt); add(time); add(color); }
        Gtk::TreeModelColumn<Glib::ustring> fkt;
        Gtk::TreeModelColumn<Glib::ustring> time;
        Gtk::TreeModelColumn<Glib::ustring> color;
};

VRGuiMonitor::VRGuiMonitor() {
    Gtk::DrawingArea* _da;
    VRGuiBuilder()->get_widget("profiler_area", _da);
    da = Glib::RefPtr<Gtk::DrawingArea>(_da);

    da->add_events((Gdk::EventMask)GDK_BUTTON_PRESS_MASK);
    da->add_events((Gdk::EventMask)GDK_BUTTON_RELEASE_MASK);
    da->add_events((Gdk::EventMask)GDK_POINTER_MOTION_MASK);

    da->signal_expose_event().connect( sigc::mem_fun(*this, &VRGuiMonitor::draw) );
    da->signal_button_press_event().connect(sigc::mem_fun(*this, &VRGuiMonitor::on_button) );
    da->signal_button_release_event().connect(sigc::mem_fun(*this, &VRGuiMonitor::on_button) );

    setTreeviewSelectCallback("treeview15", sigc::mem_fun(*this, &VRGuiMonitor::select_fkt) );
}

bool VRGuiMonitor::on_button(GdkEventButton * event) {
    //int state = 1;
    //if (event->type == GDK_BUTTON_PRESS) state = 0;

    float w = da->get_allocation().get_width();
    float x = 1.0 - event->x/w;

    if (event->y > 40) return true;
    if (event->y >= 20) selFrame = ceil(10*x);
    else selFrameRange = 10*ceil(10*x);

	selectFrame();
	return true;
}

void VRGuiMonitor::select_fkt() {
    auto row = getTreeviewSelected("treeview15");
    if (!row) return;
    VRGuiMonitor_FktColumns cols;
    string selection = row->get_value(cols.fkt);
    if (selection == selRow) return;
    selRow = selection;
    redraw(); // TODO: breaks row selection -> focus?
    //selectTreestoreRow("treeview15", row);
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

Vec3f VRGuiMonitor::getColor(string name) {
    if (color_map.count(name) == 0) {
        float r = float(rand())/RAND_MAX;
        float g = float(rand())/RAND_MAX;
        float b = float(rand())/RAND_MAX;
        color_map[name] = Vec3f(r,g,b);
    }
    return color_map[name];
}

void VRGuiMonitor::draw_call(int x0, int y0, int w, int h, string name) {
    Vec3f c = getColor(name);

    cr->set_line_width(0.5);
    if (name == selRow) cr->set_line_width(1.5);
    cr->set_source_rgb(c[0], c[1], c[2]);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    cr->rectangle(x0,y0,w,h);
    cr->stroke();
}

string VRGuiMonitor::toHex(Vec3f color) {
    stringstream ss;
    Vec3f co = color*255;
    int hcol = (int(co[0]) << 16) + (int(co[1]) << 8) + int(co[2]);
    ss << hex << uppercase << setw(6) << setfill('0') << hcol;
    return "#"+ss.str();
}

void VRGuiMonitor::redraw() {
    Glib::RefPtr<Gdk::Window> win = da->get_window();
    if (win) gdk_window_invalidate_rect( win->gobj(), NULL, false);
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

    int Nt = 1; // N threads (TODO)
    int Hl = 5; // scale height
    lineN += Nt*Hl;

    da->set_size_request(-1, line_height*lineN);

    int N = VRProfiler::get()->getHistoryLength();
    int L = 10;

    draw_timeline(0, N,   L, L*width, line_height, 0,           selFrameRange);
    draw_timeline(0, N/L, 1, width,   line_height, line_height, selFrame);

    float fl = 1./(frame.t1 - frame.t0);
    for (auto itr : frame.calls) {
        auto call = itr.second;
        float t0 = (call.t0 - frame.t0)*fl;
        float t1 = (call.t1 - frame.t0)*fl;
        float l = t1-t0;
        float h = 0.1 +l*0.9;
        draw_call(t0*width, line_height*(2 + (1-h)*0.5*Hl), l*width, line_height*h*Hl, call.name);
    }

    return true;
}

void VRGuiMonitor::selectFrame() {
    // get frame
    int f = selFrameRange - 10 + selFrame;
    frame = VRProfiler::get()->getFrame(f);

    map<string, float> fkts;
    for (auto c : frame.calls) {
        if (fkts.count(c.second.name) == 0) fkts[c.second.name] = 0;
        fkts[c.second.name] += c.second.t1 - c.second.t0;
    }

    // update list
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("prof_fkts"));
    store->clear();
    for (auto c : fkts) {
        string col = toHex( getColor(c.first) );
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, c.first.c_str(), -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 1, toString(c.second).c_str(), -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 2, col.c_str(), -1);
    }

    redraw();
}

OSG_END_NAMESPACE;
