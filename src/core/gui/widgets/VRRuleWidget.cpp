#include "VRRuleWidget.h"

#include "addons/Semantics/Reasoning/VROntologyRule.h"
#include "addons/Semantics/Reasoning/VRStatement.h"
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

VRRuleWidget::VRRuleWidget(VRGuiSemantics* m, Gtk::Fixed* canvas, VROntologyRulePtr rule) : VRSemanticWidget(m, canvas) {
    this->rule = rule;
    label->set_text(rule->getName());

    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );
    /*for (auto p : concept->properties) {
        setPropRow(liststore->append(), p.second->getName(), p.second->type, "black", 0);
    }*/
}

void VRRuleWidget::on_edit_prop_clicked() {
    if (!selected_statement) return;
    Gtk::Dialog* dialog;
    VRGuiBuilder()->get_widget("PropertyEdit", dialog);
    setTextEntry("entry23", selected_statement->verb);
    setTextEntry("entry24", selected_statement->toString());
    dialog->show();
    if (dialog->run() == Gtk::RESPONSE_OK) {
        selected_statement->verb = getTextEntry("entry23");
        //selected_statement->type = getTextEntry("entry24");
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
}

void VRRuleWidget::on_rem_clicked() {
    bool b = askUser("Delete rule " + label->get_text() + "?", "Are you sure you want to delete this concept?");
    if (b) manager->remRule(this);
}

void VRRuleWidget::on_edit_clicked() {
    string s = askUserInput("Rename rule " + label->get_text() + ":");
    if (s == "") return;
    manager->getSelectedOntology()->renameRule(rule, s);
    label->set_text(rule->getName());
}

void VRRuleWidget::on_newp_clicked() {
    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );
    string name = "is(a,b)";
    setPropRow(liststore->append(), name, "none", "orange", 0);
    rule->addStatement(name);
}

void VRRuleWidget::on_select_property() {
    Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );
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
    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );

    liststore->clear();
    for (auto p : rule->statements) {
        Gtk::TreeModel::iterator i = liststore->append();
        if (selected_statement && p->toString() == selected_statement->toString())
            setPropRow(i, p->toString(), "", "green", 1);
        else setPropRow(i, p->toString(), "", "black", 0);
    }
}

