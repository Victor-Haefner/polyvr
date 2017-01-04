
#include "VRGuiSemantics.h"
#include "VRGuiUtils.h"
#include "VRGuiFile.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "core/utils/toString.h"
#include "core/math/graph.h"

#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/combobox.h>
#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/frame.h>
#include <gtkmm/expander.h>
#include <gtkmm/targetentry.h>
#include <gtkmm/separator.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <gtkmm/filechooser.h>
#include <boost/bind.hpp>
#include "core/scene/VRScene.h"
#include "core/scene/VRSemanticManager.h"
#include "addons/Algorithms/VRGraphLayout.h"

#include "widgets/VRSemanticWidget.h"
#include "widgets/VRConceptWidget.h"
#include "widgets/VREntityWidget.h"
#include "widgets/VRRuleWidget.h"
#include "widgets/VRConnectorWidget.h"
#include "core/math/graphT.h"

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

class VRGuiSemantics_ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSemantics_ModelColumns() { add(name); add(type); }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
};

void VRGuiSemantics::on_new_clicked() {
    auto mgr = getManager();
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
    saveScene();
}

void VRGuiSemantics::on_del_clicked() {
    auto mgr = getManager();
    if (!mgr) return;
    mgr->remOntology(current);
    clearCanvas();
    updateOntoList();
    saveScene();
}

void VRGuiSemantics::on_diag_load_clicked() {
    auto mgr = getManager();
    if (!mgr) return;
    string path = VRGuiFile::getPath();
    auto o = mgr->loadOntology(path);
    o->setPersistency(666);
    o->setFlag("custom");
    updateOntoList();
    saveScene();
}

void VRGuiSemantics::on_open_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRGuiSemantics::on_diag_load_clicked) );
    VRGuiFile::gotoPath( g_get_home_dir() );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Ontology", 1, "*.owl");
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::open( "Load", Gtk::FILE_CHOOSER_ACTION_OPEN, "Load ontology" );
}

void VRGuiSemantics::clearCanvas() {
    layout->clear();
    layout_graph->clear();
    widgets.clear();
    connectors.clear();
}

void VRGuiSemantics::updateLayout() {
    auto gra = dynamic_pointer_cast< graph<graph_base::emptyNode> >(layout_graph);

    for (auto c : widgets) { // update node boxes
        if (!widgetIDs.count(c.first)) continue;
        int ID = widgetIDs[c.first];
        layout->setNodeState(ID, c.second->visible);
        if (!c.second->visible) continue;

        Vec3f s = c.second->getSize();
        s[2] = 10;

        if (ID >= gra->size()) continue;
        gra->getNode(ID).box.clear();
        gra->getNode(ID).box.update(-s);
        gra->getNode(ID).box.update(s);
        gra->getNode(ID).box.scale(1.2);
        gra->getNode(ID).box.setCenter( c.second->getPosition() );
    }

    layout->compute(2, 0.1); // N iterations, eps

    auto clamp = [&](boundingbox& bb) {
        //float W = canvas->get_width();
        //float H = canvas->get_height();
        Vec3f s = bb.size()*0.5;
        Vec3f p = bb.center();
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
        Vec3f p = gra->getNode(ID).box.center();
        c.second->move(Vec2f(p[0], p[1]));
    }

    for (auto cv : connectors) {
        for (auto c : cv.second) {
            if (!c.second->visible) continue;
            c.second->update(); // update connectors
        }
    }
}

void VRGuiSemantics::setOntology(string name) {
    auto mgr = getManager();
    if (mgr == 0) return;
    current = mgr->getOntology(name);
    updateCanvas();
}

void VRGuiSemantics::updateCanvas() {
    int i = 0;
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
        cw->move(Vec2f(x,y));
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
            ew->move(Vec2f(150,150));
            for ( auto c : e.second->getConcepts() ) connect(widgets[c->ID], ew, "#FFEE00");
        }

        for (auto r : current->rules) {
            auto rw = VRRuleWidgetPtr( new VRRuleWidget(this, canvas, r.second) );
            widgets[rw->ID()] = rw;
            addNode(rw->ID());
            rw->move(Vec2f(150,150));
            if (auto c = current->getConcept( r.second->associatedConcept) )
                connect(widgets[c->ID], rw, "#00DD00");
        }
    }

    canvas->show_all();

    for (auto w : widgets) w.second->setFolding(true);
}

void VRGuiSemantics::connect(VRSemanticWidgetPtr w1, VRSemanticWidgetPtr w2, string color) {
    auto co = VRConnectorWidgetPtr( new VRConnectorWidget(canvas, color) );
    connectors[w2->ID()][w1->ID()] = co;
    co->set(w1, w2);
    w1->children[w2.get()] = w2;
    w2->connectors[co.get()] = co;

    int sID = w2->ID();
    int pID = w1->ID();
    auto gra = dynamic_pointer_cast< graph<graph_base::emptyNode> >(layout_graph);
    if (widgetIDs.count(pID)) gra->connect(widgetIDs[pID], widgetIDs[sID], graph_base::HIERARCHY);
}

void VRGuiSemantics::addNode(int sID) {
    auto gra = dynamic_pointer_cast< graph<graph_base::emptyNode> >(layout_graph);
    widgetIDs[sID] = gra->addNode();
}

void VRGuiSemantics::disconnect(VRSemanticWidgetPtr w1, VRSemanticWidgetPtr w2) {
    if (!connectors.count(w2->ID())) return;
    if (!connectors[w2->ID()].count(w1->ID())) return;
    auto co = connectors[w2->ID()][w1->ID()];
    if (co->w1.lock() == w1 && co->w2.lock() == w2) connectors[w2->ID()].erase(w1->ID());
    if (w1->children.count(w2.get())) w1->children.erase(w2.get());

    int sID = w2->ID();
    int pID = w1->ID();
    auto gra = dynamic_pointer_cast< graph<graph_base::emptyNode> >(layout_graph);
    if (widgetIDs.count(pID)) gra->disconnect(widgetIDs[pID], widgetIDs[sID]);
}

void VRGuiSemantics::disconnectAny(VRSemanticWidgetPtr w) {
    if (!connectors.count(w->ID())) return;
    connectors.erase(w->ID());
}

void VRGuiSemantics::on_treeview_select() {
    setToolButtonSensitivity("toolbutton15", true);

    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview16"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();

    clearCanvas();
    if (!iter) return;

    // get selection
    VRGuiSemantics_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    string type = row.get_value(cols.type);
    if (type == "section") return;

    setOntology(name);
}

void VRGuiSemantics_on_name_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_name, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview16"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    auto scene = VRScene::getCurrent();
    if (!scene) return;

    // get selected socket
    VRGuiSemantics_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    auto o = scene->getSemanticManager()->renameOntology(name, new_name);
    if (o) row[cols.name] = new_name;
    saveScene();
}

VRSemanticManagerPtr VRGuiSemantics::getManager() {
    auto scene = VRScene::getCurrent();
    if (scene) return scene->getSemanticManager();
    return 0;
}

void VRGuiSemantics_on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& data, guint info, guint time, VRGuiSemantics* self) {
    if (data.get_target() != "concept") { cout << "VRGuiSemantics_on_drag_data_received, wrong dnd: " << data.get_target() << endl; return; }
    VRSemanticWidget* e = *(VRSemanticWidget**)data.get_data();
    e->move(Vec2f(x,y));
}

VRGuiSemantics::VRGuiSemantics() {
    VRGuiBuilder()->get_widget("onto_visu", canvas);
    setToolButtonCallback("toolbutton14", sigc::mem_fun(*this, &VRGuiSemantics::on_new_clicked));
    setToolButtonCallback("toolbutton15", sigc::mem_fun(*this, &VRGuiSemantics::on_del_clicked));
    setToolButtonCallback("toolbutton2", sigc::mem_fun(*this, &VRGuiSemantics::on_open_clicked));
    setTreeviewSelectCallback("treeview16", sigc::mem_fun(*this, &VRGuiSemantics::on_treeview_select) );
    setCellRendererCallback("cellrenderertext51", VRGuiSemantics_on_name_edited);
    setToolButtonSensitivity("toolbutton15", false);
    setButtonCallback("button33", sigc::mem_fun(*this, &VRGuiSemantics::on_query_clicked));

    // dnd canvas
    vector<Gtk::TargetEntry> entries;
    entries.push_back(Gtk::TargetEntry("concept", Gtk::TARGET_SAME_APP));
    canvas->drag_dest_set(entries, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_MOVE);
    canvas->signal_drag_data_received().connect( sigc::bind<VRGuiSemantics*>( sigc::ptr_fun(VRGuiSemantics_on_drag_data_received), this ) );

    // layout update cb
    auto sm = VRSceneManager::get();
    updateLayoutCb = VRFunction<int>::create("layout_update", boost::bind(&VRGuiSemantics::updateLayout, this));
    sm->addUpdateFkt(updateLayoutCb);

    layout = VRGraphLayout::create();
    layout->setAlgorithm(VRGraphLayout::SPRINGS, 0);
    layout->setAlgorithm(VRGraphLayout::OCCUPANCYMAP, 1);
    layout->setGravity(Vec3f(0,1,0));
    layout->setRadius(0);
    layout->setSpeed(0.7);

    layout_graph = shared_ptr< graph<graph_base::emptyNode> >( new graph<graph_base::emptyNode>() );
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
    // update script list
    Glib::RefPtr<Gtk::TreeStore> store = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("onto_list"));
    store->clear();

    auto setRow = [&](Gtk::TreeIter itr, string name, string type) {
        Gtk::TreeStore::Row row = *itr;
        gtk_tree_store_set(store->gobj(), row.gobj(), 0, name.c_str(), -1);
        gtk_tree_store_set(store->gobj(), row.gobj(), 1, type.c_str(), -1);
    };

    auto mgr = getManager();
    if (mgr == 0) return;

    auto itr_own = store->append();
    auto itr_lib = store->append();
    setRow( itr_own, "Custom", "section");
    setRow( itr_lib, "Library", "section");

    for (auto o : mgr->getOntologies()) {
        Gtk::TreeIter itr;
        if (o->getFlag() == "built-in") itr = store->append(itr_lib->children());
        if (o->getFlag() == "custom") itr = store->append(itr_own->children());
        setRow( itr, o->getName(), o->getFlag());
    }

    clearCanvas();
}

void VRGuiSemantics::copyConcept(VRConceptWidget* w) {
    auto c = current->addConcept(w->concept->getName() + "_derived", w->concept->getName());
    if (!c || !w) {
        cout << current->toString() << endl;
        return;
    }
    if (!c || !w) return;
    auto cw = VRConceptWidgetPtr( new VRConceptWidget(this, canvas, c) );
    widgets[c->ID] = cw;
    cw->move(w->pos + Vec2f(90,0));
    connect(widgets[w->ID()], cw, "#00CCFF");
    saveScene();
}

void VRGuiSemantics::addEntity(VRConceptWidget* w) {
    auto c = current->addEntity(w->concept->getName() + "_entity", w->concept->getName());
    auto cw = VREntityWidgetPtr( new VREntityWidget(this, canvas, c) );
    widgets[c->ID] = cw;
    cw->move(w->pos + Vec2f(90,0));
    connect(widgets[w->ID()], cw, "#FFEE00");
    saveScene();
}

void VRGuiSemantics::addRule(VRConceptWidget* w) {
    string n = w->concept->getName();
    auto c = current->addRule("q(x):"+n+"(x)", n);
    auto cw = VRRuleWidgetPtr( new VRRuleWidget(this, canvas, c) );
    widgets[c->ID] = cw;
    cw->move(w->pos + Vec2f(90,0));
    connect(widgets[w->ID()], cw, "#00DD00");
    saveScene();
}

void VRGuiSemantics::remConcept(VRConceptWidget* w) {
    widgets.erase(w->concept->ID);
    current->remConcept(w->concept);
    widgetIDs.erase(w->concept->ID);
    //disconnectAny(); // TODO
    saveScene();
}

void VRGuiSemantics::remEntity(VREntityWidget* w) {
    widgets.erase(w->entity->ID);
    current->remEntity(w->entity);
    //disconnectAny(); // TODO
    saveScene();
}

void VRGuiSemantics::remRule(VRRuleWidget* w) {
    widgets.erase(w->rule->ID);
    current->remRule(w->rule);
    saveScene();
}

VROntologyPtr VRGuiSemantics::getSelectedOntology() { return current; }

OSG_END_NAMESPACE;
