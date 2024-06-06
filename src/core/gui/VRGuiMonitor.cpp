#include "VRGuiMonitor.h"
#include "core/utils/toString.h"
#include "core/utils/VRGlobals.h"
#include "core/scene/VRScene.h"
#include "core/scene/rendering/VRRenderManager.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/gui/VRGuiManager.h"

#include <functional>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGuiMonitor::VRGuiMonitor() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("profiler_update_system", [&](OSG::VRGuiSignals::Options o){ updateSystemInfo(); return true; }, true );
    mgr->addCallback("profiler_update_scene", [&](OSG::VRGuiSignals::Options o){ updateSceneInfo(); return true; }, true );
    mgr->addCallback("profiler_update_performance", [&](OSG::VRGuiSignals::Options o){ updatePerformanceInfo(); return true; }, true );
    mgr->addCallback("profiler_update_frame", [&](OSG::VRGuiSignals::Options o){ updatePerformanceFrameInfo(toInt(o["frame"])); return true; }, true );

    updateSystemInfo();
}

void VRGuiMonitor::updateSystemInfo() {
    auto bToS = [](bool b) -> string { return b ? "yes" : "no"; };

    map<string, string> data;
    data["vendor"] = VRRenderManager::getGLVendor();
    data["version"] = VRRenderManager::getGLVersion();
    data["glsl"] = toString(VRRenderManager::getGLSLVersion());
    data["hasGeomShader"] = bToS(VRRenderManager::hasGeomShader());
    data["hasTessShader"] = bToS(VRRenderManager::hasTessShader());

    uiSignal("set_profiler_system", data);
}

void VRGuiMonitor::updateSceneInfo() {
    int Nobjs = 0;
    int Ngeos = 0;
    int Ntrans = 0;

    auto scene = VRScene::getCurrent();
    if (scene) {
        auto objs = scene->getRoot()->getChildren(true);
        Nobjs = objs.size();
        for (auto& obj : objs) {
            auto trans = dynamic_pointer_cast<VRTransform>(obj);
            if (trans) {
                Ntrans += 1;
                auto geo = dynamic_pointer_cast<VRGeometry>(obj);
                if (geo) {
                    Ngeos += 1;
                }
            }
        }
    }

    map<string, string> data;
    data["Nobjects"] = toString(Nobjs);
    data["Ntransforms"] = toString(Ntrans);
    data["Ngeometries"] = toString(Ngeos);

    uiSignal("set_profiler_scene", data);
}

void VRGuiMonitor::updatePerformanceInfo() {
    int N = VRProfiler::get()->getHistoryLength();


    map<string, string> data;
    data["Nframes"] = toString(N);
    uiSignal("set_profiler_performance", data);
}

void VRGuiMonitor::updatePerformanceFrameInfo(int frameID) {
    auto frame = VRProfiler::get()->getFrame(frameID);

    map<int, int> threadMap;
    for (auto itr : frame.calls) if (!threadMap.count(itr.second.thread)) threadMap[itr.second.thread] = threadMap.size();

    map<string, vector<Vec3i>> fkts;
    for (auto itr : frame.calls) {
        auto call = itr.second;
        if (call.t0 == 0) call.t0 = call.t1;
        if (call.t1 == 0) call.t1 = call.t0;

        int dT = call.t1 - call.t0;
        int dCPU = call.cpu1 - call.cpu0;
        int tID = threadMap[call.thread];
        //fkts[call.name].push_back( Vec3i(dT, dCPU, tID) );
        fkts[call.name].push_back( Vec3i(call.t0, call.t1, tID) );
    }

    uint frameT = frame.t1 - frame.t0;

    map<string, string> data;
    data["ID"] = toString(frame.fID);
    data["duration"] = toString(frameT);
    data["Nchanges"] = toString(frame.Nchanged);
    data["Ncreated"] = toString(frame.Ncreated);
    data["Nthreads"] = toString(threadMap.size());
    data["t0"] = toString(frame.t0);
    data["t1"] = toString(frame.t1);
    data["calls"] = toString(fkts);

    /*for (auto c : fkts) {
        string col = toHex( getColor(c.first) );

        uint T = c.second.T;
        uint CPU1 = c.second.C / c.second.T * 100;
        uint CPU2 = c.second.C / fT * 100;
        gtk_list_store_set(store, &iter, 0, c.first.c_str(), -1);
        gtk_list_store_set(store, &iter, 1, T, -1);
        gtk_list_store_set(store, &iter, 2, CPU1, -1);
        gtk_list_store_set(store, &iter, 3, CPU2, -1);
        gtk_list_store_set(store, &iter, 4, col.c_str(), -1);
    }*/

    uiSignal("set_profiler_frame", data);
}

bool VRGuiMonitor::on_button() {
    //int state = 1;
    //if (event->type == GDK_BUTTON_PRESS) state = 0;

    /*GtkAllocation rect;
    gtk_widget_get_allocation(darea, &rect);
    float w = rect.width;
    float x = 1.0 - event->x/w;

    if (event->y > 40) return true;
    if (event->y >= 20) selFrame = ceil(10*x);
    else selFrameRange = 10*ceil(10*x);*/

	selectFrame();
	return true;
}

void VRGuiMonitor::select_fkt() { // TODO
    /*string selection = VRGuiTreeView("treeview15").getSelectedStringValue(0);
    if (selection == selRow) return;
    selRow = selection;*/
    redraw(); // TODO: breaks row selection -> focus?
}

void VRGuiMonitor::draw_text(string txt, int x, int y) {
    /*PangoFontDescription* font = pango_font_description_new();
    pango_font_description_set_family(font, "Monospace");
    pango_font_description_set_weight(font, PANGO_WEIGHT_BOLD);
    PangoLayout* layout = gtk_widget_create_pango_layout(darea, txt.c_str());
    pango_layout_set_font_description(layout, font);
    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(context, x-tw*0.5, y-th);
    pango_cairo_show_layout(context, layout);*/
}

void VRGuiMonitor::draw_frame(int i, float w, float h, float x, int h0, bool fill) {
    /*cairo_set_line_width(context, 1.0);
    if (!fill) cairo_set_source_rgb(context, 0, 0.5, 0.9);
    else cairo_set_source_rgb(context, 0.75, 0.9, 1.0);

    float w2 = w*0.5;
    cairo_move_to(context, x+w2, h0);
    cairo_line_to(context, x-w2, h0);
    cairo_line_to(context, x-w2, h0+h);
    cairo_line_to(context, x+w2, h0+h);
    if (fill) cairo_fill(context);
    else cairo_stroke(context);

    draw_text(toString(i), x, h0+h);*/
}

void VRGuiMonitor::draw_timeline(int N0, int N1, int DN, int w, int h, int h0, int selection) {
    /*int N = N1-N0;
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
    cairo_stroke(context);*/
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

    /*cairo_set_line_width(context, 0.5);
    if (name == selRow) cairo_set_line_width(context, 1.5);
    cairo_set_source_rgb(context, c[0], c[1], c[2]);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    cairo_rectangle(context, x0,y0,w,h);
    cairo_stroke(context);*/
}

string VRGuiMonitor::toHex(Vec3d color) {
    stringstream ss;
    Vec3d co = color*255;
    int hcol = (int(co[0]) << 16) + (int(co[1]) << 8) + int(co[2]);
    ss << hex << uppercase << setw(6) << setfill('0') << hcol;
    return "#"+ss.str();
}

void VRGuiMonitor::redraw() {
    /*GdkWindow* win = gtk_widget_get_window(darea);
    if (win) gdk_window_invalidate_rect( win, NULL, false);*/
}

bool VRGuiMonitor::draw() {
    return true;
}

void VRGuiMonitor::selectFrame() {
    // get frame
    /*int f = selFrameRange - 10 + selFrame;
    //f = VRGlobals::CURRENT_FRAME -f -1;
    frame = VRProfiler::get()->getFrame(f);

    struct fktTime {
        float T = 0;
        float C = 0;
    };

    map<string, fktTime> fkts;
    for (auto itr : frame.calls) {
        auto call = itr.second;
        //if (fkts.count(call.name) == 0) fkts[call.name] = 0;
        if (call.t0 == 0) call.t0 = call.t1;
        if (call.t1 == 0) call.t1 = call.t0;
        fkts[call.name].T += call.t1 - call.t0;
        if (call.cpu1 != 0) fkts[call.name].C += call.cpu1 - call.cpu0;
    }

    uint fT = frame.t1 - frame.t0;*/

    // update list TODO
    /*GtkListStore* store = (GtkListStore*)VRGuiBuilder::get()->get_object("prof_fkts");
    gtk_list_store_clear(store);
    for (auto c : fkts) {
        string col = toHex( getColor(c.first) );
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);

        uint T = c.second.T;
        uint CPU1 = c.second.C / c.second.T * 100;
        uint CPU2 = c.second.C / fT * 100;
        gtk_list_store_set(store, &iter, 0, c.first.c_str(), -1);
        gtk_list_store_set(store, &iter, 1, T, -1);
        gtk_list_store_set(store, &iter, 2, CPU1, -1);
        gtk_list_store_set(store, &iter, 3, CPU2, -1);
        gtk_list_store_set(store, &iter, 4, col.c_str(), -1);
    }

    setLabel("Nframe", toString(frame.fID));
    setLabel("Tframe", toString(fT/1000.0)+"ms");
    setLabel("Nchanges", toString(frame.Nchanged));
    setLabel("Ncreated", toString(frame.Ncreated));*/

    redraw();
}

OSG_END_NAMESPACE;
