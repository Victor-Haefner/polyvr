#include "VRSemanticWidget.h"
#include "VRConceptWidget.h"
#include "VREntityWidget.h"
#include "VRRuleWidget.h"
#include "VRConnectorWidget.h"
#include "../VRGuiUtils.h"
#include "../VRGuiSemantics.h"
#include "../wrapper/VRGuiTreeView.h"

#include <gtk/gtk.h>

using namespace OSG;

namespace PL = std::placeholders;

const GdkAtom CONCEPT_ATOM = GdkAtom("concept");

void VRSemanticWidget_on_drag_data_get(GdkDragContext* context, GtkSelectionData* data, unsigned int info, unsigned int time, VRSemanticWidget* e) {
    gtk_selection_data_set(data, CONCEPT_ATOM, 0, (const guint8*)&e, sizeof(void*));
}

void VRSemanticWidget_on_drag_data_received(GdkDragContext* context, int x, int y, GtkSelectionData* data, guint info, guint time, VRSemanticWidget* self) {
    GdkAtom target = gtk_selection_data_get_target(data);
    if (target != CONCEPT_ATOM) { cout << "VRGuiSemantics_on_drag_data_received, wrong dnd: " << (const char*)target << endl; return; }
    VRSemanticWidget* e = *(VRSemanticWidget**)gtk_selection_data_get_data(data);
    self->reparent(e->ptr());
}

VRSemanticWidget::VRSemanticWidget(VRGuiSemantics* m, GtkFixed* canvas, string color) : VRCanvasWidget(canvas) {
    manager = m;

    // properties treeview
    //  name, type, prop, ptype, flag, rtype, ID
    auto treestore = gtk_tree_store_new(7, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
    treeview = GTK_TREE_VIEW( gtk_tree_view_new_with_model(GTK_TREE_MODEL(treestore)) );
    gtk_tree_view_set_headers_visible(treeview, false);
    function<void(void)> fkt = bind(&VRSemanticWidget::on_select_property, this);
    connect_signal(treeview, fkt, "cursor_changed", false);

    auto addMarkupColumn = [&](string title, int cID, bool editable = false) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        setBoolProperty(G_OBJECT(renderer), "editable", editable);

        GtkTreeViewColumn* column = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(column, title.c_str());
        gtk_tree_view_column_pack_start(column, renderer, true);
        gtk_tree_view_column_add_attribute(column, renderer, "markup", cID);
        gtk_tree_view_append_column(treeview, column);
    };

    addMarkupColumn(" Properties:", 0, true);
    addMarkupColumn("", 1);

    // toolbars
    toolbar1 = GTK_TOOLBAR( gtk_toolbar_new() ); // first toolbar is inside the expand widget
    gtk_toolbar_set_icon_size(toolbar1, GTK_ICON_SIZE_MENU);
    gtk_toolbar_set_show_arrow(toolbar1, false);
    auto toolbar2 = GTK_TOOLBAR( gtk_toolbar_new() ); // second toolbar is smaller and next to the widget name
    gtk_toolbar_set_icon_size(toolbar2, GTK_ICON_SIZE_MENU);
    gtk_toolbar_set_show_arrow(toolbar2, false);

    auto addButton = [&](string icon, string tooltip, void (VRSemanticWidget::* cb)(), bool redundant) {
        auto b = gtk_tool_button_new_from_stock(icon.c_str());
        gtk_container_add(GTK_CONTAINER(toolbar2), GTK_WIDGET(b));
        gtk_widget_set_tooltip_text(GTK_WIDGET(b), tooltip.c_str());
        function<void(void)> fkt = bind(cb, this);
        connect_signal(b, fkt, "clicked", false);
        if (redundant) {
            auto b = gtk_tool_button_new_from_stock(icon.c_str());
            gtk_container_add(GTK_CONTAINER(toolbar1), GTK_WIDGET(b));
            gtk_widget_set_tooltip_text(GTK_WIDGET(b), tooltip.c_str());
            connect_signal(b, fkt, "clicked", false);
        }
        return b;
    };

    bFold = addButton("gtk-add", "Fold/unfold subtree", &VRSemanticWidget::on_fold_clicked, true); // Gtk::Stock::REMOVE
    addButton("gtk-edit", "edit concept name", &VRSemanticWidget::on_edit_clicked, true);
    addButton("gtk-close", "remove concept", &VRSemanticWidget::on_rem_clicked, true);
    auto sep = gtk_separator_tool_item_new();
    gtk_container_add(GTK_CONTAINER(toolbar2), GTK_WIDGET(sep));// separator
    addButton("gtk-new", "new property", &VRSemanticWidget::on_newp_clicked, false);
    addButton("gtk-edit", "edit selected property", &VRSemanticWidget::on_edit_prop_clicked, false);
    addButton("gtk-delete", "remove selected property", &VRSemanticWidget::on_rem_prop_clicked, false);

    // expander
    auto vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    auto expander = gtk_expander_new("");
    label = GTK_LABEL( gtk_expander_get_label_widget(GTK_EXPANDER(expander)) );
    gtk_container_add(GTK_CONTAINER(expander), GTK_WIDGET(vbox));
    toolbars = GTK_BOX( gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0) );
    gtk_box_pack_start(toolbars, GTK_WIDGET(toolbar2), 1, 1, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(toolbars), 1, 1, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(treeview), 1, 1, 0);
    auto framed = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(framed), GTK_WIDGET(expander), 1, 1, 0);
    gtk_box_pack_start(GTK_BOX(framed), GTK_WIDGET(toolbar1), 1, 1, 0);

    function<void(GParamSpec*)> fkt2 = bind(&VRSemanticWidget::on_expander_toggled, this, PL::_1);
    connect_signal(expander, fkt2, "notify::expanded", false);

    // frame
    auto frame = gtk_frame_new("");
    gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(framed));
    widget = GTK_FRAME(frame);
    gtk_fixed_put(canvas, frame, 0, 0);
    GdkColor col;
    gdk_color_parse(color.c_str(), &col);
    gtk_widget_modify_bg(frame, GTK_STATE_NORMAL, &col);
    gtk_widget_show_all(frame);

    // dnd drag widget
    GtkTargetEntry entries[] = { { "concept", GTK_TARGET_SAME_APP, 0 } };
    gtk_drag_source_set(expander, GDK_BUTTON1_MASK, entries, 1, GDK_ACTION_MOVE);
    function<void(GdkDragContext*, GtkSelectionData*, unsigned int, unsigned int)> fkt3 = bind(VRSemanticWidget_on_drag_data_get, PL::_1, PL::_2, PL::_3, PL::_4, this);
    connect_signal(expander, fkt3, "drag_data_get" );

    // dnd drop on widget
    gtk_drag_dest_set(expander, GTK_DEST_DEFAULT_ALL, entries, 1, GDK_ACTION_MOVE);

    function<void(GdkDragContext*, int, int, GtkSelectionData*, guint, guint)> fkt4 = bind(VRSemanticWidget_on_drag_data_received, PL::_1, PL::_2, PL::_3, PL::_4, PL::_5, PL::_6, this);
    connect_signal(expander, fkt4, "drag_data_received" );
}

VRSemanticWidget::~VRSemanticWidget() {}

VRSemanticWidgetPtr VRSemanticWidget::ptr() { return dynamic_pointer_cast<VRSemanticWidget>(shared_from_this()); }

void VRSemanticWidget::reparent(VRSemanticWidgetPtr w) {
    auto c = dynamic_pointer_cast<VRConceptWidget>(w);
    auto e = dynamic_pointer_cast<VREntityWidget>(w);
    auto r = dynamic_pointer_cast<VRRuleWidget>(w);
    if (c) reparent(c);
    if (e) reparent(e);
    if (r) reparent(r);
}

void VRSemanticWidget::setPropRow(GtkTreeIter* iter, string name, string type, string color, int flag, int ID, int rtype) {
    string cname = "<span color=\""+color+"\">" + name + "</span>";
    string ctype = "<span color=\""+color+"\">" + type + "</span>";

    GtkTreeStore* treestore = GTK_TREE_STORE( gtk_tree_view_get_model(treeview) );

    gtk_tree_store_set(treestore, iter, 0, cname.c_str(), -1);
    gtk_tree_store_set(treestore, iter, 1, ctype.c_str(), -1);
    gtk_tree_store_set(treestore, iter, 2, name.c_str(), -1);
    gtk_tree_store_set(treestore, iter, 3, type.c_str(), -1);
    gtk_tree_store_set(treestore, iter, 4, flag, -1);
    gtk_tree_store_set(treestore, iter, 5, rtype, -1);
    gtk_tree_store_set(treestore, iter, 6, ID, -1);
}

void VRSemanticWidget::on_expander_toggled(GParamSpec* param_spec) {
    if (expanded) gtk_widget_show_all(GTK_WIDGET(toolbar1));
    else gtk_widget_hide(GTK_WIDGET(toolbar1));
    expanded = !expanded;
}

void VRSemanticWidget::on_select() {}

void VRSemanticWidget::on_fold_clicked() {
    subTreeFolded = !subTreeFolded;
    if (subTreeFolded) gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(bFold), "gtk-add");
    else gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(bFold), "gtk-remove");
    setFolding(subTreeFolded);
}
