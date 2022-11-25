#include "VRRuleWidget.h"

#include "addons/Semantics/Reasoning/VROntologyRule.h"
#include "addons/Semantics/Reasoning/VRStatement.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include "../VRGuiUtils.h"
#include "../VRGuiBuilder.h"
#include "../VRGuiSemantics.h"

#include "core/utils/toString.h"

using namespace OSG;

// TODO

VRRuleWidget::VRRuleWidget(VRGuiSemantics* m, GtkFixed* canvas, VROntologyRulePtr rule) : VRSemanticWidget(m, canvas, "#00DD00") {
    this->rule = rule;
    if (rule->query) gtk_label_set_text(label, rule->query->toString().c_str());
    else gtk_label_set_text(label, "rule");

    GtkTreeStore* treestore = GTK_TREE_STORE( gtk_tree_view_get_model( treeview ) );
    for (auto s : rule->statements) {
        GtkTreeIter itr;
        gtk_tree_store_append(treestore, &itr, 0);
        setPropRow(&itr, s->toString(), "", "black", 0);
    }
}

int VRRuleWidget::ID() { return rule->ID; }

void VRRuleWidget::on_edit_prop_clicked() {
    if (!selected_statement) return;
    auto dialog = VRGuiBuilder::get()->get_widget("PropertyEdit");
    setTextEntry("entry23", selected_statement->toString());
    //setTextEntry("entry24", selected_statement->toString());
    gtk_widget_show(dialog);
    auto res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_OK) {
        selected_statement->statement = getTextEntry("entry23");
        selected_statement->setup(0);
        //selected_statement->type = getTextEntry("entry24");
        saveScene();
    }
    gtk_widget_hide(dialog);
    update();
}

void VRRuleWidget::on_rem_prop_clicked() {
    if (!selected_statement) return;
    bool b = askUser("Delete statement " + selected_statement->toString() + "?", "Are you sure you want to delete this property?");
    if (!b) return;
    rule->remStatement(selected_statement);
    selected_statement = 0;
    update();
    saveScene();
}

void VRRuleWidget::on_rem_clicked() {
    string txt = gtk_label_get_text(label);
    bool b = askUser("Delete rule " + txt + "?", "Are you sure you want to delete this concept?");
    if (b) manager->remRule(this);
}

void VRRuleWidget::on_edit_clicked() {
    string txt = gtk_label_get_text(label);
    string s = askUserInput("Change rule " + txt + ":");
    if (s == "") return;
    rule->setQuery(s);
    if (rule->query) gtk_label_set_text(label, rule->query->toString().c_str());
    saveScene();
}

void VRRuleWidget::on_newp_clicked() {
    auto store = GTK_TREE_STORE( gtk_tree_view_get_model(treeview) );
    string name = rule->associatedConcept + "(x)";
    GtkTreeIter itr;
    gtk_tree_store_append(store, &itr, 0);
    setPropRow(&itr, name, "", "orange", 0);
    rule->addStatement(name);
    saveScene();
}

void VRRuleWidget::on_select_property() {
    auto store = GTK_TREE_STORE( gtk_tree_view_get_model(treeview) );
    GtkTreeIter itr;
    auto treeselection = gtk_tree_view_get_selection(treeview);
    auto model = GTK_TREE_MODEL(store);
    bool res = gtk_tree_selection_get_selected(treeselection, &model, &itr);
    if (!res) return;

    auto getStorePos = [&]() {
        GtkTreeIter itr2;
        int N = gtk_tree_model_iter_n_children( model, NULL );
        for (int i=0; i<N; i++) {
            gtk_tree_model_get_iter_from_string(model, &itr2, toString(i).c_str());
            if (&itr2 == &itr) return i;
        }
        return -1;
    };

    int flag;
    gtk_tree_model_get(model, &itr, 4, &flag, -1);
    selected_statement = flag ? 0 : rule->getStatement( getStorePos() );
    gtk_tree_selection_unselect_all(treeselection); // clear selection
    update();
}

void VRRuleWidget::update() {
    auto store = GTK_TREE_STORE( gtk_tree_view_get_model(treeview) );

    gtk_tree_store_clear(store);
    for (auto p : rule->statements) {
        GtkTreeIter itr;
        gtk_tree_store_append(store, &itr, 0);
        if (selected_statement && p->toString() == selected_statement->toString())
            setPropRow(&itr, p->toString(), "", "green", 1);
        else setPropRow(&itr, p->toString(), "", "black", 0);
    }
}

void VRRuleWidget::reparent(VRConceptWidgetPtr w) {}
void VRRuleWidget::reparent(VREntityWidgetPtr w) {}
void VRRuleWidget::reparent(VRRuleWidgetPtr w) {}
