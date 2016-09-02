
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
}

void VRGuiSemantics::on_del_clicked() {
    auto mgr = getManager();
    if (!mgr) return;
    mgr->remOntology(current);
    clearCanvas();
    updateOntoList();
}

void VRGuiSemantics::on_diag_load_clicked() {
    auto mgr = getManager();
    if (!mgr) return;
    string path = VRGuiFile::getPath();
    auto o = mgr->loadOntology(path);
    o->setPersistency(666);
    o->setFlag("custom");
    updateOntoList();
}

void VRGuiSemantics::on_open_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRGuiSemantics::on_diag_load_clicked) );
    VRGuiFile::gotoPath( g_get_home_dir() );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Ontology", 2, "*.owl");
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::open( "Load", Gtk::FILE_CHOOSER_ACTION_OPEN, "Load ontology" );
}

void VRGuiSemantics::clearCanvas() {
    for (auto c : canvas->get_children()) canvas->remove(*c);
    canvas->show_all();
}

void VRGuiSemantics::updateLayout() {
    VRGraphLayout layout;
    layout.setAlgorithm(VRGraphLayout::SPRINGS, 0);
    layout.setAlgorithm(VRGraphLayout::OCCUPANCYMAP, 1);
    VRGraphLayout::layout& g = layout.getGraph();
    layout.setGravity(Vec3f(0,1,0));
    layout.setRadius(20);

    map<int, int> widgetIDs;

    for (auto c : widgets) { // build up graph nodes
        widgetIDs[c.first] = g.addNode( c.second->toGraphLayoutNode() );
        if (c.first == current->thing->ID) layout.fixNode(widgetIDs[c.first]);
    }

    for (auto w : widgets) { // add graph edges
        VRConceptWidgetPtr c = dynamic_pointer_cast<VRConceptWidget>(w.second);
        VREntityWidgetPtr e = dynamic_pointer_cast<VREntityWidget>(w.second);
        VRRuleWidgetPtr r = dynamic_pointer_cast<VRRuleWidget>(w.second);

        if (c) {
            for (auto c2 : c->concept->children) { // parent child connection
                g.connect(widgetIDs[c->concept->ID], widgetIDs[c2.second->ID], VRGraphLayout::layout::HIERARCHY);
            }
        }

        if (e) {
            for (auto c : e->entity->getConcepts()) {
                g.connect(widgetIDs[c->ID], widgetIDs[w.first], VRGraphLayout::layout::HIERARCHY);
            }
        }

        if (r) {
            if (auto c = current->getConcept( r->rule->associatedConcept) )
                g.connect(widgetIDs[c->ID], widgetIDs[w.first], VRGraphLayout::layout::HIERARCHY);
        }
    }

    layout.compute(1, 0.002);

    int i = 0;
    for (auto c : widgets) { // update widget positions
        Vec3f p = g.getElement(i).bb.center();
        c.second->move(Vec2f(p[0], p[1]));
        i++;
    }

    for (auto c : connectors) c.second->update(); // update connectors
    canvas->show_all();
}

void VRGuiSemantics::setOntology(string name) {
    auto mgr = getManager();
    if (mgr == 0) return;
    current = mgr->getOntology(name);
    updateCanvas();
}

void VRGuiSemantics::updateCanvas() {
    int i = 0;
    widgets.clear();
    connectors.clear();

    function<void(VRConceptPtr,int,int,VRConceptWidgetPtr)> travConcepts = [&](VRConceptPtr c, int cID, int lvl, VRConceptWidgetPtr cp) {
        auto cw = VRConceptWidgetPtr( new VRConceptWidget(this, canvas, c) );
        widgets[c->ID] = cw;

        int x = 150+cID*60;
        int y = 150+40*lvl;
        cw->move(Vec2f(x,y));
        if (lvl > 0) connect(cp, cw, "#00CCFF");

        i++;
        int child_i = 0;
        for (auto ci : c->children) { travConcepts(ci.second, child_i, lvl+1, cw); child_i++; }
    };

    if (current) {
        travConcepts(current->thing, 0, 0, 0);

        for (auto e : current->instances) {
            auto ew = VREntityWidgetPtr( new VREntityWidget(this, canvas, e.second) );
            widgets[ew->ID()] = ew;
            ew->move(Vec2f(150,150));
            for ( auto c : e.second->getConcepts() ) connect(widgets[c->ID], ew, "#FFEE00");
        }

        for (auto r : current->rules) {
            auto rw = VRRuleWidgetPtr( new VRRuleWidget(this, canvas, r.second) );
            widgets[rw->ID()] = rw;
            rw->move(Vec2f(150,150));
            if (auto c = current->getConcept( r.second->associatedConcept) )
                connect(widgets[c->ID], rw, "#00DD00");
        }
    }

    canvas->show_all();
}

void VRGuiSemantics::connect(VRSemanticWidgetPtr w1, VRSemanticWidgetPtr w2, string color) {
    auto co = VRConnectorWidgetPtr( new VRConnectorWidget(canvas, color) );
    connectors[w2->ID()] = co;
    co->set(w1, w2);
}

void VRGuiSemantics::disconnect(VRSemanticWidgetPtr w1, VRSemanticWidgetPtr w2) {
    if (!connectors.count(w2->ID())) return;
    auto co = connectors[w2->ID()];
    if (co->w1.lock() == w1 && co->w2.lock() == w2) connectors.erase(w2->ID());
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

    // get selected socket
    VRGuiSemantics_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    row[cols.name] = new_name;
    auto scene = VRSceneManager::getCurrent();
    if (scene) return scene->getSemanticManager()->renameOntology(name, new_name);
}

VRSemanticManagerPtr VRGuiSemantics::getManager() {
    auto scene = VRSceneManager::getCurrent();
    if (scene) return scene->getSemanticManager();
    return 0;
}

void VRGuiSemantics_on_notebook_switched(GtkNotebook* notebook, GtkNotebookPage* page, guint pageN, gpointer data) {
    //if (pageN == 4) update();
}

void VRGuiSemantics_on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& data, guint info, guint time, VRGuiSemantics* self) {
    if (data.get_target() != "concept") { cout << "VRGuiSemantics_on_drag_data_received, wrong dnd: " << data.get_target() << endl; return; }
    VRConceptWidget* e = *(VRConceptWidget**)data.get_data();
    e->move(Vec2f(x,y));
}

VRGuiSemantics::VRGuiSemantics() {
    VRGuiBuilder()->get_widget("onto_visu", canvas);
    setToolButtonCallback("toolbutton14", sigc::mem_fun(*this, &VRGuiSemantics::on_new_clicked));
    setToolButtonCallback("toolbutton15", sigc::mem_fun(*this, &VRGuiSemantics::on_del_clicked));
    setToolButtonCallback("toolbutton2", sigc::mem_fun(*this, &VRGuiSemantics::on_open_clicked));
    setTreeviewSelectCallback("treeview16", sigc::mem_fun(*this, &VRGuiSemantics::on_treeview_select) );
    setCellRendererCallback("cellrenderertext51", VRGuiSemantics_on_name_edited);
    setNoteBookCallback("notebook3", VRGuiSemantics_on_notebook_switched);
    setToolButtonSensitivity("toolbutton15", false);
    setButtonCallback("button33", sigc::mem_fun(*this, &VRGuiSemantics::on_query_clicked));

    // dnd
    vector<Gtk::TargetEntry> entries;
    entries.push_back(Gtk::TargetEntry("concept", Gtk::TARGET_SAME_APP));
    canvas->drag_dest_set(entries, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_MOVE);
    canvas->signal_drag_data_received().connect( sigc::bind<VRGuiSemantics*>( sigc::ptr_fun(VRGuiSemantics_on_drag_data_received), this ) );

    // layout update cb
    auto sm = VRSceneManager::get();
    updateLayoutCb = VRFunction<int>::create("layout_update", boost::bind(&VRGuiSemantics::updateLayout, this));
    sm->addUpdateFkt(updateLayoutCb);
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
    canvas->show_all();
}

void VRGuiSemantics::addEntity(VRConceptWidget* w) {
    auto c = current->addInstance(w->concept->getName() + "_entity", w->concept->getName());
    auto cw = VREntityWidgetPtr( new VREntityWidget(this, canvas, c) );
    widgets[c->ID] = cw;
    cw->move(w->pos + Vec2f(90,0));
    connect(widgets[w->ID()], cw, "#FFEE00");
    canvas->show_all();
}

void VRGuiSemantics::addRule(VRConceptWidget* w) {
    string n = w->concept->getName();
    auto c = current->addRule("q(x):"+n+"(x)", n);
    auto cw = VRRuleWidgetPtr( new VRRuleWidget(this, canvas, c) );
    widgets[c->ID] = cw;
    cw->move(w->pos + Vec2f(90,0));
    connect(widgets[w->ID()], cw, "#00DD00");
    canvas->show_all();
}

void VRGuiSemantics::remConcept(VRConceptWidget* w) {
    widgets.erase(w->concept->ID);
    current->remConcept(w->concept);
    //disconnectAny(); // TODO
}

void VRGuiSemantics::remEntity(VREntityWidget* w) {
    widgets.erase(w->entity->ID);
    current->remEntity(w->entity);
    //disconnectAny(); // TODO
}

void VRGuiSemantics::remRule(VRRuleWidget* w) {
    widgets.erase(w->rule->ID);
    current->remRule(w->rule);
}


VROntologyPtr VRGuiSemantics::getSelectedOntology() { return current; }

OSG_END_NAMESPACE;
