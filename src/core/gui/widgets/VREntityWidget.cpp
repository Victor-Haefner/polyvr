#include "VREntityWidget.h"

#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include "../VRGuiUtils.h"
#include "../VRGuiSemantics.h"
#include "../VRGuiBuilder.h"

#include "core/utils/toString.h"

using namespace OSG;

// TODO

VREntityWidget::VREntityWidget(VRGuiSemantics* m, GtkFixed* canvas, VREntityPtr entity) : VRSemanticWidget(m, canvas, "#FFEE00") {
    this->entity = entity;
    gtk_label_set_text(label, entity->getName().c_str());
    update();
}

int VREntityWidget::ID() { return entity->ID; }

void VREntityWidget::on_edit_prop_clicked() {
    if (!selected_entity_property) return;
    auto dialog = VRGuiBuilder::get()->get_widget("PropertyEdit");
    setTextEntry("entry23", selected_entity_property->getName());
    setTextEntry("entry24", selected_entity_property->value);
    gtk_widget_show(dialog);
    auto res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_OK) {
        //selected_entity_property->setName( getTextEntry("entry23") );
        selected_entity_property->setValue( getTextEntry("entry24") );
    }
    gtk_widget_hide(dialog);
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
    string name = selected_concept_property->getName();
    //setPropRow(selected_concept_property->append(), name, "", "orange", 0);
    entity->add(name, "");
    update();
    saveScene();
}

void VREntityWidget::on_rem_clicked() {
    string txt = gtk_label_get_text(label);
    bool b = askUser("Delete entity " + txt + "?", "Are you sure you want to delete this entity?");
    if (b) manager->remEntity(this);
}

void VREntityWidget::on_edit_clicked() {
    string txt = gtk_label_get_text(label);
    string s = askUserInput("Rename entity " + txt + ":");
    if (s == "") return;
    manager->getSelectedOntology()->renameEntity(entity, s);
    gtk_label_set_text(label, entity->getName().c_str());
    saveScene();
}

void VREntityWidget::on_select_property() {
    auto model = gtk_tree_view_get_model(treeview);
    GtkTreeIter itr;
    auto treeselection = gtk_tree_view_get_selection(treeview);
    auto res = gtk_tree_selection_get_selected(treeselection, &model, &itr);
    if (!res) return;

    int type, flag, ID;
    const char* prop;
    gtk_tree_model_get(model, &itr, 2, &prop, -1);
    gtk_tree_model_get(model, &itr, 4, &flag, -1);
    gtk_tree_model_get(model, &itr, 5, &type, -1);
    gtk_tree_model_get(model, &itr, 6, &ID, -1);

    VRPropertyPtr p = 0;
    if (type == 0) p = entity->getProperty( prop, true );
    if (type == 1) {
        auto pv = entity->getAll( prop );
        for (auto pi : pv) if (pi->ID == ID) { p = pi; break; }
    }

    selected_concept_property = selected_entity_property = 0;
    if (flag == 0 && type == 0) selected_concept_property = p;
    if (flag == 0 && type == 1) selected_entity_property = p;
    gtk_tree_selection_unselect_all(treeselection); // clear selection
    update();
}

void VREntityWidget::update() {
    auto store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    gtk_tree_store_clear(store);
    if (!entity) return;

    for (auto cp : entity->getProperties()) {
        GtkTreeIter itr;
        gtk_tree_store_append(store, &itr, 0);
        bool selected = selected_concept_property == cp;
        string color = selected ? "green" : "black";
        setPropRow(&itr, cp->getName(), cp->type, color, selected, cp->ID, 0);

        for (auto ep : entity->getAll(cp->getName())) {
            selected = selected_entity_property == ep;
            color = selected ? "green" : "black";
            GtkTreeIter itr2;
            gtk_tree_store_append(store, &itr2, &itr);
            setPropRow(&itr2, ep->getName(), ep->value, color, selected, ep->ID, 1);
        }
    }

    gtk_tree_view_expand_all(treeview);
}

void VREntityWidget::reparent(VRConceptWidgetPtr w) {}
void VREntityWidget::reparent(VREntityWidgetPtr w) {}
void VREntityWidget::reparent(VRRuleWidgetPtr w) {}

