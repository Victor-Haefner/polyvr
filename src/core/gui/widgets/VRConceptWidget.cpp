#include "VRConceptWidget.h"
#include "VREntityWidget.h"
#include "VRRuleWidget.h"

#include "addons/Semantics/Reasoning/VRConcept.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include "../VRGuiSemantics.h"

#include "core/utils/toString.h"

using namespace OSG;

// TODO

VRConceptWidget::VRConceptWidget(VRGuiSemantics* m, VRConceptPtr concept) : VRSemanticWidget(m, "#00CCFF") {
    this->concept = concept;
    /*gtk_label_set_text(label, concept->getName().c_str());

    auto toolbar2 = gtk_toolbar_new();
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar2), GTK_ICON_SIZE_MENU);
    gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar2), 0);
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar2), GTK_TOOLBAR_TEXT);
    auto bConceptNew = gtk_tool_button_new(0, "C");
    auto bEntityNew = gtk_tool_button_new(0, "E");
    auto bRuleNew = gtk_tool_button_new(0, "R");
    auto sep = gtk_separator_tool_item_new();
    gtk_container_add(GTK_CONTAINER(toolbar2), GTK_WIDGET(sep));
    gtk_container_add(GTK_CONTAINER(toolbar2), GTK_WIDGET(bConceptNew));
    gtk_container_add(GTK_CONTAINER(toolbar2), GTK_WIDGET(bEntityNew));
    gtk_container_add(GTK_CONTAINER(toolbar2), GTK_WIDGET(bRuleNew));
    gtk_box_pack_start(toolbars, toolbar2, true, true, 0);
    gtk_widget_show_all(GTK_WIDGET(toolbars));

    gtk_widget_set_tooltip_text(GTK_WIDGET(bConceptNew), "new concept");
    gtk_widget_set_tooltip_text(GTK_WIDGET(bEntityNew), "new entity");
    gtk_widget_set_tooltip_text(GTK_WIDGET(bRuleNew), "new rule");

    connect_signal<void>(bConceptNew, bind(&VRConceptWidget::on_new_concept_clicked, this), "clicked", false);
    connect_signal<void>(bEntityNew, bind(&VRConceptWidget::on_new_entity_clicked, this), "clicked", false);
    connect_signal<void>(bRuleNew, bind(&VRConceptWidget::on_new_rule_clicked, this), "clicked", false);


    auto store = GTK_TREE_STORE( gtk_tree_view_get_model(treeview) );
    for (auto p : concept->properties) {
        GtkTreeIter itr;
        gtk_tree_store_append(store, &itr, 0);
        setPropRow(&itr, p.second->getName(), p.second->type, "black", 0);
    }*/
}

int VRConceptWidget::ID() { return concept->ID; }

void VRConceptWidget::on_edit_prop_clicked() {
    if (!selected_property) return;
    /*auto dialog = VRGuiBuilder::get()->get_widget("PropertyEdit");
    setTextEntry("entry23", selected_property->getName());
    setTextEntry("entry24", selected_property->type);
    gtk_widget_show(dialog);
    auto res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_OK) {
        selected_property->setName( getTextEntry("entry23") );
        selected_property->type = getTextEntry("entry24");
        saveScene();
    }
    gtk_widget_hide(dialog);*/
    update();
}

void VRConceptWidget::on_rem_prop_clicked() {
    if (!selected_property) return;
    /*bool b = askUser("Delete property " + selected_property->getName() + "?", "Are you sure you want to delete this property?");
    if (!b) return;
    concept->remProperty(selected_property);
    selected_property = 0;*/
    update();
    saveScene();
}

void VRConceptWidget::on_rem_clicked() {
    /*string txt = gtk_label_get_text(label);
    bool b = askUser("Delete concept " + txt + "?", "Are you sure you want to delete this concept?");
    if (b) manager->remConcept(this);*/
}

void VRConceptWidget::on_edit_clicked() {
    /*string txt = gtk_label_get_text(label);
    string s = askUserInput("Rename concept " + txt + ":");
    if (s == "") return;
    manager->getSelectedOntology()->renameConcept(concept, s);
    gtk_label_set_text(label, concept->getName().c_str());*/
    saveScene();
}

void VRConceptWidget::on_newp_clicked() {
    /*auto store = GTK_TREE_STORE( gtk_tree_view_get_model(treeview) );
    string name = "new_property";
    int i=0;
    do {
        i++;
        name = "new_property_" + toString(i);
    } while(concept->getProperty(name));
    GtkTreeIter itr;
    gtk_tree_store_append(store, &itr, 0);
    setPropRow(&itr, name, "none", "orange", 0);
    concept->addProperty(name, "none");*/
    saveScene();
}

void VRConceptWidget::on_select_property() {
    /*GtkTreeIter itr;
    auto treeselection = gtk_tree_view_get_selection(treeview);
    auto model = gtk_tree_view_get_model(treeview);
    bool res = gtk_tree_selection_get_selected(treeselection, &model, &itr);
    if (!res) return;

    int flag;
    const char* prop;
    gtk_tree_model_get(model, &itr, 2, &prop, -1);
    gtk_tree_model_get(model, &itr, 4, &flag, -1);
    selected_property = flag ? 0 : concept->getProperty( prop );
    gtk_tree_selection_unselect_all(treeselection); // clear selection*/
    update();
}

void VRConceptWidget::update() {
    /*auto store = GTK_TREE_STORE( gtk_tree_view_get_model(treeview) );

    gtk_tree_store_clear(store);
    for (auto p : concept->properties) {
        GtkTreeIter itr;
        gtk_tree_store_append(store, &itr, 0);
        if (selected_property && p.second->getName() == selected_property->getName())
            setPropRow(&itr, p.second->getName(), p.second->type, "green", 1);
        else setPropRow(&itr, p.second->getName(), p.second->type, "black", 0);
    }*/
}

void VRConceptWidget::on_new_concept_clicked() { manager->copyConcept(this); }
void VRConceptWidget::on_new_entity_clicked() { manager->addEntity(this); }
void VRConceptWidget::on_new_rule_clicked() { manager->addRule(this); }

void VRConceptWidget::reparent(VREntityWidgetPtr w) {}

void VRConceptWidget::reparent(VRConceptWidgetPtr w) {
    VRConceptPtr c = w->concept;
    if (c->hasParent(concept) && c->parents.size() > 1) { // remove parent if he has at least two!
        c->removeParent(concept);
        manager->disconnect(ptr(), w);
    } else { // add parent
        concept->append(c);
        manager->connect(ptr(), w, "#00CCFF");
    }
    saveScene();
}

void VRConceptWidget::reparent(VRRuleWidgetPtr w) {
    w->rule->associatedConcept = concept->getName();
    manager->disconnectAny(w);
    manager->connect(ptr(), w, "#00DD00");
    saveScene();
}




