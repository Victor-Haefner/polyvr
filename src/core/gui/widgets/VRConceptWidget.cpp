#include "VRConceptWidget.h"
#include "VREntityWidget.h"
#include "VRRuleWidget.h"

#include "addons/Semantics/Reasoning/VRConcept.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include "../VRGuiUtils.h"
#include "../VRGuiSemantics.h"

#include "core/utils/toString.h"

using namespace OSG;

// TODO

/*VRConceptWidget::VRConceptWidget(VRGuiSemantics* m, GtkFixed* canvas, VRConceptPtr concept) : VRSemanticWidget(m, canvas, "#00CCFF") {
    this->concept = concept;
    label->set_text(concept->getName());

    auto toolbar2 = Gtk::manage( new Gtk::Toolbar() );
    toolbar2->set_icon_size(Gtk::ICON_SIZE_MENU);
    toolbar2->set_show_arrow(0);
    toolbar2->set_toolbar_style(Gtk::TOOLBAR_TEXT);
    auto bConceptNew = Gtk::manage( new Gtk::ToolButton("C") );
    auto bEntityNew = Gtk::manage( new Gtk::ToolButton("E") );
    auto bRuleNew = Gtk::manage( new Gtk::ToolButton("R") );
    auto sep = Gtk::manage( new Gtk::SeparatorToolItem() );
    toolbar2->add(*sep);
    toolbar2->add(*bConceptNew);
    toolbar2->add(*bEntityNew);
    toolbar2->add(*bRuleNew);
    toolbars->pack_start(*toolbar2);
    toolbars->show_all();

    bConceptNew->set_tooltip_text("new concept");
    bEntityNew->set_tooltip_text("new entity");
    bRuleNew->set_tooltip_text("new rule");

    bConceptNew->signal_clicked().connect( sigc::mem_fun(*this, &VRConceptWidget::on_new_concept_clicked) );
    bEntityNew->signal_clicked().connect( sigc::mem_fun(*this, &VRConceptWidget::on_new_entity_clicked) );
    bRuleNew->signal_clicked().connect( sigc::mem_fun(*this, &VRConceptWidget::on_new_rule_clicked) );

    Glib::RefPtr<Gtk::TreeStore> treestore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );
    for (auto p : concept->properties) {
        setPropRow(treestore->append(), p.second->getName(), p.second->type, "black", 0);
    }
}

int VRConceptWidget::ID() { return concept->ID; }

void VRConceptWidget::on_edit_prop_clicked() {
    if (!selected_property) return;
    Gtk::Dialog* dialog;
    VRGuiBuilder::get()->get_widget("PropertyEdit", dialog);
    setTextEntry("entry23", selected_property->getName());
    setTextEntry("entry24", selected_property->type);
    dialog->show();
    if (dialog->run() == Gtk::RESPONSE_OK) {
        selected_property->setName( getTextEntry("entry23") );
        selected_property->type = getTextEntry("entry24");
        saveScene();
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
    saveScene();
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
    saveScene();
}

void VRConceptWidget::on_newp_clicked() {
    Glib::RefPtr<Gtk::TreeStore> treestore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );
    string name = "new_property";
    int i=0;
    do {
        i++;
        name = "new_property_" + toString(i);
    } while(concept->getProperty(name));
    setPropRow(treestore->append(), name, "none", "orange", 0);
    concept->addProperty(name, "none");
    saveScene();
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
    Glib::RefPtr<Gtk::TreeStore> treestore = Glib::RefPtr<Gtk::TreeStore>::cast_dynamic( treeview->get_model() );

    treestore->clear();
    for (auto p : concept->properties) {
        Gtk::TreeModel::iterator i = treestore->append();
        if (selected_property && p.second->getName() == selected_property->getName())
            setPropRow(i, p.second->getName(), p.second->type, "green", 1);
        else setPropRow(i, p.second->getName(), p.second->type, "black", 0);
    }
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
}*/




