#include "VRGuiSemantics.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
#include "VRGuiFile.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "core/utils/toString.h"
#include "core/math/partitioning/graph.h"

#include "core/scene/VRScene.h"
#include "core/scene/VRSemanticManager.h"
#include "addons/Algorithms/VRGraphLayout.h"

#include "widgets/VRWidgetsCanvas.h"
#include "widgets/VRCanvasWidget.h"
#include "widgets/VRSemanticWidget.h"
#include "widgets/VRConceptWidget.h"
#include "widgets/VREntityWidget.h"
#include "widgets/VRRuleWidget.h"
#include "widgets/VRConnectorWidget.h"

#include "wrapper/VRGuiTreeView.h"


/** TODO:

- VRGraphLayout
    - 3D bounding boxes, especially when opening a concept widget

- add entities
- add rules

- fix connectors
    - add connector anchor class
        - fix connector type depending on anchor normal
    - link entities to concepts
    - link object properties of entites to property entity
    - link object properties of concept to property concept

- dnd concept onto other concept for reparenting concept!

- ontology bugs
    - inherited properties not shown??

IDEAS:
- use color highlights:
    - click on a concept -> all parent concept names go red
                         -> all child concepts go blue
    - click on a obj property -> concept name goes green

*/

OSG_BEGIN_NAMESPACE;
using namespace std;

void VRGuiSemantics::clear() {
    canvas->clear();
}

void VRGuiSemantics::on_new_clicked() {
    auto mgr = getManager();
    if (!mgr) return;

    string name = "new_concept";
    int i=0;
    do {
        i++;
        name = "new_concept_" + toString(i);
    } while(mgr->getOntology(name));

    auto o = mgr->addOntology(name);
    o->setPersistency(666);
    o->setFlag("custom");
    updateOntoList();
    saveScene();
}

void VRGuiSemantics::on_del_clicked() {
    auto mgr = getManager();
    if (!mgr) return;
    mgr->remOntology(current);
    clear();
    updateOntoList();
    saveScene();
}

void VRGuiSemantics::on_diag_load_clicked() {
    auto mgr = getManager();
    if (!mgr) return;
    string path = VRGuiFile::getPath();
    auto o = mgr->loadOntology(path);
    o->setPersistency(666);
    o->setFlag("custom");
    updateOntoList();
    saveScene();
}

void VRGuiSemantics::on_open_clicked() {
    VRGuiFile::setCallbacks( bind(&VRGuiSemantics::on_diag_load_clicked, this) );
    VRGuiFile::gotoPath( g_get_home_dir() );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Ontology", 1, "*.owl");
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::open( "Load", "open", "Load ontology" );
}

void VRGuiSemantics::setOntology(string name) {
    auto mgr = getManager();
    if (mgr == 0) return;
    current = mgr->getOntology(name);
    updateCanvas();
}

void VRGuiSemantics::updateCanvas() {
    VRTimer t;
    int i = 0;
    clear();

    function<void(map<int, vector<VRConceptPtr>>&, VRConceptPtr,int,int,VRConceptWidgetPtr)> travConcepts = [&](map<int, vector<VRConceptPtr>>& cMap, VRConceptPtr c, int cID, int lvl, VRConceptWidgetPtr cp) {
        if (auto w = canvas->getWidget(c->ID)) {
            if (cp) connect(cp, w, "#00CCFF");
            return;
        }

        auto cw = VRConceptWidgetPtr( new VRConceptWidget(this, canvas->getCanvas(), c) );
        canvas->addWidget(c->ID, cw);
        canvas->addNode(c->ID);

        int x = 150+cID*60;
        int y = 150+40*lvl;
        cw->move(Vec2d(x,y));
        if (cp) connect(cp, cw, "#00CCFF");

        i++;
        int child_i = 0;
        for (auto ci : cMap[c->ID]) { travConcepts(cMap, ci, child_i, lvl+1, cw); child_i++; }
    };

    if (current) {
        auto cMap = current->getChildrenMap();
        travConcepts(cMap, current->thing, 0, 0, 0);
        auto layout = canvas->getLayout();
        layout->fixNode( canvas->getNode(current->thing->ID) );

        for (auto e : current->entities) {
            auto ew = VREntityWidgetPtr( new VREntityWidget(this, canvas->getCanvas(), e.second) );
            canvas->addWidget(ew->ID(), ew);
            canvas->addNode(ew->ID());
            ew->move(Vec2d(150,150));
            for ( auto c : e.second->getConcepts() ) connect(canvas->getWidget(c->ID), ew, "#FFEE00");
        }

        for (auto r : current->rules) {
            auto rw = VRRuleWidgetPtr( new VRRuleWidget(this, canvas->getCanvas(), r.second) );
            canvas->addWidget(rw->ID(), rw);
            canvas->addNode(rw->ID());
            rw->move(Vec2d(150,150));
            if (auto c = current->getConcept( r.second->associatedConcept) )
                connect(canvas->getWidget(c->ID), rw, "#00DD00");
        }
    }

    gtk_widget_show_all(GTK_WIDGET(canvas->getCanvas()));
    canvas->foldAll(true);
    cout << "updateCanvas, took " << t.stop() << endl;
}

void VRGuiSemantics::connect(VRCanvasWidgetPtr w1, VRCanvasWidgetPtr w2, string color) {
    canvas->connect(w1, w2, color);
}

void VRGuiSemantics::disconnect(VRCanvasWidgetPtr w1, VRCanvasWidgetPtr w2) {
    canvas->disconnect(w1, w2);
}

void VRGuiSemantics::disconnectAny(VRCanvasWidgetPtr w) {
    canvas->disconnectAny(w);
}

void VRGuiSemantics::on_treeview_select() {
    setWidgetSensitivity("toolbutton15", true);
    clear();
    VRGuiTreeView tview("treeview16");
    string name = tview.getSelectedStringValue(0);
    string type = tview.getSelectedStringValue(1);
    if (type == "section") return;
    setOntology(name);
}

void VRGuiSemantics_on_name_edited(gchar *path_string, gchar *new_name) {
    auto scene = VRScene::getCurrent();
    if (!scene) return;

    VRGuiTreeView tview("treeview16");
    string name = tview.getSelectedStringValue(0);
    auto o = scene->getSemanticManager()->renameOntology(name, new_name);
    tview.setSelectedStringValue(0, new_name);
    saveScene();
}

VRSemanticManagerPtr VRGuiSemantics::getManager() {
    auto scene = VRScene::getCurrent();
    if (scene) return scene->getSemanticManager();
    return 0;
}

namespace PL = std::placeholders;

VRGuiSemantics::VRGuiSemantics() {
    canvas = VRWidgetsCanvas::create("onto_visu");
    setToolButtonCallback("toolbutton14", bind(&VRGuiSemantics::on_new_clicked, this));
    setToolButtonCallback("toolbutton15", bind(&VRGuiSemantics::on_del_clicked, this));
    setToolButtonCallback("toolbutton2", bind(&VRGuiSemantics::on_open_clicked, this));
    setTreeviewSelectCallback("treeview16", bind(&VRGuiSemantics::on_treeview_select, this));
    setCellRendererCallback("cellrenderertext51", bind(VRGuiSemantics_on_name_edited, placeholders::_1, placeholders::_2));
    setWidgetSensitivity("toolbutton15", false);
    setButtonCallback("button33", bind(&VRGuiSemantics::on_query_clicked, this));
    setNoteBookCallback("notebook3", bind(&VRGuiSemantics::onTabSwitched, this, placeholders::_1, placeholders::_2));
}

void VRGuiSemantics::onTabSwitched(GtkWidget* page, unsigned int tab) {
    auto nbook = VRGuiBuilder::get()->get_widget("notebook3");
    string name = gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(nbook), page);
    if (name == "Semantics") {
        updateOntoList();
    }
}

void VRGuiSemantics::on_query_clicked() {
    auto query = getTextEntry("entry26");
    if (!current) return;

    auto r = VRReasoner::create();
    auto res = r->process(query, current);
    cout << "res " << res.size() << endl;

}

bool VRGuiSemantics::updateOntoList() {
	cout << "VRGuiSemantics::updateOntoList" << endl;
    // update script list
    auto store = GTK_TREE_STORE( VRGuiBuilder::get()->get_object("onto_list") );
    gtk_tree_store_clear(store);

    auto setRow = [&](GtkTreeIter* itr, string name, string type) {
        gtk_tree_store_set(store, itr, 0, name.c_str(), -1);
        gtk_tree_store_set(store, itr, 1, type.c_str(), -1);
    };

    auto addToSection = [&](VROntologyPtr o, GtkTreeIter* sec) {
        GtkTreeIter itr;
        gtk_tree_store_append(store, &itr, sec);
        setRow( &itr, o->getName(), o->getFlag());
    };

    auto mgr = getManager();
    if (mgr == 0) return true;

    GtkTreeIter itr_sce, itr_own, itr_lib;
    gtk_tree_store_append(store, &itr_sce, NULL);
    gtk_tree_store_append(store, &itr_own, NULL);
    gtk_tree_store_append(store, &itr_lib, NULL);
    setRow( &itr_sce, "Scene", "section");
    setRow( &itr_own, "Custom", "section");
    setRow( &itr_lib, "Built-in", "section");

    for (auto o : mgr->getOntologies()) {
        cout << " ontology: " << o->getName() << " " << o->getFlag() << endl;
        if (o->getFlag() == "internal") addToSection(o, &itr_sce);
        if (o->getFlag() == "custom") addToSection(o, &itr_own);
        if (o->getFlag() == "built-in") addToSection(o, &itr_lib);
    }

    clear();
    return true;
}

void VRGuiSemantics::copyConcept(VRConceptWidget* w) {
    auto c = current->addConcept(w->concept->getName() + "_derived", w->concept->getName());
    if (!c || !w) {
        cout << current->toString() << endl;
        return;
    }
    if (!c || !w) return;
    auto cw = VRConceptWidgetPtr( new VRConceptWidget(this, canvas->getCanvas(), c) );
    canvas->addWidget(c->ID, cw);
    cw->move(w->pos + Vec2d(90,0));
    canvas->connect(canvas->getWidget(w->ID()), cw, "#00CCFF");
    saveScene();
}

void VRGuiSemantics::addEntity(VRConceptWidget* w) {
    auto c = current->addEntity(w->concept->getName() + "_entity", w->concept->getName());
    auto cw = VREntityWidgetPtr( new VREntityWidget(this, canvas->getCanvas(), c) );
    canvas->addWidget(c->ID, cw);
    cw->move(w->pos + Vec2d(90,0));
    canvas->connect(canvas->getWidget(w->ID()), cw, "#FFEE00");
    saveScene();
}

void VRGuiSemantics::addRule(VRConceptWidget* w) {
    string n = w->concept->getName();
    auto c = current->addRule("q(x):"+n+"(x)", n);
    auto cw = VRRuleWidgetPtr( new VRRuleWidget(this, canvas->getCanvas(), c) );
    canvas->addWidget(c->ID, cw);
    cw->move(w->pos + Vec2d(90,0));
    canvas->connect(canvas->getWidget(w->ID()), cw, "#00DD00");
    saveScene();
}

void VRGuiSemantics::remConcept(VRConceptWidget* w) {
    canvas->remWidget(w->concept->ID);
    current->remConcept(w->concept);
    canvas->remNode(w->concept->ID);
    //disconnectAny(); // TODO
    saveScene();
}

void VRGuiSemantics::remEntity(VREntityWidget* w) {
    canvas->remWidget(w->entity->ID);
    current->remEntity(w->entity);
    //disconnectAny(); // TODO
    saveScene();
}

void VRGuiSemantics::remRule(VRRuleWidget* w) {
    canvas->remWidget(w->rule->ID);
    current->remRule(w->rule);
    saveScene();
}

VROntologyPtr VRGuiSemantics::getSelectedOntology() { return current; }

OSG_END_NAMESPACE;
