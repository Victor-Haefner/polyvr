
#include "VRGuiSemantics.h"
#include "VRGuiUtils.h"
#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "core/utils/toString.h"
#include "core/math/graph.h"

#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/combobox.h>
#include <gtkmm/builder.h>
#include <gtkmm/fixed.h>
#include <gtkmm/frame.h>
#include <gtkmm/expander.h>
#include <gtkmm/targetentry.h>
#include <gtkmm/separator.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <boost/bind.hpp>
#include "core/scene/VRScene.h"
#include "core/scene/VRSemanticManager.h"
#include "addons/Algorithms/VRGraphLayout.h"

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

class VRGuiSemantics_ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSemantics_ModelColumns() { add(name); add(type); }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
};

class VRGuiSemantics_PropsColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSemantics_PropsColumns() { add(name); add(type); add(prop); add(ptype); add(flag); }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<Glib::ustring> prop;
        Gtk::TreeModelColumn<Glib::ustring> ptype;
        Gtk::TreeModelColumn<int> flag;
};

void VRGuiSemantics_on_drag_data_get(const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& data, unsigned int info, unsigned int time, VRGuiSemantics::BaseWidget* e) {
    data.set("concept", 0, (const guint8*)&e, sizeof(void*));
}

VRGuiSemantics::BaseWidget::BaseWidget(VRGuiSemantics* m, Gtk::Fixed* canvas) {
    this->canvas = canvas;
    manager = m;

    // properties treeview
    VRGuiSemantics_PropsColumns cols;
    auto liststore = Gtk::ListStore::create(cols);
    treeview = Gtk::manage( new Gtk::TreeView() );
    treeview->set_model(liststore);
    treeview->set_headers_visible(false);

    auto addMarkupColumn = [&](string title, Gtk::TreeModelColumn<Glib::ustring>& col, bool editable = false) {
        Gtk::CellRendererText* renderer = Gtk::manage(new Gtk::CellRendererText());
        renderer->property_editable() = editable;
        Gtk::TreeViewColumn* column = Gtk::manage(new Gtk::TreeViewColumn(title, *renderer));
        column->add_attribute(renderer->property_markup(), col);
        treeview->append_column(*column);
    };

    addMarkupColumn(" Properties:", cols.name, true);
    addMarkupColumn("", cols.type);

    // buttons
    auto toolbar = Gtk::manage( new Gtk::Toolbar() );
    auto bConceptRem = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::CLOSE) ); // Gtk::Stock::MEDIA_RECORD
    auto bConceptNew = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::MEDIA_PLAY) );
    auto bConceptName = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::EDIT) );
    auto bPropRem = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::DELETE) ); // Gtk::Stock::MEDIA_RECORD
    auto bPropNew = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::NEW) );
    auto bPropEdit = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::EDIT) );
    toolbar->add(*bConceptNew);
    toolbar->add(*bConceptName);
    toolbar->add(*bConceptRem);
    auto sep = Gtk::manage( new Gtk::SeparatorToolItem() );
    toolbar->add(*sep);
    toolbar->add(*bPropNew);
    toolbar->add(*bPropEdit);
    toolbar->add(*bPropRem);
    toolbar->set_icon_size(Gtk::ICON_SIZE_MENU);
    toolbar->set_show_arrow(0);

    bConceptNew->set_tooltip_text("new concept");
    bConceptName->set_tooltip_text("edit concept name");
    bConceptRem->set_tooltip_text("remove concept");
    bPropNew->set_tooltip_text("new property");
    bPropRem->set_tooltip_text("remove selected property");

    // expander and frame
    auto vbox = Gtk::manage( new Gtk::VBox() );
    auto expander = Gtk::manage( new Gtk::Expander("") );
    label = (Gtk::Label*)expander->get_label_widget();
    expander->add(*vbox);
    vbox->pack_start(*toolbar);
    vbox->pack_start(*treeview);
    auto frame = Gtk::manage( new Gtk::Frame() );
    frame->add(*expander);
    widget = frame;
    canvas->put(*frame, 0, 0);

    // signals
    treeview->signal_cursor_changed().connect( sigc::mem_fun(*this, &VRGuiSemantics::BaseWidget::on_select_property) );
    bConceptNew->signal_clicked().connect( sigc::mem_fun(*this, &VRGuiSemantics::BaseWidget::on_new_clicked) );
    bConceptName->signal_clicked().connect( sigc::mem_fun(*this, &VRGuiSemantics::BaseWidget::on_edit_clicked) );
    bConceptRem->signal_clicked().connect( sigc::mem_fun(*this, &VRGuiSemantics::BaseWidget::on_rem_clicked) );
    bPropNew->signal_clicked().connect( sigc::mem_fun(*this, &VRGuiSemantics::BaseWidget::on_newp_clicked) );
    bPropEdit->signal_clicked().connect( sigc::mem_fun(*this, &VRGuiSemantics::BaseWidget::on_edit_prop_clicked) );
    bPropRem->signal_clicked().connect( sigc::mem_fun(*this, &VRGuiSemantics::BaseWidget::on_rem_prop_clicked) );

    // dnd
    vector<Gtk::TargetEntry> entries;
    entries.push_back(Gtk::TargetEntry("concept", Gtk::TARGET_SAME_APP));
    expander->drag_source_set(entries, Gdk::BUTTON1_MASK, Gdk::ACTION_MOVE);
    expander->signal_drag_data_get().connect( sigc::bind<BaseWidget*>( sigc::ptr_fun(VRGuiSemantics_on_drag_data_get), this ) );
}

VRGuiSemantics::BaseWidget::~BaseWidget() {
    if (widget->get_parent() == canvas) {
        canvas->remove(*widget);
        canvas->show_all();
    }
}

void VRGuiSemantics::BaseWidget::setPropRow(Gtk::TreeModel::iterator iter, string name, string type, string color, int flag) {
    string cname = "<span color=\""+color+"\">" + name + "</span>";
    string ctype = "<span color=\""+color+"\">" + type + "</span>";

    Gtk::ListStore::Row row = *iter;
    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );

    gtk_list_store_set(liststore->gobj(), row.gobj(), 0, cname.c_str(), -1);
    gtk_list_store_set(liststore->gobj(), row.gobj(), 1, ctype.c_str(), -1);
    gtk_list_store_set(liststore->gobj(), row.gobj(), 2, name.c_str(), -1);
    gtk_list_store_set(liststore->gobj(), row.gobj(), 3, type.c_str(), -1);
    gtk_list_store_set(liststore->gobj(), row.gobj(), 4, flag, -1);
}

bool VRGuiSemantics::BaseWidget::on_expander_clicked(GdkEventButton* e) {
    cout << "EXPAND\n";
    return true;
}

void VRGuiSemantics::BaseWidget::on_select() {

}

void VRGuiSemantics::BaseWidget::move(Vec2f p) {
    pos = p;
    float w = widget->get_width();
    float h = widget->get_height();
    canvas->move(*widget, p[0]-w*0.5, p[1]-h*0.5);
}

Vec2f VRGuiSemantics::BaseWidget::getAnchorPoint(Vec2f p) {
    float w = abs(p[0]-pos[0]);
    float h = abs(p[1]-pos[1]);
    if (w >= h && p[0] < pos[0]) return pos - Vec2f(widget->get_width()*0.5, 0);
    if (w >= h && p[0] > pos[0]) return pos + Vec2f(widget->get_width()*0.5, 0);
    if (w < h && p[1] < pos[1]) return pos - Vec2f(0, widget->get_height()*0.5);
    if (w < h && p[1] > pos[1]) return pos + Vec2f(0, widget->get_height()*0.5);
    return pos;
}

VRGuiSemantics::ConnectorWidget::ConnectorWidget(Gtk::Fixed* canvas) {
    sh1 = Gtk::manage( new Gtk::HSeparator() );
    sh2 = Gtk::manage( new Gtk::HSeparator() );
    sv1 = Gtk::manage( new Gtk::VSeparator() );
    sv2 = Gtk::manage( new Gtk::VSeparator() );
    this->canvas = canvas;
    canvas->put(*sh1, 0, 0);
    canvas->put(*sh2, 0, 0);
    canvas->put(*sv1, 0, 0);
    canvas->put(*sv2, 0, 0);
}

void VRGuiSemantics::ConnectorWidget::set(ConceptWidgetPtr w1, ConceptWidgetPtr w2) {
    this->w1 = w1;
    this->w2 = w2;
    update();
}

void VRGuiSemantics::ConnectorWidget::update() {
    auto ws1 = w1.lock();
    auto ws2 = w2.lock();
    if (ws1 && ws2) {
        Vec2f a1 = ws1->getAnchorPoint(ws2->pos);
        Vec2f a2 = ws2->getAnchorPoint(ws1->pos);
        float x1 = a1[0];
        float x2 = a2[0];
        float y1 = a1[1];
        float y2 = a2[1];

        float w = abs(x2-x1);
        float h = abs(y2-y1);

        sh1->set_size_request(0, 0);
        sh2->set_size_request(0, 0);
        sv1->set_size_request(0, 0);
        sv2->set_size_request(0, 0);

        if (w <= 2 && h <= 2) return;

        if (h <= 2) {
            sh1->show();
            sh1->set_size_request(w, 2);
            if (x2 < x1) swap(x2,x1);
            canvas->move(*sh1, x1, y1);
            return;
        }

        if (w <= 2) {
            sv1->show();
            sv1->set_size_request(2, h);
            if (y2 < y1) swap(y2,y1);
            canvas->move(*sv1, x1, y1);
            return;
        }

        if (w < h) {
            sh1->show();
            sh2->show();
            sv1->show();
            sh1->set_size_request(w*0.5, 2);
            sh2->set_size_request(w*0.5, 2);
            sv1->set_size_request(2, h);

            if (y2 < y1 && x2 > x1) {
                swap(y1,y2);
                canvas->move(*sh1, x1, y2);
                canvas->move(*sh2, x1+w*0.5, y1);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 > y1 && x2 < x1) {
                swap(x1,x2);
                canvas->move(*sh1, x1, y2);
                canvas->move(*sh2, x1+w*0.5, y1);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 > y1 && x2 > x1) {
                canvas->move(*sh1, x1, y1);
                canvas->move(*sh2, x1+w*0.5, y2);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            if (y2 < y1 && x2 < x1) {
                swap(y1,y2);
                swap(x1,x2);
                canvas->move(*sh1, x1, y1);
                canvas->move(*sh2, x1+w*0.5, y2);
                canvas->move(*sv1, x1+w*0.5, y1);
                return;
            }
            return;
        } else {
            sv1->show();
            sv2->show();
            sh1->show();
            sv1->set_size_request(2, h*0.5);
            sv2->set_size_request(2, h*0.5);
            sh1->set_size_request(w, 2);

            if (y2 < y1 && x2 > x1) {
                swap(y1,y2);
                canvas->move(*sv1, x2, y1);
                canvas->move(*sv2, x1, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 < y1 && x2 < x1) {
                swap(x1,x2);
                swap(y1,y2);
                canvas->move(*sv1, x1, y1);
                canvas->move(*sv2, x2, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 > y1 && x2 < x1) {
                swap(x1,x2);
                canvas->move(*sv1, x2, y1);
                canvas->move(*sv2, x1, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            if (y2 > y1 && x2 > x1) {
                canvas->move(*sv1, x1, y1);
                canvas->move(*sv2, x2, y1+h*0.5);
                canvas->move(*sh1, x1, y1+h*0.5);
                return;
            }
            return;
        }
    }
}

VRGuiSemantics::ConceptWidget::ConceptWidget(VRGuiSemantics* m, Gtk::Fixed* canvas, VRConceptPtr concept) : BaseWidget(m, canvas) {
    this->concept = concept;
    label->set_text(concept->getName());

    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );
    for (auto p : concept->properties) {
        setPropRow(liststore->append(), p.second->getName(), p.second->type, "black", 0);
    }
}

void VRGuiSemantics::ConceptWidget::on_edit_prop_clicked() {
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

void VRGuiSemantics::ConceptWidget::on_rem_prop_clicked() {
    if (!selected_property) return;
    bool b = askUser("Delete property " + selected_property->getName() + "?", "Are you sure you want to delete this property?");
    if (!b) return;
    concept->remProperty(selected_property);
    selected_property = 0;
    update();
}

void VRGuiSemantics::ConceptWidget::on_rem_clicked() {
    bool b = askUser("Delete concept " + label->get_text() + "?", "Are you sure you want to delete this concept?");
    if (b) manager->remConcept(this);
}

void VRGuiSemantics::ConceptWidget::on_edit_clicked() {
    string s = askUserInput("Rename concept " + label->get_text() + ":");
    if (s == "") return;
    manager->current->renameConcept(concept, s);
    label->set_text(concept->getName());
}

void VRGuiSemantics::ConceptWidget::on_newp_clicked() {
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

void VRGuiSemantics::ConceptWidget::on_select_property() {
    Gtk::TreeModel::iterator iter = treeview->get_selection()->get_selected();
    if (!iter) return;

    VRGuiSemantics_PropsColumns cols;
    Gtk::TreeModel::Row row = *iter;
    int flag = row.get_value(cols.flag);
    selected_property = flag ? 0 : concept->getProperty( row.get_value(cols.prop) );
    treeview->get_selection()->unselect_all(); // clear selection
    update();
}

void VRGuiSemantics::ConceptWidget::update() {
    Glib::RefPtr<Gtk::ListStore> liststore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic( treeview->get_model() );

    liststore->clear();
    for (auto p : concept->properties) {
        Gtk::TreeModel::iterator i = liststore->append();
        if (selected_property && p.second->getName() == selected_property->getName())
            setPropRow(i, p.second->getName(), p.second->type, "green", 1);
        else setPropRow(i, p.second->getName(), p.second->type, "black", 0);
    }
}

void VRGuiSemantics::ConceptWidget::on_new_clicked() { manager->copyConcept(this); }

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
}

void VRGuiSemantics::on_del_clicked() {
    auto mgr = getManager();
    if (!mgr) return;
    mgr->remOntology(current);
}

void VRGuiSemantics::clearCanvas() {
    for (auto c : canvas->get_children()) canvas->remove(*c);
    canvas->show_all();
}

void VRGuiSemantics::updateLayout() {
    VRGraphLayout layout;
    layout.setAlgorithm(VRGraphLayout::SPRINGS, 0);
    layout.setAlgorithm(VRGraphLayout::OCCUPANCYMAP, 1);
    graph<Vec3f>& g = layout.getGraph();
    layout.setGravity(Vec3f(0,1,0));
    layout.setRadius(100);

    map<string, int> conceptIDs;

    for (auto c : widgets) {
        Vec3f p = Vec3f(c.second->pos[0], c.second->pos[1], 0);
        conceptIDs[c.first] = g.addNode(p);
        if (c.first == "Thing") layout.fixNode(conceptIDs[c.first]);
    }

    for (auto _c : widgets) {
        ConceptWidgetPtr c = dynamic_pointer_cast<ConceptWidget>(_c.second);
        for (auto c2 : c->concept->children) { // parent child connection
            g.connect(conceptIDs[_c.first], conceptIDs[c2.second->getName()], graph<Vec3f>::HIERARCHY);
            for (auto c3 : c->concept->children) { // sibling connection, TODO: use occupancy map!!
                if (c2 != c3)
                    g.connect(conceptIDs[c2.second->getName()], conceptIDs[c3.second->getName()], graph<Vec3f>::SIBLING);
            }
        }
    }

    layout.compute(1, 0.002);

    int i = 0;
    for (auto c : widgets) {
        Vec3f& p = g.getNodes()[i];
        c.second->move(Vec2f(p[0], p[1]));
        i++;
    }

    for (auto c : connectors) {
        c.second->update();
    }

    canvas->show_all();
}

void VRGuiSemantics::setOntology(string name) {
    auto mgr = getManager();
    if (mgr == 0) return;
    current = mgr->getOntology(name);
    updateCanvas();
}

void VRGuiSemantics::updateCanvas() {
    int i = 0;
    widgets.clear();

    function<void(VRConceptPtr,int,int,ConceptWidgetPtr)> travConcepts = [&](VRConceptPtr c, int cID, int lvl, ConceptWidgetPtr cp) {
        auto cw = ConceptWidgetPtr( new ConceptWidget(this, canvas, c) );
        widgets[c->getName()] = cw;

        int x = 10+cID*60;
        int y = 10+40*lvl;
        cw->move(Vec2f(x,y));

        if (lvl > 0) {
            auto co = ConnectorWidgetPtr( new ConnectorWidget(canvas) );
            connectors[c->getName()] = co;
            co->set(cp, cw);
        }

        i++;
        int child_i = 0;
        for (auto ci : c->children) { travConcepts(ci.second, child_i, lvl+1, cw); child_i++; }
    };

    if (current) travConcepts(current->thing, 0, 0, 0);
    canvas->show_all();
}

void VRGuiSemantics::on_treeview_select() {
    setToolButtonSensitivity("toolbutton15", true);

    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview16"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();

    clearCanvas();
    if (!iter) return;

    // get selection
    VRGuiSemantics_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    string type = row.get_value(cols.type);
    if (type == "section") return;

    setOntology(name);
}

void VRGuiSemantics_on_name_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_name, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview16"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // get selected socket
    VRGuiSemantics_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    row[cols.name] = new_name;
    auto scene = VRSceneManager::getCurrent();
    if (scene) return scene->getSemanticManager()->renameOntology(name, new_name);
}

VRSemanticManagerPtr VRGuiSemantics::getManager() {
    auto scene = VRSceneManager::getCurrent();
    if (scene) return scene->getSemanticManager();
    return 0;
}

void VRGuiSemantics_on_notebook_switched(GtkNotebook* notebook, GtkNotebookPage* page, guint pageN, gpointer data) {
    //if (pageN == 4) update();
}

void VRGuiSemantics_on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, const Gtk::SelectionData& data, guint info, guint time, VRGuiSemantics* self) {
    if (data.get_target() != "concept") { cout << "VRGuiSemantics_on_drag_data_received, wrong dnd: " << data.get_target() << endl; return; }
    VRGuiSemantics::ConceptWidget* e = *(VRGuiSemantics::ConceptWidget**)data.get_data();
    e->move(Vec2f(x,y));
}

VRGuiSemantics::VRGuiSemantics() {
    VRGuiBuilder()->get_widget("onto_visu", canvas);
    setToolButtonCallback("toolbutton14", sigc::mem_fun(*this, &VRGuiSemantics::on_new_clicked));
    setToolButtonCallback("toolbutton15", sigc::mem_fun(*this, &VRGuiSemantics::on_del_clicked));
    setTreeviewSelectCallback("treeview16", sigc::mem_fun(*this, &VRGuiSemantics::on_treeview_select) );
    setCellRendererCallback("cellrenderertext51", VRGuiSemantics_on_name_edited);
    setNoteBookCallback("notebook3", VRGuiSemantics_on_notebook_switched);
    setToolButtonSensitivity("toolbutton15", false);

    // dnd
    vector<Gtk::TargetEntry> entries;
    entries.push_back(Gtk::TargetEntry("concept", Gtk::TARGET_SAME_APP));
    canvas->drag_dest_set(entries, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_MOVE);
    canvas->signal_drag_data_received().connect( sigc::bind<VRGuiSemantics*>( sigc::ptr_fun(VRGuiSemantics_on_drag_data_received), this ) );

    // layout update cb
    auto sm = VRSceneManager::get();
    updateLayoutCb = VRFunction<int>::create("layout_update", boost::bind(&VRGuiSemantics::updateLayout, this));
    sm->addUpdateFkt(updateLayoutCb);
}

void VRGuiSemantics::updateOntoList() {
    // update script list
    Glib::RefPtr<Gtk::TreeStore> store = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("onto_list"));
    store->clear();

    auto setRow = [&](Gtk::TreeIter itr, string name, string type) {
        Gtk::TreeStore::Row row = *itr;
        gtk_tree_store_set(store->gobj(), row.gobj(), 0, name.c_str(), -1);
        gtk_tree_store_set(store->gobj(), row.gobj(), 1, type.c_str(), -1);
    };

    auto mgr = getManager();
    if (mgr == 0) return;

    auto itr_own = store->append();
    auto itr_lib = store->append();
    setRow( itr_own, "Custom", "section");
    setRow( itr_lib, "Library", "section");

    for (auto o : mgr->getOntologies()) {
        Gtk::TreeIter itr;
        if (o->getFlag() == "built-in") itr = store->append(itr_lib->children());
        if (o->getFlag() == "custom") itr = store->append(itr_own->children());
        setRow( itr, o->getName(), o->getFlag());
    }

    clearCanvas();
}

void VRGuiSemantics::copyConcept(ConceptWidget* w) {
    auto c = current->addConcept(w->concept->getName() + "_derived", w->concept->getName());
    auto cw = ConceptWidgetPtr( new ConceptWidget(this, canvas, c) );
    widgets[c->getName()] = cw;
    cw->move(w->pos + Vec2f(90,0));
    canvas->show_all();
}

void VRGuiSemantics::remConcept(ConceptWidget* w) {
    widgets.erase(w->concept->getName());
    current->remConcept(w->concept);
}

OSG_END_NAMESPACE;
