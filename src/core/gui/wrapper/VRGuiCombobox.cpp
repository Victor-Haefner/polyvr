#include "VRGuiCombobox.h"
#include "../VRGuiUtils.h"
#include "../VRGuiBuilder.h"

#include <gtk/gtk.h>

VRGuiCombobox::VRGuiCombobox(string name) : VRGuiWidget(name) {
    auto widget = VRGuiBuilder::get()->get_widget(name);
    init(widget);
}

VRGuiCombobox::VRGuiCombobox(_GtkWidget* widget) : VRGuiWidget(widget) {
    init(widget);
}

VRGuiCombobox::~VRGuiCombobox() {
    if (selection) delete selection;
}

void VRGuiCombobox::init(_GtkWidget* widget) {
    combobox = (GtkComboBox*)widget;
    tree_model = (GtkTreeModel*)gtk_combo_box_get_model(combobox);
    selection = new GtkTreeIter();
}

bool VRGuiCombobox::hasSelection() {
    updateSelection();
    return (selection >= 0);
}

void VRGuiCombobox::updateSelection() {
    gtk_combo_box_get_active_iter(combobox, selection);
}

void VRGuiCombobox::selectRow(int i) {
    gtk_combo_box_set_active(combobox, i);
    grabFocus();
}

void VRGuiCombobox::setValue(GtkTreeIter* itr, int column, void* data) {
    gtk_list_store_set((GtkListStore*)tree_model, itr, column, data, -1);
}

void VRGuiCombobox::setStringValue(GtkTreeIter* itr, int column, string data) {
    setValue(itr, column, (void*)data.c_str());
}

void* VRGuiCombobox::getValue(GtkTreeIter* itr, int column) {
    void* data = 0;
    gtk_tree_model_get(tree_model, itr, column, &data, -1);
    return data;
}

string VRGuiCombobox::getStringValue(GtkTreeIter* itr, int column) {
    char* v = (char*)getValue(itr, column);
    return v?v:"";
}

void VRGuiCombobox::setSelectedValue(int column, void* data) {
    updateSelection();
    GtkTreeIter itr;
    gtk_combo_box_get_active_iter(combobox, &itr);
    if (hasSelection()) setValue(&itr, column, data);
}

void VRGuiCombobox::setSelectedStringValue(int column, string data) {
    setSelectedValue(column, (void*)data.c_str());
}

void* VRGuiCombobox::getSelectedValue(int column) {
    updateSelection();
    if (hasSelection()) return getValue(selection, column);
    else return 0;
}

string VRGuiCombobox::getSelectedStringValue(int column) {
    char* v = (char*)getSelectedValue(column);
    return v?v:"";
}

int VRGuiCombobox::getSelectedIntValue(int column) {
    int* p = (int*)getSelectedValue(column);
    return p?*p:0;
}

void VRGuiCombobox::removeSelected() {
    updateSelection();
    if (hasSelection()) removeRow(selection);
}

void VRGuiCombobox::removeRow(GtkTreeIter* itr) {
    gtk_list_store_remove((GtkListStore*)tree_model, itr);
}


