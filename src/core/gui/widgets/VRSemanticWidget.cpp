#include "VRSemanticWidget.h"
#include "../VRGuiSemantics.h"

#include <gtkmm/object.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/stock.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/fixed.h>
#include <gtkmm/expander.h>

using namespace OSG;

void VRGuiSemantics_on_drag_data_get(const Glib::RefPtr<Gdk::DragContext>& context, Gtk::SelectionData& data, unsigned int info, unsigned int time, VRSemanticWidget* e) {
    data.set("concept", 0, (const guint8*)&e, sizeof(void*));
}

VRSemanticWidget::VRSemanticWidget(VRGuiSemantics* m, Gtk::Fixed* canvas) {
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
    toolbar->set_icon_size(Gtk::ICON_SIZE_MENU);
    toolbar->set_show_arrow(0);
    auto bConceptRem = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::CLOSE) ); // Gtk::Stock::MEDIA_RECORD
    auto bConceptName = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::EDIT) );
    auto bPropRem = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::DELETE) ); // Gtk::Stock::MEDIA_RECORD
    auto bPropNew = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::NEW) );
    auto bPropEdit = Gtk::manage( new Gtk::ToolButton(Gtk::Stock::EDIT) );
    toolbar->add(*bConceptName);
    toolbar->add(*bConceptRem);
    auto sep = Gtk::manage( new Gtk::SeparatorToolItem() );
    toolbar->add(*sep);
    toolbar->add(*bPropNew);
    toolbar->add(*bPropEdit);
    toolbar->add(*bPropRem);

    auto toolbar2 = Gtk::manage( new Gtk::Toolbar() );
    toolbar2->set_icon_size(Gtk::ICON_SIZE_MENU);
    toolbar2->set_show_arrow(0);
    toolbar2->set_toolbar_style(Gtk::TOOLBAR_TEXT);
    auto bConceptNew = Gtk::manage( new Gtk::ToolButton("C") );
    auto bEntityNew = Gtk::manage( new Gtk::ToolButton("E") );
    auto bRuleNew = Gtk::manage( new Gtk::ToolButton("R") );
    sep = Gtk::manage( new Gtk::SeparatorToolItem() );
    toolbar2->add(*sep);
    toolbar2->add(*bConceptNew);
    toolbar2->add(*bEntityNew);
    toolbar2->add(*bRuleNew);

    bConceptName->set_tooltip_text("edit concept name");
    bConceptRem->set_tooltip_text("remove concept");
    bPropNew->set_tooltip_text("new property");
    bPropRem->set_tooltip_text("remove selected property");

    bConceptNew->set_tooltip_text("new concept");
    bEntityNew->set_tooltip_text("new entity");
    bRuleNew->set_tooltip_text("new rule");

    // expander and frame
    auto vbox = Gtk::manage( new Gtk::VBox() );
    auto hbox = Gtk::manage( new Gtk::HBox() );
    auto expander = Gtk::manage( new Gtk::Expander("") );
    label = (Gtk::Label*)expander->get_label_widget();
    expander->add(*vbox);
    hbox->pack_start(*toolbar);
    hbox->pack_start(*toolbar2);
    vbox->pack_start(*hbox);
    vbox->pack_start(*treeview);
    auto frame = Gtk::manage( new Gtk::Frame() );
    frame->add(*expander);
    widget = frame;
    canvas->put(*frame, 0, 0);

    // signals
    treeview->signal_cursor_changed().connect( sigc::mem_fun(*this, &VRSemanticWidget::on_select_property) );
    bConceptNew->signal_clicked().connect( sigc::mem_fun(*this, &VRSemanticWidget::on_new_clicked) );
    bConceptName->signal_clicked().connect( sigc::mem_fun(*this, &VRSemanticWidget::on_edit_clicked) );
    bConceptRem->signal_clicked().connect( sigc::mem_fun(*this, &VRSemanticWidget::on_rem_clicked) );
    bPropNew->signal_clicked().connect( sigc::mem_fun(*this, &VRSemanticWidget::on_newp_clicked) );
    bPropEdit->signal_clicked().connect( sigc::mem_fun(*this, &VRSemanticWidget::on_edit_prop_clicked) );
    bPropRem->signal_clicked().connect( sigc::mem_fun(*this, &VRSemanticWidget::on_rem_prop_clicked) );

    // dnd
    vector<Gtk::TargetEntry> entries;
    entries.push_back(Gtk::TargetEntry("concept", Gtk::TARGET_SAME_APP));
    expander->drag_source_set(entries, Gdk::BUTTON1_MASK, Gdk::ACTION_MOVE);
    expander->signal_drag_data_get().connect( sigc::bind<VRSemanticWidget*>( sigc::ptr_fun(VRGuiSemantics_on_drag_data_get), this ) );
}

VRSemanticWidget::~VRSemanticWidget() {
    if (widget->get_parent() == canvas) {
        canvas->remove(*widget);
        canvas->show_all();
    }
}

void VRSemanticWidget::setPropRow(Gtk::TreeModel::iterator iter, string name, string type, string color, int flag) {
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

bool VRSemanticWidget::on_expander_clicked(GdkEventButton* e) {
    cout << "EXPAND\n";
    return true;
}

void VRSemanticWidget::on_select() {

}

void VRSemanticWidget::move(Vec2f p) {
    pos = p;
    float w = widget->get_width();
    float h = widget->get_height();
    canvas->move(*widget, p[0]-w*0.5, p[1]-h*0.5);
}

Vec2f VRSemanticWidget::getAnchorPoint(Vec2f p) {
    float w = abs(p[0]-pos[0]);
    float h = abs(p[1]-pos[1]);
    if (w >= h && p[0] < pos[0]) return pos - Vec2f(widget->get_width()*0.5, 0);
    if (w >= h && p[0] > pos[0]) return pos + Vec2f(widget->get_width()*0.5, 0);
    if (w < h && p[1] < pos[1]) return pos - Vec2f(0, widget->get_height()*0.5);
    if (w < h && p[1] > pos[1]) return pos + Vec2f(0, widget->get_height()*0.5);
    return pos;
}
