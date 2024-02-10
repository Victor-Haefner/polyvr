#include "VRWidgetsCanvas.h"
#include "VRCanvasWidget.h"
#include "VRConnectorWidget.h"

#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunction.h"

#include "addons/Algorithms/VRGraphLayout.h"

using namespace OSG;
namespace PL = std::placeholders;

/*void VRWidgetsCanvas_on_drag_data_received( GdkDragContext* context, int x, int y, GtkSelectionData* data, guint info, guint time, VRWidgetsCanvas* self) {
    GdkAtom target = gtk_selection_data_get_target(data);
    string targetName = gdk_atom_name(target);
    if (targetName != "concept") { cout << "VRWidgetsCanvas_on_drag_data_received, wrong dnd: " << targetName << endl; return; }
    VRCanvasWidget* e = *(VRCanvasWidget**)gtk_selection_data_get_data(data);
    e->move(Vec2d(x,y));
}*/

VRWidgetsCanvas::VRWidgetsCanvas(string canvasName) {
    /*canvas = (GtkFixed*)VRGuiBuilder::get()->get_widget(canvasName);

    // dnd canvas
    GtkTargetEntry entries[] = {{ "concept", 0, GTK_TARGET_SAME_APP }};
    gtk_drag_dest_set((GtkWidget*)canvas, GTK_DEST_DEFAULT_ALL, entries, 1, GDK_ACTION_MOVE);
    connect_signal<void, GdkDragContext*, int, int, GtkSelectionData*, guint, guint>((GtkWidget*)canvas, bind(VRWidgetsCanvas_on_drag_data_received, PL::_1, PL::_2, PL::_3, PL::_4, PL::_5, PL::_6, this), "drag_data_received");

    layout = VRGraphLayout::create();
    layout->setAlgorithm(VRGraphLayout::SPRINGS, 0);
    layout->setAlgorithm(VRGraphLayout::OCCUPANCYMAP, 1);
    layout->setGravity(Vec3d(0,1,0));
    layout->setRadius(0);
    layout->setSpeed(0.7);

    layout_graph = Graph::create();
    layout->setGraph(layout_graph);

    // layout update cb
    auto sm = VRSceneManager::get();
    updateLayoutCb = VRUpdateCb::create("layout_update", bind(&VRWidgetsCanvas::updateLayout, this));
    sm->addUpdateFkt(updateLayoutCb);*/
}

VRWidgetsCanvas::~VRWidgetsCanvas() {}

VRWidgetsCanvasPtr VRWidgetsCanvas::create(string canvasName) { return VRWidgetsCanvasPtr( new VRWidgetsCanvas(canvasName) ); }
VRWidgetsCanvasPtr VRWidgetsCanvas::ptr() { return static_pointer_cast<VRWidgetsCanvas>(shared_from_this()); }

//GtkFixed* VRWidgetsCanvas::getCanvas() { return canvas; }
VRGraphLayoutPtr VRWidgetsCanvas::getLayout() { return layout; }

void VRWidgetsCanvas::clear() {
    if (layout) layout->clear();
    if (layout_graph) layout_graph->clear();
    widgets.clear();
    connectors.clear();
}

void VRWidgetsCanvas::addNode(int sID) {
    widgetIDs[sID] = layout_graph->addNode();
}

void VRWidgetsCanvas::remNode(int sID) {
    widgetIDs.erase(sID);
}

int VRWidgetsCanvas::getNode(int sID) {
    if (widgetIDs.count(sID)) return widgetIDs[sID];
    return -1;
}

void VRWidgetsCanvas::addWidget(int sID, VRCanvasWidgetPtr w) {
    widgets[sID] = w;
}

void VRWidgetsCanvas::remWidget(int sID) {
    widgets.erase(sID);
}

VRCanvasWidgetPtr VRWidgetsCanvas::getWidget(int sID) {
    if (widgets.count(sID)) return widgets[sID];
    return 0;
}

void VRWidgetsCanvas::foldAll(bool b) {
    for (auto w : widgets) w.second->setFolding(b);
}

void VRWidgetsCanvas::disconnect(VRCanvasWidgetPtr w1, VRCanvasWidgetPtr w2) {
    if (!connectors.count(w2->ID())) return;
    if (!connectors[w2->ID()].count(w1->ID())) return;
    auto co = connectors[w2->ID()][w1->ID()];
    if (co->w1.lock() == w1 && co->w2.lock() == w2) connectors[w2->ID()].erase(w1->ID());
    if (w1->children.count(w2.get())) w1->children.erase(w2.get());

    int sID = w2->ID();
    int pID = w1->ID();
    if (widgetIDs.count(pID)) layout_graph->disconnect(widgetIDs[pID], widgetIDs[sID]);
}

void VRWidgetsCanvas::disconnectAny(VRCanvasWidgetPtr w) {
    if (!connectors.count(w->ID())) return;
    connectors.erase(w->ID());
}

void VRWidgetsCanvas::connect(VRCanvasWidgetPtr w1, VRCanvasWidgetPtr w2, string color) {
    auto co = VRConnectorWidgetPtr( new VRConnectorWidget(canvas, color) );
    connectors[w2->ID()][w1->ID()] = co;
    co->set(w1, w2);
    w1->children[w2.get()] = w2;
    w2->connectors[co.get()] = co;

    int sID = w2->ID();
    int pID = w1->ID();
    if (widgetIDs.count(pID)) layout_graph->connect(widgetIDs[pID], widgetIDs[sID], Graph::HIERARCHY);
}

void VRWidgetsCanvas::updateLayout() {
    auto gra = layout_graph;

    for (auto c : widgets) { // update node boxes
        if (!widgetIDs.count(c.first)) continue;
        int ID = widgetIDs[c.first];
        layout->setNodeState(ID, c.second->visible);
        if (!c.second->visible) continue;

        Vec3d s = c.second->getSize();
        s[2] = 10;

        if (ID >= gra->size()) continue;
        gra->getNode(ID).box.clear();
        gra->getNode(ID).box.update(-s);
        gra->getNode(ID).box.update(s);
        gra->getNode(ID).box.scale(1.2);
        gra->getNode(ID).box.setCenter( c.second->getPosition() );
    }

    layout->compute(2, 0.1); // N iterations, eps

    auto clamp = [&](Boundingbox& bb) {
        //float W = canvas->get_width();
        //float H = canvas->get_height();
        Vec3d s = bb.size()*0.5;
        Vec3d p = bb.center();
        if (p[0] < s[0]) p[0] = s[0];
        if (p[1] < s[1]) p[1] = s[1];
        p[2] = 0;
        bb.setCenter(p);
    };

    for (auto c : widgets) { // update widget positions
        if (!c.second->visible) continue;
        int ID = widgetIDs[c.first];
        if (ID >= gra->size()) break;
        clamp( gra->getNode(ID).box );
        Vec3d p = gra->getNode(ID).box.center();
        c.second->move(Vec2d(p[0], p[1]));
    }

    for (auto cv : connectors) {
        for (auto c : cv.second) {
            if (!c.second->visible) continue;
            c.second->update(); // update connectors
        }
    }
}

