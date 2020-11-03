#include <gtk/gtk.h>
#include "VRGuiSemantics.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
#include "VRGuiFile.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "core/utils/toString.h"
#include "core/math/graph.h"

#include "core/scene/VRScene.h"
#include "core/scene/VRSemanticManager.h"
#include "addons/Algorithms/VRGraphLayout.h"

#include "widgets/VRSemanticWidget.h"
#include "widgets/VRConceptWidget.h"
#include "widgets/VREntityWidget.h"
#include "widgets/VRRuleWidget.h"
#include "widgets/VRConnectorWidget.h"

#include "wrapper/VRGuiTreeView.h"


// TODOd, refactoring from gtk mm

/** TODO:

- VRGraphLayout
    - 3D bounding boxes, especially when opening a concept widget

- add entities
- add rules

- fix connectors
    - add connector anchor class
        - fix connector type depending on anchor normal
    - link entities to concepts
    - link object properties of entites to property entity
    - link object properties of concept to property concept

- dnd concept onto other concept for reparenting concept!

- ontology bugs
    - inherited properties not shown??

IDEAS:
- use color highlights:
    - click on a concept -> all parent concept names go red
                         -> all child concepts go blue
    - click on a obj property -> concept name goes green

*/

OSG_BEGIN_NAMESPACE;
using namespace std;

/*class VRGuiSemantics_ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSemantics_ModelColumns() { add(name); add(type); }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
};*/

void VRGuiSemantics::on_new_clicked() {
    /*auto mgr = getManager();
    if (!mgr) return;

    string name = "new_concept";
    int i=0;
    do {
        i++;
        name = "new_concept_" + toString(i);
    } while(mgr->getOntology(name));

    auto o = mgr->addOntology(name);
    o->setPersistency(666);
    o->setFlag("custom");
    updateOntoList();
    saveScene();*/
}

void VRGuiSemantics::on_del_clicked() {
    /*auto mgr = getManager();
    if (!mgr) return;
    mgr->remOntology(current);
    clearCanvas();
    updateOntoList();
    saveScene();*/
}

void VRGuiSemantics::on_diag_load_clicked() {
    /*auto mgr = getManager();
    if (!mgr) return;
    string path = VRGuiFile::getPath();
    auto o = mgr->loadOntology(path);
    o->setPersistency(666);
    o->setFlag("custom");
    updateOntoList();
    saveScene();*/
}

void VRGuiSemantics::on_open_clicked() {
    /*VRGuiFile::setCallbacks( bind(&VRGuiSemantics::on_diag_load_clicked, this) );
    VRGuiFile::gotoPath( g_get_home_dir() );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Ontology", 1, "*.owl");
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::open( "Load", GTK_FILE_CHOOSER_ACTION_OPEN, "Load ontology" );*/
}

void VRGuiSemantics::clearCanvas() {
    /*layout->clear();
    layout_graph->clear();
    widgets.clear();
    connectors.clear();*/
}

void VRGuiSemantics::updateLayout() {
    //auto gra = dynamic_pointer_cast< graph<Graph::emptyNode*> >(layout_graph);
    /*auto gra = layout_graph;

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
    }*/
}

void VRGuiSemantics::setOntology(string name) {
    /*auto mgr = getManager();
    if (mgr == 0) return;
    current = mgr->getOntology(name);
    updateCanvas();*/
}

void VRGuiSemantics::updateCanvas() {
    /*int i = 0;
    clearCanvas();

    function<void(map<int, vector<VRConceptPtr>>&, VRConceptPtr,int,int,VRConceptWidgetPtr)> travConcepts = [&](map<int, vector<VRConceptPtr>>& cMap, VRConceptPtr c, int cID, int lvl, VRConceptWidgetPtr cp) {
        if (widgets.count(c->ID)) {
            if (cp) connect(cp, widgets[c->ID], "#00CCFF");
            return;
        }

        auto cw = VRConceptWidgetPtr( new VRConceptWidget(this, canvas, c) );
        widgets[c->ID] = cw;
        addNode(c->ID);

        int x = 150+cID*60;
        int y = 150+40*lvl;
        cw->move(Vec2d(x,y));
        if (cp) connect(cp, cw, "#00CCFF");

        i++;
        int child_i = 0;
        for (auto ci : cMap[c->ID]) { travConcepts(cMap, ci, child_i, lvl+1, cw); child_i++; }
    };

    if (current) {
        auto cMap = current->getChildrenMap();
        travConcepts(cMap, current->thing, 0, 0, 0);
        layout->fixNode( widgetIDs[current->thing->ID] );

        for (auto e : current->entities) {
            auto ew = VREntityWidgetPtr( new VREntityWidget(this, canvas, e.second) );
            widgets[ew->ID()] = ew;
            addNode(ew->ID());
            ew->move(Vec2d(150,150));
            for ( auto c : e.second->getConcepts() ) connect(widgets[c->ID], ew, "#FFEE00");
        }

        for (auto r : current->rules) {
            auto rw = VRRuleWidgetPtr( new VRRuleWidget(this, canvas, r.second) );
            widgets[rw->ID()] = rw;
            addNode(rw->ID());
            rw->move(Vec2d(150,150));
            if (auto c = current->getConcept( r.second->associatedConcept) )
                connect(widgets[c->ID], rw, "#00DD00");
        }
    }

    gtk_widget_show_all((GtkWidget*)canvas);

    for (auto w : widgets) w.second->setFolding(true);*/
}

void VRGuiSemantics::connect(VRSemanticWidgetPtr w1, VRSemanticWidgetPtr w2, string color) {
    /*auto co = VRConnectorWidgetPtr( new VRConnectorWidget(canvas, color) );
    connectors[w2->ID()][w1->ID()] = co;
    co->set(w1, w2);
    w1->children[w2.get()] = w2;
    w2->connectors[co.get()] = co;

    int sID = w2->ID();
    int pID = w1->ID();
    if (widgetIDs.count(pID)) layout_graph->connect(widgetIDs[pID], widgetIDs[sID], Graph::HIERARCHY);*/
}

void VRGuiSemantics::addNode(int sID) {
    widgetIDs[sID] = layout_graph->addNode();
}

void VRGuiSemantics::disconnect(VRSemanticWidgetPtr w1, VRSemanticWidgetPtr w2) {
    if (!connectors.count(w2->ID())) return;
    if (!connectors[w2->ID()].count(w1->ID())) return;
    auto co = connectors[w2->ID()][w1->ID()];
    if (co->w1.lock() == w1 && co->w2.lock() == w2) connectors[w2->ID()].erase(w1->ID());
    if (w1->children.count(w2.get())) w1->children.erase(w2.get());

    int sID = w2->ID();
    int pID = w1->ID();
    if (widgetIDs.count(pID)) layout_graph->disconnect(widgetIDs[pID], widgetIDs[sID]);
}

void VRGuiSemantics::disconnectAny(VRSemanticWidgetPtr w) {
    if (!connectors.count(w->ID())) return;
    connectors.erase(w->ID());
}

void VRGuiSemantics::on_treeview_select() {
    setWidgetSensitivity("toolbutton15", true);
    clearCanvas();
    VRGuiTreeView tview("treeview16");
    string name = tview.getSelectedStringValue(0);
    string type = tview.getSelectedStringValue(1);
    if (type == "section") return;
    setOntology(name);
}

void VRGuiSemantics_on_name_edited(gchar *path_string, gchar *new_name) {
    auto scene = VRScene::getCurrent();
    if (!scene) return;

    VRGuiTreeView tview("treeview16");
    string name = tview.getSelectedStringValue(0);
    auto o = scene->getSemanticManager()->renameOntology(name, new_name);
    tview.setSelectedStringValue(0, new_name);
    saveScene();
}

VRSemanticManagerPtr VRGuiSemantics::getManager() {
    auto scene = VRScene::getCurrent();
    if (scene) return scene->getSemanticManager();
    return 0;
}

void VRGuiSemantics_on_drag_data_received( GdkDragContext* context, int x, int y, GtkSelectionData* data, guint info, guint time, VRGuiSemantics* self) {
    /*GdkAtom target = gtk_selection_data_get_target(data);
    string targetName = gdk_atom_name(target);
    if (targetName != "concept") { cout << "VRGuiSemantics_on_drag_data_received, wrong dnd: " << targetName << endl; return; }
    VRSemanticWidget* e = *(VRSemanticWidget**)gtk_selection_data_get_data(data);
    e->move(Vec2d(x,y));*/
}

namespace PL = std::placeholders;

VRGuiSemantics::VRGuiSemantics() {
    canvas = (GtkFixed*)VRGuiBuilder::get()->get_widget("onto_visu");
    setToolButtonCallback("toolbutton14", bind(&VRGuiSemantics::on_new_clicked, this));
    setToolButtonCallback("toolbutton15", bind(&VRGuiSemantics::on_del_clicked, this));
    setToolButtonCallback("toolbutton2", bind(&VRGuiSemantics::on_open_clicked, this));
    setTreeviewSelectCallback("treeview16", bind(&VRGuiSemantics::on_treeview_select, this));
    setCellRendererCallback("cellrenderertext51", bind(VRGuiSemantics_on_name_edited, placeholders::_1, placeholders::_2));
    setWidgetSensitivity("toolbutton15", false);
    setButtonCallback("button33", bind(&VRGuiSemantics::on_query_clicked, this));

    // dnd canvas
    GtkTargetEntry entries[] = {{ "concept", 0, GTK_TARGET_SAME_APP }};
    gtk_drag_dest_set((GtkWidget*)canvas, GTK_DEST_DEFAULT_ALL, entries, 1, GDK_ACTION_MOVE);
    connect_signal<void, GdkDragContext*, int, int, GtkSelectionData*, guint, guint>((GtkWidget*)canvas, bind(VRGuiSemantics_on_drag_data_received, PL::_1, PL::_2, PL::_3, PL::_4, PL::_5, PL::_6, this), "drag_data_received");

    // layout update cb
    auto sm = VRSceneManager::get();
    updateLayoutCb = VRUpdateCb::create("layout_update", bind(&VRGuiSemantics::updateLayout, this));
    sm->addUpdateFkt(updateLayoutCb);

    layout = VRGraphLayout::create();
    layout->setAlgorithm(VRGraphLayout::SPRINGS, 0);
    layout->setAlgorithm(VRGraphLayout::OCCUPANCYMAP, 1);
    layout->setGravity(Vec3d(0,1,0));
    layout->setRadius(0);
    layout->setSpeed(0.7);

    layout_graph = Graph::create();
    layout->setGraph(layout_graph);
}

void VRGuiSemantics::on_query_clicked() {
    auto query = getTextEntry("entry26");
    if (!current) return;

    auto r = VRReasoner::create();
    auto res = r->process(query, current);
    cout << "res " << res.size() << endl;

}

void VRGuiSemantics::updateOntoList() {
	cout << "VRGuiSemantics::updateOntoList" << endl;
    // update script list
    auto store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("onto_list");
    gtk_tree_store_clear(store);

    auto setRow = [&](GtkTreeIter* itr, string name, string type) {
        gtk_tree_store_set(store, itr, 0, name.c_str(), -1);
        gtk_tree_store_set(store, itr, 1, type.c_str(), -1);
    };

    auto mgr = getManager();
    if (mgr == 0) return;

    GtkTreeIter itr_own, itr_lib;
    gtk_tree_store_append(store, &itr_own, NULL);
    gtk_tree_store_append(store, &itr_lib, NULL);
    setRow( &itr_own, "Custom", "section");
    setRow( &itr_lib, "Library", "section");

    for (auto o : mgr->getOntologies()) {
        GtkTreeIter itr;
        if (o->getFlag() == "built-in") gtk_tree_store_append(store, &itr, &itr_lib);
        if (o->getFlag() == "custom") gtk_tree_store_append(store, &itr, &itr_own);
        setRow( &itr, o->getName(), o->getFlag());
    }

    clearCanvas();
}

void VRGuiSemantics::copyConcept(VRConceptWidget* w) {
    /*auto c = current->addConcept(w->concept->getName() + "_derived", w->concept->getName());
    if (!c || !w) {
        cout << current->toString() << endl;
        return;
    }
    if (!c || !w) return;
    auto cw = VRConceptWidgetPtr( new VRConceptWidget(this, canvas, c) );
    widgets[c->ID] = cw;
    cw->move(w->pos + Vec2d(90,0));
    connect(widgets[w->ID()], cw, "#00CCFF");
    saveScene();*/
}

void VRGuiSemantics::addEntity(VRConceptWidget* w) {
    /*auto c = current->addEntity(w->concept->getName() + "_entity", w->concept->getName());
    auto cw = VREntityWidgetPtr( new VREntityWidget(this, canvas, c) );
    widgets[c->ID] = cw;
    cw->move(w->pos + Vec2d(90,0));
    connect(widgets[w->ID()], cw, "#FFEE00");
    saveScene();*/
}

void VRGuiSemantics::addRule(VRConceptWidget* w) {
    /*string n = w->concept->getName();
    auto c = current->addRule("q(x):"+n+"(x)", n);
    auto cw = VRRuleWidgetPtr( new VRRuleWidget(this, canvas, c) );
    widgets[c->ID] = cw;
    cw->move(w->pos + Vec2d(90,0));
    connect(widgets[w->ID()], cw, "#00DD00");
    saveScene();*/
}

void VRGuiSemantics::remConcept(VRConceptWidget* w) {
    /*widgets.erase(w->concept->ID);
    current->remConcept(w->concept);
    widgetIDs.erase(w->concept->ID);
    //disconnectAny(); // TODO
    saveScene();*/
}

void VRGuiSemantics::remEntity(VREntityWidget* w) {
    /*widgets.erase(w->entity->ID);
    current->remEntity(w->entity);
    //disconnectAny(); // TODO
    saveScene();*/
}

void VRGuiSemantics::remRule(VRRuleWidget* w) {
    /*widgets.erase(w->rule->ID);
    current->remRule(w->rule);
    saveScene();*/
}

VROntologyPtr VRGuiSemantics::getSelectedOntology() { return current; }

OSG_END_NAMESPACE;
