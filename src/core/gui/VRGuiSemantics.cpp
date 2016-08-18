
#include "VRGuiSemantics.h"
#include "VRGuiUtils.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
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
#include <boost/bind.hpp>
#include "core/scene/VRScene.h"
#include "core/scene/VRSemanticManager.h"
#include "addons/Algorithms/VRGraphLayout.h"

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

VRGuiSemantics::ConnectorWidget::ConnectorWidget(Gtk::Fixed* canvas) {
    sh1 = Gtk::manage( new Gtk::HSeparator() );
    sh2 = Gtk::manage( new Gtk::HSeparator() );
    sv1 = Gtk::manage( new Gtk::VSeparator() );
    sv2 = Gtk::manage( new Gtk::VSeparator() );
    this->canvas = canvas;
    canvas->put(*sh1, 0, 0);
    canvas->put(*sh2, 0, 0);
    canvas->put(*sv1, 0, 0);
    canvas->put(*sv2, 0, 0);
}

void VRGuiSemantics::ConnectorWidget::set(VRConceptWidgetPtr w1, VRConceptWidgetPtr w2) {
    this->w1 = w1;
    this->w2 = w2;
    update();
}

void VRGuiSemantics::ConnectorWidget::update() {
    auto ws1 = w1.lock();
    auto ws2 = w2.lock();
    if (ws1 && ws2) {
        Vec2f a1 = ws1->getAnchorPoint(ws2->pos);
        Vec2f a2 = ws2->getAnchorPoint(ws1->pos);
        float x1 = a1[0];
        float x2 = a2[0];
        float y1 = a1[1];
        float y2 = a2[1];

        float w = abs(x2-x1);
        float h = abs(y2-y1);

        sh1->set_size_request(0, 0);
        sh2->set_size_request(0, 0);
        sv1->set_size_request(0, 0);
        sv2->set_size_request(0, 0);

        if (w <= 2 && h <= 2) return;

        if (h <= 2) {
            sh1->show();
            sh1->set_size_request(w, 2);
            if (x2 < x1) swap(x2,x1);
            canvas->move(*sh1, x1, y1);
            return;
        }

        if (w <= 2) {
            sv1->show();
            sv1->set_size_request(2, h);
            if (y2 < y1) swap(y2,y1);
            canvas->move(*sv1, x1, y1);
            return;
        }

        if (w < h) {
            sh1->show();
            sh2->show();
            sv1->show();
            sh1->set_size_request(w*0.5, 2);
            sh2->set_size_request(w*0.5, 2);
            sv1->set_size_request(2, h);

            if (y2 < y1 && x2 > x1) {
                swap(y1,y2);
                canvas->move(*sh1, x1, y2);
                canvas->move(*sh2, x1+w*0.5, y1);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 > y1 && x2 < x1) {
                swap(x1,x2);
                canvas->move(*sh1, x1, y2);
                canvas->move(*sh2, x1+w*0.5, y1);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 > y1 && x2 > x1) {
                canvas->move(*sh1, x1, y1);
                canvas->move(*sh2, x1+w*0.5, y2);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 < y1 && x2 < x1) {
                swap(y1,y2);
                swap(x1,x2);
                canvas->move(*sh1, x1, y1);
                canvas->move(*sh2, x1+w*0.5, y2);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            return;
        } else {
            sv1->show();
            sv2->show();
            sh1->show();
            sv1->set_size_request(2, h*0.5);
            sv2->set_size_request(2, h*0.5);
            sh1->set_size_request(w, 2);

            if (y2 < y1 && x2 > x1) {
                swap(y1,y2);
                canvas->move(*sv1, x2, y1);
                canvas->move(*sv2, x1, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 < y1 && x2 < x1) {
                swap(x1,x2);
                swap(y1,y2);
                canvas->move(*sv1, x1, y1);
                canvas->move(*sv2, x2, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 > y1 && x2 < x1) {
                swap(x1,x2);
                canvas->move(*sv1, x2, y1);
                canvas->move(*sv2, x1, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 > y1 && x2 > x1) {
                canvas->move(*sv1, x1, y1);
                canvas->move(*sv2, x2, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            return;
        }
    }
}


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
}

void VRGuiSemantics::clearCanvas() {
    for (auto c : canvas->get_children()) canvas->remove(*c);
    canvas->show_all();
}

void VRGuiSemantics::updateLayout() {
    VRGraphLayout layout;
    layout.setAlgorithm(VRGraphLayout::SPRINGS, 0);
    layout.setAlgorithm(VRGraphLayout::OCCUPANCYMAP, 1);
    graph<Vec3f>& g = layout.getGraph();
    layout.setGravity(Vec3f(0,1,0));
    layout.setRadius(100);

    map<string, int> conceptIDs;

    for (auto c : widgets) {
        Vec3f p = Vec3f(c.second->pos[0], c.second->pos[1], 0);
        conceptIDs[c.first] = g.addNode(p);
        if (c.first == "Thing") layout.fixNode(conceptIDs[c.first]);
    }

    for (auto _c : widgets) {
        VRConceptWidgetPtr c = dynamic_pointer_cast<VRConceptWidget>(_c.second);
        for (auto c2 : c->concept->children) { // parent child connection
            g.connect(conceptIDs[_c.first], conceptIDs[c2.second->getName()], graph<Vec3f>::HIERARCHY);
            for (auto c3 : c->concept->children) { // sibling connection, TODO: use occupancy map!!
                if (c2 != c3)
                    g.connect(conceptIDs[c2.second->getName()], conceptIDs[c3.second->getName()], graph<Vec3f>::SIBLING);
            }
        }
    }

    layout.compute(1, 0.002);

    int i = 0;
    for (auto c : widgets) {
        Vec3f& p = g.getNodes()[i];
        c.second->move(Vec2f(p[0], p[1]));
        i++;
    }

    for (auto c : connectors) {
        c.second->update();
    }

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

    function<void(VRConceptPtr,int,int,VRConceptWidgetPtr)> travConcepts = [&](VRConceptPtr c, int cID, int lvl, VRConceptWidgetPtr cp) {
        auto cw = VRConceptWidgetPtr( new VRConceptWidget(this, canvas, c) );
        widgets[c->getName()] = cw;

        int x = 10+cID*60;
        int y = 10+40*lvl;
        cw->move(Vec2f(x,y));

        if (lvl > 0) {
            auto co = ConnectorWidgetPtr( new ConnectorWidget(canvas) );
            connectors[c->getName()] = co;
            co->set(cp, cw);
        }

        i++;
        int child_i = 0;
        for (auto ci : c->children) { travConcepts(ci.second, child_i, lvl+1, cw); child_i++; }
    };

    if (current) travConcepts(current->thing, 0, 0, 0);
    canvas->show_all();
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
    setTreeviewSelectCallback("treeview16", sigc::mem_fun(*this, &VRGuiSemantics::on_treeview_select) );
    setCellRendererCallback("cellrenderertext51", VRGuiSemantics_on_name_edited);
    setNoteBookCallback("notebook3", VRGuiSemantics_on_notebook_switched);
    setToolButtonSensitivity("toolbutton15", false);

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
    auto cw = VRConceptWidgetPtr( new VRConceptWidget(this, canvas, c) );
    widgets[c->getName()] = cw;
    cw->move(w->pos + Vec2f(90,0));
    canvas->show_all();
}

void VRGuiSemantics::remConcept(VRConceptWidget* w) {
    widgets.erase(w->concept->getName());
    current->remConcept(w->concept);
}

VROntologyPtr VRGuiSemantics::getSelectedOntology() { return current; }

OSG_END_NAMESPACE;
