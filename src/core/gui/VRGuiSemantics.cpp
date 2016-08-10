
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
#include <gtkmm/toolbutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/combobox.h>
#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/frame.h>
#include <gtkmm/expander.h>
#include <gtkmm/targetentry.h>
#include "core/scene/VRScene.h"

/** TODO:

- add core/utils/VRGraphLayout
    - 3D bounding boxes and connectors
    - 3D layout algorithms

- add an auto layout button for the concept visualisation
- add new concept button, extrude from existing concepts
- add concept renaming
- add new property button

- visualise entities
- link concepts to parent concepts
- link entities to concepts
- link object properties of entites to property entity
- link object properties of concept to property concept

- store user changes to build in SBBs
- store user SBBs

IDEAS:
- instead of connectors use color highlights:
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

class VRGuiSemantics_PropsColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSemantics_PropsColumns() { add(name); add(type); }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
};

void VRGuiSemantics_on_drag_data_get(const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& data, unsigned int info, unsigned int time, Gtk::Widget* e) {
    data.set("concept", 0, (const guint8*)&e, sizeof(void*));
}

VRGuiSemantics::ConceptWidget::ConceptWidget(VRConceptPtr concept) {
    this->concept = concept;

    // widget elements
    VRGuiSemantics_PropsColumns cols;
    auto liststore = Gtk::ListStore::create(cols);
    treeview = Gtk::manage( new Gtk::TreeView() );
    treeview->set_model(liststore);
    treeview->append_column(" Properties:", cols.name);
    treeview->append_column("", cols.type);

    for (auto p : concept->properties) {
        Gtk::ListStore::Row row = *liststore->append();
        gtk_list_store_set (liststore->gobj(), row.gobj(), 0, p.second->name.c_str(), -1);
        gtk_list_store_set (liststore->gobj(), row.gobj(), 1, p.second->type.c_str(), -1);
    }

    auto e = Gtk::manage( new Gtk::Expander( concept->name ) );
    label = (Gtk::Label*)e->get_label_widget();
    e->add(*treeview);
    auto f = Gtk::manage( new Gtk::Frame() );
    f->add(*e);
    widget = f;

    // signals
    treeview->signal_cursor_changed().connect( sigc::mem_fun(*this, &VRGuiSemantics::ConceptWidget::on_select_property) );

    // dnd
    vector<Gtk::TargetEntry> entries;
    entries.push_back(Gtk::TargetEntry("concept", Gtk::TARGET_SAME_APP));
    e->drag_source_set(entries, Gdk::BUTTON1_MASK, Gdk::ACTION_MOVE);
    e->signal_drag_data_get().connect( sigc::bind<Gtk::Widget*>( sigc::ptr_fun(VRGuiSemantics_on_drag_data_get), f ) );
}

void VRGuiSemantics::ConceptWidget::on_select() {

}

void VRGuiSemantics::ConceptWidget::on_select_property() {
    Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
    if (!iter) return;

    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );

    // get selection
    VRGuiSemantics_PropsColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    string type = row.get_value(cols.type);

    // reset all rows to black
    liststore->clear();
    for (auto p : concept->properties) {
        Gtk::ListStore::Row row = *liststore->append();
        gtk_list_store_set (liststore->gobj(), row.gobj(), 0, p.second->name.c_str(), -1);
        gtk_list_store_set (liststore->gobj(), row.gobj(), 1, p.second->type.c_str(), -1);
    }

    // highlight row in green
    name = "<span color=green>" + name + "</span>";
    type = "<span color=green>" + type + "</span>";

    gtk_list_store_set (liststore->gobj(), row.gobj(), 0, name.c_str(), -1);
    gtk_list_store_set (liststore->gobj(), row.gobj(), 1, type.c_str(), -1);
}

void VRGuiSemantics::on_new_clicked() {
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    update();
}

void VRGuiSemantics::on_del_clicked() {
    /*Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview9"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    VRGuiSemantics_SocketCols cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);

    string msg1 = "Delete socket " + name;
    if (!askUser(msg1, "Are you sure you want to delete this socket?")) return;
    auto scene = VRSceneManager::getCurrent();
    if (scene) scene->remSocket(name);

    Glib::RefPtr<Gtk::ListStore> list_store  = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("Sockets"));
    list_store->erase(iter);

    Gtk::ToolButton* b;
    VRGuiBuilder()->get_widget("toolbutton15", b);
    b->set_sensitive(false);*/
}

void VRGuiSemantics::clearCanvas() {
    for (auto c : canvas->get_children()) canvas->remove(*c);
    canvas->show_all();
}

void VRGuiSemantics::drawCanvas(string name) {
    auto onto = VROntology::library[name];

    int i = 0;
    function<void(VRConceptPtr,int)> travConcepts = [&](VRConceptPtr c, int lvl) {
        auto cw = ConceptWidget(c);
        concepts[c->name] = cw;
        canvas->put(*cw.widget, 10+lvl*30, 10+25*i);
        i++;
        for (auto ci : c->children) travConcepts(ci.second, lvl+1);
    };

    travConcepts(onto->thing, 0);
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

    drawCanvas(name);
}

void VRGuiSemantics_on_name_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_name, gpointer d) {
    /*Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview9"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // get selected socket
    VRGuiSemantics_SocketCols cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    row[cols.name] = new_name;

    // update key in map
    auto scene = VRSceneManager::getCurrent();
    if (scene) scene->changeSocketName(name, new_name);*/
}

void VRGuiSemantics_on_notebook_switched(GtkNotebook* notebook, GtkNotebookPage* page, guint pageN, gpointer data) {
    //if (pageN == 4) update();
}

void VRGuiSemantics_on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& data, guint info, guint time, Gtk::Fixed* canvas) {
    Gtk::Widget* e = *(Gtk::Widget**)data.get_data();
    if (data.get_target() != "concept") { cout << "VRGuiSemantics_on_drag_data_received, wrong dnd: " << data.get_target() << endl; return; }
    canvas->move(*e, x, y);
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
    canvas->signal_drag_data_received().connect( sigc::bind<Gtk::Fixed*>( sigc::ptr_fun(VRGuiSemantics_on_drag_data_received), canvas ) );
}

void VRGuiSemantics::update() {
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    // update script list
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("onto_list"));
    store->clear();

    for (auto o : VROntology::library) {
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set(store->gobj(), row.gobj(), 0, o.first.c_str(), -1);
        gtk_list_store_set(store->gobj(), row.gobj(), 1, "built-in", -1);
    }
}

OSG_END_NAMESPACE;
