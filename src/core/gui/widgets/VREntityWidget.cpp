#include "VREntityWidget.h"

#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include "../VRGuiUtils.h"
#include "../VRGuiSemantics.h"

#include "core/utils/toString.h"

using namespace OSG;

// TODO

/*VREntityWidget::VREntityWidget(VRGuiSemantics* m, Gtk::Fixed* canvas, VREntityPtr entity) : VRSemanticWidget(m, canvas, "#FFEE00") {
    this->entity = entity;
    label->set_text(entity->getName());
    update();
}

int VREntityWidget::ID() { return entity->ID; }

void VREntityWidget::on_edit_prop_clicked() {
    if (!selected_entity_property) return;
    Gtk::Dialog* dialog;
    VRGuiBuilder::get()->get_widget("PropertyEdit", dialog);
    setTextEntry("entry23", selected_entity_property->getName());
    setTextEntry("entry24", selected_entity_property->value);
    dialog->show();
    if (dialog->run() == Gtk::RESPONSE_OK) {
        //selected_entity_property->setName( getTextEntry("entry23") );
        selected_entity_property->setValue( getTextEntry("entry24") );
    }
    dialog->hide();
    update();
    saveScene();
}

void VREntityWidget::on_rem_prop_clicked() {
    if (!selected_entity_property) return;
    bool b = askUser("Delete property " + selected_entity_property->getName() + "?", "Are you sure you want to delete this property?");
    if (!b) return;
    entity->rem(selected_entity_property); // TODO
    selected_entity_property = 0;
    update();
    saveScene();
}

void VREntityWidget::on_newp_clicked() {
    if (!selected_concept_property) return;
    Glib::RefPtr<Gtk::TreeStore> treestore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );
    string name = selected_concept_property->getName();
    //setPropRow(selected_concept_property->append(), name, "", "orange", 0);
    entity->add(name, "");
    update();
    saveScene();
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
    saveScene();
}

void VREntityWidget::on_select_property() {
    Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
    if (!iter) return;

    VRGuiSemantics_PropsColumns cols;
    Gtk::TreeModel::Row row = *iter;
    int flag = row.get_value(cols.flag);
    int type = row.get_value(cols.rtype);
    int ID = row.get_value(cols.ID);

    VRPropertyPtr p = 0;
    if (type == 0) p = entity->getProperty( row.get_value(cols.prop), true );
    if (type == 1) {
        auto pv = entity->getAll( row.get_value(cols.prop) );
        for (auto pi : pv) if (pi->ID == ID) { p = pi; break; }
    }

    selected_concept_property = selected_entity_property = 0;
    if (flag == 0 && type == 0) selected_concept_property = p;
    if (flag == 0 && type == 1) selected_entity_property = p;
    treeview->get_selection()->unselect_all(); // clear selection
    update();
}

void VREntityWidget::update() {
    Glib::RefPtr<Gtk::TreeStore> treestore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );
    treestore->clear();
    if (!entity) return;

    for (auto cp : entity->getProperties()) {
        Gtk::TreeModel::iterator i = treestore->append();
        bool selected = selected_concept_property == cp;
        string color = selected ? "green" : "black";
        setPropRow(i, cp->getName(), cp->type, color, selected, cp->ID, 0);

        for (auto ep : entity->getAll(cp->getName())) {
            selected = selected_entity_property == ep;
            color = selected ? "green" : "black";
            Gtk::TreeModel::iterator j = treestore->append(i->children());
            setPropRow(j, ep->getName(), ep->value, color, selected, ep->ID, 1);
        }
    }

    treeview->expand_all();
}

void VREntityWidget::reparent(VRConceptWidgetPtr w) {}
void VREntityWidget::reparent(VREntityWidgetPtr w) {}
void VREntityWidget::reparent(VRRuleWidgetPtr w) {}*/

