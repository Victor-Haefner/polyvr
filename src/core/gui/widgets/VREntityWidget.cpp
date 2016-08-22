#include "VREntityWidget.h"

#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include "../VRGuiUtils.h"
#include "../VRGuiSemantics.h"

#include "core/utils/toString.h"

#include <gtkmm/label.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/dialog.h>
#include <gtkmm/builder.h>

using namespace OSG;

VREntityWidget::VREntityWidget(VRGuiSemantics* m, Gtk::Fixed* canvas, VREntityPtr entity) : VRSemanticWidget(m, canvas, "#FFEE00") {
    this->entity = entity;
    label->set_text(entity->getName());

    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );
    for (auto pv : entity->properties) {
        for (auto p : pv.second) {
            setPropRow(liststore->append(), p->getName(), p->type, "black", 0);
        }
    }
}

int VREntityWidget::ID() { return entity->ID; }

void VREntityWidget::on_edit_prop_clicked() {
    if (!selected_property) return;
    Gtk::Dialog* dialog;
    VRGuiBuilder()->get_widget("PropertyEdit", dialog);
    setTextEntry("entry23", selected_property->getName());
    setTextEntry("entry24", selected_property->type);
    dialog->show();
    if (dialog->run() == Gtk::RESPONSE_OK) {
        selected_property->setName( getTextEntry("entry23") );
        selected_property->type = getTextEntry("entry24");
    }
    dialog->hide();
    update();
}

void VREntityWidget::on_rem_prop_clicked() { // needed? the properties are defined in the concept!
    /*if (!selected_property) return;
    bool b = askUser("Delete property " + selected_property->getName() + "?", "Are you sure you want to delete this property?");
    if (!b) return;
    entity->remProperty(selected_property);
    selected_property = 0;
    update();*/
}

void VREntityWidget::on_rem_clicked() {
    bool b = askUser("Delete entity " + label->get_text() + "?", "Are you sure you want to delete this entity?");
    if (b) manager->remEntity(this);
}

void VREntityWidget::on_edit_clicked() {
    string s = askUserInput("Rename entity " + label->get_text() + ":");
    if (s == "") return;
    manager->getSelectedOntology()->renameEntity(entity, s);
    label->set_text(entity->getName());
}

void VREntityWidget::on_newp_clicked() { // deprecated
    /*Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );
    string name = "new_property";
    int i=0;
    do {
        i++;
        name = "new_property_" + toString(i);
    } while(entity->getProperty(name));
    setPropRow(liststore->append(), name, "none", "orange", 0);
    entity->addProperty(name, "none");*/
}

void VREntityWidget::on_select_property() {
    Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
    if (!iter) return;

    VRGuiSemantics_PropsColumns cols;
    Gtk::TreeModel::Row row = *iter;
    int flag = row.get_value(cols.flag);
    selected_property = flag ? 0 : entity->concept->getProperty( row.get_value(cols.prop) );
    treeview->get_selection()->unselect_all(); // clear selection
    update();
}

void VREntityWidget::update() {
    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );

    liststore->clear();
    for (auto pv : entity->properties) {
        for (auto p : pv.second) {
            Gtk::TreeModel::iterator i = liststore->append();
            if (selected_property && p->getName() == selected_property->getName())
                setPropRow(i, p->getName(), p->type, "green", 1);
            else setPropRow(i, p->getName(), p->type, "black", 0);
        }
    }
}


