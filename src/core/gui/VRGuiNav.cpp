#include "VRGuiNav.h"

#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/cellrenderercombo.h>
#include <gtkmm/treeviewcolumn.h>
#include <iostream>
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/objects/VRCamera.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

Glib::RefPtr<Gtk::ListStore> navBindings_store;

class VRGuiNav_BindingColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiNav_BindingColumns() { add(key); add(state); add(type); add(callback); add(obj); }

        Gtk::TreeModelColumn<gint> key;
        Gtk::TreeModelColumn<gint> state;
        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<Glib::ustring> callback;
        Gtk::TreeModelColumn<gpointer> obj;
};

class VRGuiNav_BindingTypeColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiNav_BindingTypeColumns() { add(type); }

        Gtk::TreeModelColumn<Glib::ustring> type;
};


// --------------------------
// ---------SIGNALS----------
// --------------------------

void VRGuiNav::on_preset_changed() {
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRNavPreset* preset = scene->getNavigation(getComboboxText("combobox5"));
    if (preset == 0) return;

    //get binding type liststore
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("binding_types"));

    //TODO: get all bindings from preset && update nav_bindings
    navBindings_store->clear();
    for (auto& b : *preset) {
        string cb_name;
        if (b.cb) cb_name = b.cb->getName();

        string type = "Event";
        if (b.doRepeat) type = "State";

        Gtk::ListStore::Row row = *navBindings_store->append();
        gtk_list_store_set (navBindings_store->gobj(), row.gobj(), 0, b.key, -1);
        gtk_list_store_set (navBindings_store->gobj(), row.gobj(), 1, b.state, -1);
        gtk_list_store_set (navBindings_store->gobj(), row.gobj(), 2, type.c_str(), -1);
        gtk_list_store_set (navBindings_store->gobj(), row.gobj(), 3, cb_name.c_str(), -1);
        gtk_list_store_set (navBindings_store->gobj(), row.gobj(), 4, NULL, -1);
    }
}


void VRGuiNav::on_new_preset_clicked() {
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRNavPreset* preset = new VRNavPreset();
    scene->addNavigation(preset);
    preset->setTarget(scene->getActiveCamera());

    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("nav_presets"));
    Gtk::ListStore::Row row = *store->append();
    gtk_list_store_set (store->gobj(), row.gobj(), 0, preset->getName().c_str(), -1);

    setComboboxLastActive("combobox5");
}

void VRGuiNav::on_del_preset_clicked() {
    string preset = getComboboxText("combobox5");
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    scene->remNavigation(preset);
    eraseComboboxActive("combobox5");
}

void VRGuiNav::on_new_binding_clicked() {
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRNavPreset* preset = scene->getNavigation(getComboboxText("combobox5"));
    VRDeviceCb fkt;
    //cb = VRFunction<VRDevice*>::create( boost::bind(&VRNavigator::sandBoxNavigation, this, _1) ); //TODO
    VRNavBinding binding(fkt, 0, 0, false);
    preset->addKeyBinding(binding);

    on_preset_changed();// update bindings in treeview
}

void VRGuiNav::on_del_binding_clicked() { // TODO
    ;
}


void VRGuiNav_on_keybinding_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview4"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new key
    int key = toInt(new_text);
    VRGuiNav_BindingColumns cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.key] = key;

    // do something
    int i = toInt(path_string);
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRNavPreset* preset = scene->getNavigation(getComboboxText("combobox5"));
    preset->getBindings()[i].key = key;
    preset->updateBinding(preset->getBindings()[i]);
}

void VRGuiNav_on_statebinding_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview4"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new state
    int state = toInt(new_text);
    VRGuiNav_BindingColumns cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.state] = state;

    // do something
    int i = toInt(path_string);
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRNavPreset* preset = scene->getNavigation(getComboboxText("combobox5"));
    preset->getBindings()[i].state = state;
    preset->updateBinding(preset->getBindings()[i]);
}

void VRGuiNav_on_typebinding_changed(GtkCellRendererCombo* crc, gchar *path_string, GtkTreeIter *new_iter, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview4"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new type
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("binding_types"));
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list->gobj(), new_iter, 0, &t, -1);
    string type = string(t);
    Gtk::TreeModel::Row row = *iter;
    VRGuiNav_BindingColumns cols;
    row[cols.type] = type;

    // do something
    int i = toInt(path_string);
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRNavPreset* preset = scene->getNavigation(getComboboxText("combobox5"));
    if (type == "Event") preset->getBindings()[i].doRepeat = false;
    if (type == "State") preset->getBindings()[i].doRepeat = true;
    preset->updateBinding(preset->getBindings()[i]);
}

void VRGuiNav_on_cbbinding_changed(GtkCellRendererCombo* crc, gchar *path_string, GtkTreeIter *new_iter, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview4"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new type
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("binding_callbacks"));
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list->gobj(), new_iter, 0, &t, -1);
    string cb = string(t);
    Gtk::TreeModel::Row row = *iter;
    VRGuiNav_BindingColumns cols;
    row[cols.callback] = cb;

    // do something
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRDeviceCb cback = scene->getNavigationCallbacks()[cb];
    int i = toInt(path_string);
    VRNavPreset* preset = scene->getNavigation(getComboboxText("combobox5"));
    auto& b = preset->getBindings()[i];
    b.clearSignal();
    b.cb = cback;
    preset->updateBinding(b);
}

// --------------------------
// ---------Main-------------
// --------------------------

VRGuiNav::VRGuiNav() {
    navBindings_store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("nav_bindings"));
    setComboboxCallback("combobox5", sigc::mem_fun(*this, &VRGuiNav::on_preset_changed) );

    setButtonCallback("button2", sigc::mem_fun(*this, &VRGuiNav::on_new_preset_clicked) );
    setButtonCallback("button7", sigc::mem_fun(*this, &VRGuiNav::on_del_preset_clicked) );

    setButtonCallback("button5", sigc::mem_fun(*this, &VRGuiNav::on_new_binding_clicked) );
    setButtonCallback("button8", sigc::mem_fun(*this, &VRGuiNav::on_del_binding_clicked) );

    Glib::RefPtr<Gtk::CellRendererText> crt;
    crt = Glib::RefPtr<Gtk::CellRendererText>::cast_static(VRGuiBuilder()->get_object("cellrenderertext11"));
    g_signal_connect (crt->gobj(), "edited", G_CALLBACK (VRGuiNav_on_keybinding_edited), NULL);

    crt = Glib::RefPtr<Gtk::CellRendererText>::cast_static(VRGuiBuilder()->get_object("cellrenderertext12"));
    g_signal_connect (crt->gobj(), "edited", G_CALLBACK (VRGuiNav_on_statebinding_edited), NULL);

    VRGuiNav_BindingColumns collums;
    setCellRendererCombo("treeviewcolumn12", "binding_types", collums.type, VRGuiNav_on_typebinding_changed);
    setCellRendererCombo("treeviewcolumn13", "binding_callbacks", collums.callback, VRGuiNav_on_cbbinding_changed);
}

// scene updated, get cameras && nav presets
void VRGuiNav::update() {
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    Glib::RefPtr<Gtk::ListStore> combo_list;
    combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("binding_callbacks"));
        // get navigator callback library!
    combo_list->clear();
    for (auto cb : scene->getNavigationCallbacks()) {
        Gtk::ListStore::Row row = *combo_list->append();
        gtk_list_store_set(combo_list->gobj(), row.gobj(), 0, cb.first.c_str(), -1);
    }

    setComboboxLastActive("combobox5");
    setCombobox("combobox5", getListStorePos( "nav_presets", scene->getActiveNavigation() ) );
}

OSG_END_NAMESPACE;
