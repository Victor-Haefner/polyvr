#include "VRConceptWidget.h"

#include "addons/Semantics/Reasoning/VRConcept.h"
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

VRConceptWidget::VRConceptWidget(VRGuiSemantics* m, Gtk::Fixed* canvas, VRConceptPtr concept) : VRSemanticWidget(m, canvas) {
    this->concept = concept;
    label->set_text(concept->getName());

    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );
    for (auto p : concept->properties) {
        setPropRow(liststore->append(), p.second->getName(), p.second->type, "black", 0);
    }
}

void VRConceptWidget::on_edit_prop_clicked() {
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

void VRConceptWidget::on_rem_prop_clicked() {
    if (!selected_property) return;
    bool b = askUser("Delete property " + selected_property->getName() + "?", "Are you sure you want to delete this property?");
    if (!b) return;
    concept->remProperty(selected_property);
    selected_property = 0;
    update();
}

void VRConceptWidget::on_rem_clicked() {
    bool b = askUser("Delete concept " + label->get_text() + "?", "Are you sure you want to delete this concept?");
    if (b) manager->remConcept(this);
}

void VRConceptWidget::on_edit_clicked() {
    string s = askUserInput("Rename concept " + label->get_text() + ":");
    if (s == "") return;
    manager->getSelectedOntology()->renameConcept(concept, s);
    label->set_text(concept->getName());
}

void VRConceptWidget::on_newp_clicked() {
    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );
    string name = "new_property";
    int i=0;
    do {
        i++;
        name = "new_property_" + toString(i);
    } while(concept->getProperty(name));
    setPropRow(liststore->append(), name, "none", "orange", 0);
    concept->addProperty(name, "none");
}

void VRConceptWidget::on_select_property() {
    Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
    if (!iter) return;

    VRGuiSemantics_PropsColumns cols;
    Gtk::TreeModel::Row row = *iter;
    int flag = row.get_value(cols.flag);
    selected_property = flag ? 0 : concept->getProperty( row.get_value(cols.prop) );
    treeview->get_selection()->unselect_all(); // clear selection
    update();
}

void VRConceptWidget::update() {
    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );

    liststore->clear();
    for (auto p : concept->properties) {
        Gtk::TreeModel::iterator i = liststore->append();
        if (selected_property && p.second->getName() == selected_property->getName())
            setPropRow(i, p.second->getName(), p.second->type, "green", 1);
        else setPropRow(i, p.second->getName(), p.second->type, "black", 0);
    }
}

void VRConceptWidget::on_new_clicked() { manager->copyConcept(this); }
