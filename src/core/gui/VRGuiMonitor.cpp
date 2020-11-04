#include <gtk/gtk.h>
#include "VRGuiMonitor.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
#include "core/utils/toString.h"
#include "core/utils/VRGlobals.h"

#include <functional>
#include <cairo.h>

#include "wrapper/VRGuiTreeView.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGuiMonitor::VRGuiMonitor() {
    darea = VRGuiBuilder::get()->get_widget("profiler_area");

    gtk_widget_add_events(darea, (int)GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(darea, (int)GDK_BUTTON_RELEASE_MASK);
    gtk_widget_add_events(darea, (int)GDK_POINTER_MOTION_MASK);

    function<bool(cairo_t*)> sig1 = bind(&VRGuiMonitor::draw, this, placeholders::_1);
    function<bool(GdkEventButton*)> sig2 = bind(&VRGuiMonitor::on_button, this, placeholders::_1);
    function<void(void)> sig3 = bind(&VRGuiMonitor::select_fkt, this);

    connect_signal(darea, sig1, "draw");
    connect_signal(darea, sig2, "button_press_event");
    connect_signal(darea, sig2, "button_release_event");

    setTreeviewSelectCallback("treeview15", sig3 );
}

bool VRGuiMonitor::on_button(GdkEventButton * event) {
    //int state = 1;
    //if (event->type == GDK_BUTTON_PRESS) state = 0;

    GtkAllocation rect;
    gtk_widget_get_allocation(darea, &rect);
    float w = rect.width;
    float x = 1.0 - event->x/w;

    if (event->y > 40) return true;
    if (event->y >= 20) selFrame = ceil(10*x);
    else selFrameRange = 10*ceil(10*x);

	selectFrame();
	return true;
}

void VRGuiMonitor::select_fkt() {
    string selection = VRGuiTreeView("treeview15").getSelectedStringValue(0);
    if (selection == selRow) return;
    selRow = selection;
    redraw(); // TODO: breaks row selection -> focus?
    //selectTreestoreRow("treeview15", row);
}

void VRGuiMonitor::draw_text(string txt, int x, int y) {
    PangoFontDescription* font = pango_font_description_new();
    pango_font_description_set_family(font, "Monospace");
    pango_font_description_set_weight(font, PANGO_WEIGHT_BOLD);
    PangoLayout* layout = gtk_widget_create_pango_layout(darea, txt.c_str());
    pango_layout_set_font_description(layout, font);
    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(context, x-tw*0.5, y-th);
    pango_cairo_show_layout(context, layout);
}

void VRGuiMonitor::draw_frame(int i, float w, float h, float x, int h0, bool fill) {
    cairo_set_line_width(context, 1.0);
    if (!fill) cairo_set_source_rgb(context, 0, 0.5, 0.9);
    else cairo_set_source_rgb(context, 0.75, 0.9, 1.0);

    float w2 = w*0.5;
    cairo_move_to(context, x+w2, h0);
    cairo_line_to(context, x-w2, h0);
    cairo_line_to(context, x-w2, h0+h);
    cairo_line_to(context, x+w2, h0+h);
    if (fill) cairo_fill(context);
    else cairo_stroke(context);

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

    cairo_set_line_width(context, 1.0);
    cairo_set_source_rgb(context, 0, 0.5, 0.9);
    cairo_rectangle(context, 0,h0,j*d,h);
    cairo_stroke(context);
}

Vec3d VRGuiMonitor::getColor(string name) {
    if (color_map.count(name) == 0) {
        float r = float(rand())/RAND_MAX;
        float g = float(rand())/RAND_MAX;
        float b = float(rand())/RAND_MAX;
        color_map[name] = Vec3d(r,g,b);
    }
    return color_map[name];
}

void VRGuiMonitor::draw_call(int x0, int y0, int w, int h, string name) {
    Vec3d c = getColor(name);

    cairo_set_line_width(context, 0.5);
    if (name == selRow) cairo_set_line_width(context, 1.5);
    cairo_set_source_rgb(context, c[0], c[1], c[2]);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    cairo_rectangle(context, x0,y0,w,h);
    cairo_stroke(context);
}

string VRGuiMonitor::toHex(Vec3d color) {
    stringstream ss;
    Vec3d co = color*255;
    int hcol = (int(co[0]) << 16) + (int(co[1]) << 8) + int(co[2]);
    ss << hex << uppercase << setw(6) << setfill('0') << hcol;
    return "#"+ss.str();
}

void VRGuiMonitor::redraw() {
    GdkWindow* win = gtk_widget_get_window(darea);
    if (win) gdk_window_invalidate_rect( win, NULL, false);
}

bool VRGuiMonitor::draw(cairo_t* cr) {
    int w = gtk_widget_get_allocated_width(darea);
    int h = gtk_widget_get_allocated_height(darea);

    GtkStyleContext* style = gtk_widget_get_style_context(darea);
    context = cr;

    GtkAllocation rect;
    gtk_widget_get_allocation(darea, &rect);

    // construction parameters
    int line_height = 20;
    int width = rect.width;

    // get needed lines
    int lineN = 2; // timeline 1 and 2

    int Nt = 1; // N threads (TODO)
    int Hl = 5; // scale height
    lineN += Nt*Hl;

    gtk_widget_set_size_request(darea, -1, line_height*lineN);

    int N = VRProfiler::get()->getHistoryLength();
    int L = 10;

    draw_timeline(0, N,   L, L*width, line_height, 0,           selFrameRange);
    draw_timeline(0, N/L, 1, width,   line_height, line_height, selFrame);

    map<int, int> threadMap;
    for (auto itr : frame.calls) if (!threadMap.count(itr.second.thread)) threadMap[itr.second.thread] = threadMap.size();

    float fl = 1./(frame.t1 - frame.t0);
    for (auto itr : frame.calls) {
        auto call = itr.second;
        float t0 = (call.t0 - frame.t0)*fl;
        float t1 = (call.t1 - frame.t0)*fl;
        float l = t1-t0;
        float h = 0.1 +l*0.9;
        int tID = threadMap[call.thread];
        draw_call(t0*width, line_height*(tID*6 + 2 + (1-h)*0.5*Hl), l*width, line_height*h*Hl, call.name);
    }

    return true;
}

void VRGuiMonitor::selectFrame() {
    // get frame
    int f = selFrameRange - 10 + selFrame;
    //f = VRGlobals::CURRENT_FRAME -f -1;
    frame = VRProfiler::get()->getFrame(f);

    map<string, float> fkts;
    for (auto itr : frame.calls) {
        auto call = itr.second;
        if (fkts.count(call.name) == 0) fkts[call.name] = 0;
        if (call.t0 == 0) call.t0 = call.t1;
        if (call.t1 == 0) call.t1 = call.t0;
        fkts[call.name] += call.t1 - call.t0;
    }

    // update list
    GtkListStore* store = (GtkListStore*)VRGuiBuilder::get()->get_object("prof_fkts");
    gtk_list_store_clear(store);
    for (auto c : fkts) {
        string col = toHex( getColor(c.first) );
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);

        uint T = c.second;
        gtk_list_store_set(store, &iter, 0, c.first.c_str(), -1);
        gtk_list_store_set(store, &iter, 1, T, -1);
        gtk_list_store_set(store, &iter, 2, col.c_str(), -1);
    }

    setLabel("Nframe", toString(frame.fID));
    setLabel("Tframe", toString((frame.t1 - frame.t0)/1000.0)+"ms");
    setLabel("Nchanges", toString(frame.Nchanged));
    setLabel("Ncreated", toString(frame.Ncreated));

    redraw();
}

OSG_END_NAMESPACE;
