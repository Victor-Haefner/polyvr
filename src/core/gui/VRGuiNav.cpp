#include "VRGuiNav.h"

#include <iostream>
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/objects/VRCamera.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
#include "VRGuiSignals.h"

#include "wrapper/VRGuiTreeView.h"

#include <gtk/gtk.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


// --------------------------
// ---------SIGNALS----------
// --------------------------

GtkListStore* navBindings_store = 0;

void VRGuiNav::on_preset_changed() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    auto preset = scene->getNavigation(getComboboxText("combobox5"));
    if (preset == 0) return;

    //get binding type liststore
    //GtkListStore* store = (GtkListStore*)VRGuiBuilder::get()->get_object("binding_types");

    //TODO: get all bindings from preset && update nav_bindings
    gtk_list_store_clear(navBindings_store);
    for (auto& b : *preset) {
        string cb_name;
        if (b.cb) cb_name = b.cb->name;

        string type = "Event";
        if (b.doRepeat) type = "State";

        GtkTreeIter row;
        gtk_list_store_append(navBindings_store, &row);
        gtk_list_store_set(navBindings_store, &row, 0, b.key, -1);
        gtk_list_store_set(navBindings_store, &row, 1, b.state, -1);
        gtk_list_store_set(navBindings_store, &row, 2, type.c_str(), -1);
        gtk_list_store_set(navBindings_store, &row, 3, cb_name.c_str(), -1);
        gtk_list_store_set(navBindings_store, &row, 4, NULL, -1);
    }
}


void VRGuiNav::on_new_preset_clicked() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    auto preset = VRNavPreset::create();
    scene->addNavigation(preset);
    preset->setTarget(scene->getActiveCamera());

    GtkListStore* store = (GtkListStore*)VRGuiBuilder::get()->get_object("nav_presets");
    GtkTreeIter row;
    gtk_list_store_append(store, &row);
    gtk_list_store_set(store, &row, 0, preset->getName().c_str(), -1);

    setComboboxLastActive("combobox5");
}

void VRGuiNav::on_del_preset_clicked() {
    string preset = getComboboxText("combobox5");
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->remNavigation(preset);
    eraseComboboxActive("combobox5");
}

void VRGuiNav::on_new_binding_clicked() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    auto preset = scene->getNavigation(getComboboxText("combobox5"));
    VRDeviceCbPtr fkt;
    //cb = VRFunction<VRDeviceWeakPtr>::create( bind(&VRNavigator::sandBoxNavigation, this, _1) ); //TODO
    VRNavBinding binding(fkt, 0, 0, false);
    preset->addKeyBinding(binding);

    on_preset_changed();// update bindings in treeview
}

void VRGuiNav::on_del_binding_clicked() { // TODO
    ;
}

void VRGuiNav_on_keybinding_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer d) {
    VRGuiTreeView("treeview4").setSelectedStringValue(0, new_text);

    // do something
    int i = toInt(path_string);
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    string pname = getComboboxText("combobox5");
    auto preset = scene->getNavigation(pname);
    if (!preset) { cout << "VRGuiNav_on_keybinding_edited preset not found: " << pname << endl; return; }
    preset->getBindings()[i].key = toInt(new_text);
    preset->updateBinding(preset->getBindings()[i]);
}

void VRGuiNav_on_statebinding_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer d) {
    VRGuiTreeView("treeview4").setSelectedStringValue(1, new_text);

    // do something
    int i = toInt(path_string);
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    auto preset = scene->getNavigation(getComboboxText("combobox5"));
    preset->getBindings()[i].state = toInt(new_text);
    preset->updateBinding(preset->getBindings()[i]);
}

void VRGuiNav_on_typebinding_changed(const char* path_string, GtkTreeIter* new_iter) {
    auto combo_list = VRGuiBuilder::get()->get_object("binding_types");
    gchar* new_text;
    gtk_tree_model_get((GtkTreeModel*)combo_list, new_iter, 0, &new_text, -1);
    VRGuiTreeView("treeview4").setSelectedStringValue(2, new_text);

    // do something
    string type = new_text;
    int i = toInt(path_string);
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    auto preset = scene->getNavigation(getComboboxText("combobox5"));
    if (type == "Event") preset->getBindings()[i].doRepeat = false;
    if (type == "State") preset->getBindings()[i].doRepeat = true;
    preset->updateBinding(preset->getBindings()[i]);
}

void VRGuiNav_on_cbbinding_changed(const char* path_string, GtkTreeIter* new_iter) {
    auto combo_list = VRGuiBuilder::get()->get_object("binding_callbacks");
    gchar* new_text;
    gtk_tree_model_get((GtkTreeModel*)combo_list, new_iter, 0, &new_text, -1);
    VRGuiTreeView("treeview4").setSelectedStringValue(3, new_text);

    // do something
    string cb = new_text;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    VRDeviceCbPtr cback = scene->getNavigationCallbacks()[cb];
    int i = toInt(path_string);
    auto preset = scene->getNavigation(getComboboxText("combobox5"));
    auto& b = preset->getBindings()[i];
    b.clearSignal();
    b.cb = cback;
    preset->updateBinding(b);
}

// --------------------------
// ---------Main-------------
// --------------------------

VRGuiNav::VRGuiNav() {
    navBindings_store = (GtkListStore*)(VRGuiBuilder::get()->get_object("nav_bindings"));
    setComboboxCallback("combobox5", bind(&VRGuiNav::on_preset_changed, this) );

    setButtonCallback("button2", bind(&VRGuiNav::on_new_preset_clicked, this) );
    setButtonCallback("button7", bind(&VRGuiNav::on_del_preset_clicked, this) );

    setButtonCallback("button5", bind(&VRGuiNav::on_new_binding_clicked, this) );
    setButtonCallback("button8", bind(&VRGuiNav::on_del_binding_clicked, this) );

    GtkCellRendererText* crt = (GtkCellRendererText*)VRGuiBuilder::get()->get_widget("cellrenderertext11");
    g_signal_connect(crt, "edited", G_CALLBACK(VRGuiNav_on_keybinding_edited), NULL);

    crt = (GtkCellRendererText*)VRGuiBuilder::get()->get_widget("cellrenderertext12");
    g_signal_connect(crt, "edited", G_CALLBACK(VRGuiNav_on_statebinding_edited), NULL);

    setCellRendererCombo("treeviewcolumn12", "binding_types", 2, bind(VRGuiNav_on_typebinding_changed, placeholders::_1, placeholders::_2));
    setCellRendererCombo("treeviewcolumn13", "binding_callbacks", 3, bind(VRGuiNav_on_cbbinding_changed, placeholders::_1, placeholders::_2));
}

// scene updated, get cameras && nav presets
void VRGuiNav::update() {
	cout << "VRGuiNav::update" << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    vector<string> callbacks; // get navigator callback library!
    for (auto cb : scene->getNavigationCallbacks()) callbacks.push_back(cb.first);
    fillStringListstore("binding_callbacks", callbacks);

    setComboboxLastActive("combobox5");
    setCombobox("combobox5", getListStorePos( "nav_presets", scene->getActiveNavigation() ) );
}

OSG_END_NAMESPACE;
