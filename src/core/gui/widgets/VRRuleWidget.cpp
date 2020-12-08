#include "VRRuleWidget.h"

#include "addons/Semantics/Reasoning/VROntologyRule.h"
#include "addons/Semantics/Reasoning/VRStatement.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include "../VRGuiUtils.h"
#include "../VRGuiSemantics.h"

#include "core/utils/toString.h"

using namespace OSG;

// TODO

/*VRRuleWidget::VRRuleWidget(VRGuiSemantics* m, Gtk::Fixed* canvas, VROntologyRulePtr rule) : VRSemanticWidget(m, canvas, "#00DD00") {
    this->rule = rule;
    label->set_text("rule");
    if (rule->query) label->set_text(rule->query->toString());

    Glib::RefPtr<Gtk::TreeStore> treestore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );
    for (auto s : rule->statements) {
        setPropRow(treestore->append(), s->toString(), "", "black", 0);
    }
}

int VRRuleWidget::ID() { return rule->ID; }

void VRRuleWidget::on_edit_prop_clicked() {
    if (!selected_statement) return;
    Gtk::Dialog* dialog;
    VRGuiBuilder::get()->get_widget("PropertyEdit", dialog);
    setTextEntry("entry23", selected_statement->toString());
    //setTextEntry("entry24", selected_statement->toString());
    dialog->show();
    if (dialog->run() == Gtk::RESPONSE_OK) {
        selected_statement->statement = getTextEntry("entry23");
        selected_statement->setup(0);
        //selected_statement->type = getTextEntry("entry24");
        saveScene();
    }
    dialog->hide();
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
    bool b = askUser("Delete rule " + label->get_text() + "?", "Are you sure you want to delete this concept?");
    if (b) manager->remRule(this);
}

void VRRuleWidget::on_edit_clicked() {
    string s = askUserInput("Change rule " + label->get_text() + ":");
    if (s == "") return;
    rule->setQuery(s);
    if (rule->query) label->set_text(rule->query->toString());
    saveScene();
}

void VRRuleWidget::on_newp_clicked() {
    Glib::RefPtr<Gtk::TreeStore> treestore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );
    string name = rule->associatedConcept + "(x)";
    setPropRow(treestore->append(), name, "", "orange", 0);
    rule->addStatement(name);
    saveScene();
}

void VRRuleWidget::on_select_property() {
    Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
    Glib::RefPtr<Gtk::TreeStore> store = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );
    if (!iter) return;

    auto getStorePos = [&]() {
        Gtk::TreeModel::iterator iter2;
        int N = gtk_tree_model_iter_n_children( (GtkTreeModel*) store->gobj(), NULL );
        for (int i=0; i<N; i++) {
            iter2 = store->get_iter( toString(i) );
            if (iter2 == iter) return i;
        }
        return -1;
    };

    VRGuiSemantics_PropsColumns cols;
    Gtk::TreeModel::Row row = *iter;
    int flag = row.get_value(cols.flag);
    selected_statement = flag ? 0 : rule->getStatement( getStorePos() );
    treeview->get_selection()->unselect_all(); // clear selection
    update();
}

void VRRuleWidget::update() {
    Glib::RefPtr<Gtk::TreeStore> treestore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );

    treestore->clear();
    for (auto p : rule->statements) {
        Gtk::TreeModel::iterator i = treestore->append();
        if (selected_statement && p->toString() == selected_statement->toString())
            setPropRow(i, p->toString(), "", "green", 1);
        else setPropRow(i, p->toString(), "", "black", 0);
    }
}

void VRRuleWidget::reparent(VRConceptWidgetPtr w) {}
void VRRuleWidget::reparent(VREntityWidgetPtr w) {}
void VRRuleWidget::reparent(VRRuleWidgetPtr w) {}*/
