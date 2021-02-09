#include <gtk/gtk.h>

#include "VRGuiBuilder.h"
#include "VRGuiFile.h"
#include "glarea/glarea.h"
#include "core/utils/system/VRSystem.h"

#include <iostream>

VRGuiBuilder::VRGuiBuilder() {}
VRGuiBuilder::~VRGuiBuilder() {}

string getDefaultPath(bool standalone) {
	if (standalone) return "ressources/gui/VRDirector_min.glade";
#if GTK_MAJOR_VERSION == 2
	return "ressources/gui/VRDirector.glade";
#else
	return "ressources/gui/VRDirector3.glade";
#endif
}

VRGuiBuilder* VRGuiBuilder::get(bool standalone) {
	static VRGuiBuilder* b = 0;

	if (!b) {
        b = new VRGuiBuilder();
        if (!standalone) b->buildBaseUI();
        else b->buildMinimalUI();
	}

    return b;
}

void VRGuiBuilder::read(string path) {
    //XML xml;
    //xml.read(path);
    cout << "VRGuiBuilder::read " << path << endl;
    builder = gtk_builder_new();
    cout << " VRGuiBuilder::read file" << endl;
    GError* error = 0;
    gtk_builder_add_from_file(builder, path.c_str(), &error);
    cout << " VRGuiBuilder::read done" << endl;
    if (error) {
        cout << "  -- VRGuiBuilder read error: " << error->message << endl;
        g_clear_error(&error);
        g_error_free(error);
    }
}

void VRGuiBuilder::reg_widget(_GtkWidget* w, string name) { widgets[name] = w; }
void VRGuiBuilder::reg_object(_GtkObject* o, string name) { objects[name] = o; }

bool VRGuiBuilder::has_object(string name) { return objects.count(name); }
bool VRGuiBuilder::has_widget(string name) { return widgets.count(name); }

GtkWidget* VRGuiBuilder::get_widget(string name) {
    /*auto w = (GtkWidget*)gtk_builder_get_object(builder, name.c_str());
    if (!w) cout << " Error in VRGuiBuilder::get_widget, no widget called " << name << endl;
    return w;*/

    if (widgets.count(name)) return widgets[name];
    else {
        cout << " ! --------------------- Error in VRGuiBuilder::get_widget, no object called " << name << endl;
        printBacktrace();
        return 0;
    }
}

_GtkObject* VRGuiBuilder::get_object(string name) {
    /*auto o = (_GtkObject*)gtk_builder_get_object(builder, name.c_str());
    if (!o) cout << " Error in VRGuiBuilder::get_object, no object called " << name << endl;
    return o;*/

    if (objects.count(name)) return objects[name];
    else {
        cout << " ! --------------------- Error in VRGuiBuilder::get_object, no object called " << name << endl;
        printBacktrace();
        return 0;
    }
}

GtkWidget* addWindow(string ID, string name) {
    GtkWidget* w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    cout << "addWindow " << ID << " " << name << endl;

    VRGuiBuilder::get()->reg_widget(w, ID);
    gtk_window_set_title(GTK_WINDOW(w), name.c_str());
    return w;
}

GtkWidget* addDialog(string ID) {
    GtkWidget* w = gtk_dialog_new();
    VRGuiBuilder::get()->reg_widget(w, ID);
    return w;
}

GtkWidget* addGrid(string ID) {
    auto g = gtk_grid_new();
    VRGuiBuilder::get()->reg_widget(g, ID);
    return g;
}

GtkWidget* addTextview(string ID, string mID) {
    auto b = gtk_text_buffer_new(0);
    auto g = gtk_text_view_new_with_buffer(b);
    VRGuiBuilder::get()->reg_object(G_OBJECT(b), mID);
    VRGuiBuilder::get()->reg_widget(g, ID);
    return g;
}

GtkWidget* addSeparator(string ID, GtkOrientation o) {
    auto s = gtk_separator_new(o);
    VRGuiBuilder::get()->reg_widget(s, ID);
    return s;
}

GtkWidget* addPaned(string ID, GtkOrientation o) {
    auto p = gtk_paned_new(o);
    VRGuiBuilder::get()->reg_widget(p, ID);
    return p;
}

GtkWidget* addImage(string ID, string path) {
    auto p = gtk_image_new_from_file(path.c_str());
    VRGuiBuilder::get()->reg_widget(p, ID);
    return p;
}

GtkWidget* addStockImage(string ID, string sID, GtkIconSize iSize) {
    auto p = gtk_image_new_from_stock(sID.c_str(), iSize);
    VRGuiBuilder::get()->reg_widget(p, ID);
    return p;
}

GtkWidget* addLabel(string ID, string label) {
    auto p = gtk_label_new(label.c_str());
    VRGuiBuilder::get()->reg_widget(p, ID);
    return p;
}

GtkWidget* addScale(string ID, float min, float max, float step) {
    auto p = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step);
    VRGuiBuilder::get()->reg_widget(p, ID);
    return p;
}

GtkWidget* addSpacer(int height) {
    auto b = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    auto l = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(b), l, false, true, height*0.5);
    return b;
}

GtkWidget* addFixed(string ID) {
    auto p = gtk_fixed_new();
    VRGuiBuilder::get()->reg_widget(p, ID);
    return p;
}

GtkWidget* addToolbar(string ID, GtkIconSize iSize, GtkOrientation o) {
    auto p = gtk_toolbar_new();
    VRGuiBuilder::get()->reg_widget(p, ID);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(p), iSize);
    gtk_toolbar_set_style(GTK_TOOLBAR(p), GTK_TOOLBAR_ICONS);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(p), o);
    return p;
}

GtkToolItem* addToolButton(string ID, string stock, GtkWidget* bar, string tooltip) {
    auto item = gtk_tool_button_new_from_stock(stock.c_str());
    VRGuiBuilder::get()->reg_widget(GTK_WIDGET(item), ID);
    gtk_toolbar_insert(GTK_TOOLBAR(bar), item, -1);
    gtk_widget_set_tooltip_text(GTK_WIDGET(item), tooltip.c_str());
    return item;
}

GtkToolItem* addToggleToolButton(string ID, string stock, GtkWidget* bar, string tooltip) {
    auto item = gtk_toggle_tool_button_new_from_stock(stock.c_str());
    VRGuiBuilder::get()->reg_widget(GTK_WIDGET(item), ID);
    gtk_toolbar_insert(GTK_TOOLBAR(bar), item, -1);
    gtk_widget_set_tooltip_text(GTK_WIDGET(item), tooltip.c_str());
    return item;
}

GtkWidget* addNotebook(string ID) {
    auto n = gtk_notebook_new();
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

void add1ToPaned(GtkWidget* p, GtkWidget* w) {
    //GtkWidget* v = gtk_scrolled_window_new(0,0);
    //gtk_container_add(GTK_CONTAINER(v), w);
    /*if (useLayout) {
        GtkWidget* v = gtk_layout_new(0,0);
        gtk_container_add(GTK_CONTAINER(v), w);
        w = v;
    }*/
    gtk_paned_pack1(GTK_PANED(p), w, true, true);
}

// TODO: wrong signature
void onPanedMove(GtkPaned* widget, GdkEvent* event, GtkWidget* content) {
    int p = gtk_paned_get_position(widget);
    int h = gtk_widget_get_allocated_height(GTK_WIDGET(widget));
    gtk_widget_set_size_request(content, p, h);
}

void add2ToPaned(GtkWidget* p, GtkWidget* w) {
    gtk_paned_pack2(GTK_PANED(p), w, true, true);
}

GtkWidget* addBox(string ID, GtkOrientation o) {
    auto n = gtk_box_new(o, 0);
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addButtonBox(string ID, GtkOrientation o) {
    auto n = gtk_button_box_new(o);
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addButton(string ID, string label) {
    auto n = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(n), label.c_str());
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addImgButton(string ID, string stockID) {
    auto n = gtk_button_new();
    auto img = gtk_image_new_from_stock(stockID.c_str(), GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(n), img);
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addRadiobutton(string ID, string label, GtkWidget* groupWidget) {
    GtkWidget* n = 0;
    if (groupWidget == 0) n = gtk_radio_button_new_with_label(NULL, label.c_str());
    else n = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(groupWidget), label.c_str());
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addEntry(string ID, int Nwidth = 0) {
    auto n = gtk_entry_new();
    VRGuiBuilder::get()->reg_widget(n, ID);
    if (Nwidth > 0) {
        gtk_entry_set_width_chars(GTK_ENTRY(n), Nwidth);
        gtk_widget_set_halign(n, GTK_ALIGN_CENTER);
    }
    return n;
}

GtkWidget* addDrawingArea(string ID) {
    auto n = gtk_drawing_area_new();
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addColorChooser(string ID) {
    auto f = gtk_frame_new(0);
    auto n = gtk_drawing_area_new();
    VRGuiBuilder::get()->reg_widget(n, ID);
    gtk_container_add(GTK_CONTAINER(f), n);
    gtk_widget_set_size_request(n, 50, 30);
    gtk_widget_set_halign(f, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(f, GTK_ALIGN_CENTER);
    return f;
}

GtkWidget* addCombobox(string ID, string mID) {
    GtkTreeModel* m = 0;
    if (VRGuiBuilder::get()->has_object(mID)) m = GTK_TREE_MODEL(VRGuiBuilder::get()->get_object(mID));
    else m = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING, -1));

    auto n = gtk_combo_box_new_with_model_and_entry(m);
    //auto r = gtk_cell_renderer_text_new();
    gtk_combo_box_set_id_column(GTK_COMBO_BOX(n), 0);
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(n), 0);
    gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(n), GTK_SENSITIVITY_ON);
    VRGuiBuilder::get()->reg_widget(n, ID);
    VRGuiBuilder::get()->reg_object(G_OBJECT(m), mID);
    return n;
}

GtkWidget* addScrolledWindow(string ID) {
    auto n = gtk_scrolled_window_new(0, 0);
    VRGuiBuilder::get()->reg_widget(n, ID);
    gtk_widget_set_hexpand(n, true);
    gtk_widget_set_vexpand(n, true);
    return n;
}

GtkWidget* addViewport(string ID) {
    auto n = gtk_viewport_new(0, 0);
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addCheckbutton(string ID, string label) {
    auto n = gtk_check_button_new_with_label(label.c_str());
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

pair<GtkWidget*,GtkWidget*> addTreeview(string ID, string mID, GtkTreeModel* m, bool hscroll = false, bool vscroll = true) {
    auto f = gtk_frame_new(NULL);
    auto w = gtk_scrolled_window_new(0,0);
    auto n = gtk_tree_view_new_with_model(m);
    VRGuiBuilder::get()->reg_widget(n, ID);
    VRGuiBuilder::get()->reg_object(G_OBJECT(m), mID);
    gtk_container_add(GTK_CONTAINER(f), w);
    gtk_container_add(GTK_CONTAINER(w), n);
    gtk_widget_set_vexpand(f, true);
    auto hp = hscroll ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER;
    auto vp = vscroll ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER;
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w), hp, vp);
    return make_pair(n,f);
}

void on_expander_activate(GtkExpander* expander, gpointer user_data) {
    bool open = !gtk_expander_get_expanded(expander);
    GList* children = gtk_container_get_children(GTK_CONTAINER(expander));

    for (GList* elem = children; elem; elem = elem->next) {
        GtkWidget* w = GTK_WIDGET(elem->data);
        if (open) gtk_widget_show(w);
        else gtk_widget_hide(w);
    }

    // fix to allways show label
    auto lbl = gtk_expander_get_label_widget(expander);
    gtk_widget_show(lbl);
}

GtkWidget* addExpander(string ID, string label, GtkWidget* child) {
    auto f = gtk_frame_new(NULL);
    auto n = gtk_expander_new(label.c_str());
    VRGuiBuilder::get()->reg_widget(n, ID);
    gtk_container_add(GTK_CONTAINER(n), child);
    gtk_container_add(GTK_CONTAINER(f), n);
    g_signal_connect(n, "activate", (GCallback)on_expander_activate, NULL); // to fix the bug where the collapsed expander content still gets mouse signals
    return f;
}

GtkWidget* appendExpander(string ID, string label, string gID, GtkWidget* box) {
    auto g = addGrid(gID);
    auto n = addExpander(ID, label, g);
    gtk_box_pack_start(GTK_BOX(box), n, false, true, 0);
    return g;
}

void addNotebookPage(GtkWidget* notebook, GtkWidget* content, string label) {
    auto lbl = gtk_label_new(label.c_str());
    gtk_widget_set_size_request(lbl, 1, -1);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content, lbl);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), true);

    /*auto i = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content, lbl);
    auto page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
    //gtk_widget_set_hexpand(page, true);
    //gtk_widget_set_vexpand(page, true);
    GValue v = G_VALUE_INIT;
    g_value_init(&v, G_TYPE_BOOLEAN);
    g_value_set_boolean(&v, true);
    g_object_set_property(G_OBJECT(page), "tab-fill", &v);*/
}

void setStringProperty(GtkWidget* w, string p, string v) {
    GValue V = G_VALUE_INIT;
    g_value_init(&V, G_TYPE_STRING);
    g_value_set_string(&V, v.c_str());
    g_object_set_property(G_OBJECT(w), p.c_str(), &V);
}

void setBoolProperty(GtkWidget* w, string p, bool v) {
    GValue V = G_VALUE_INIT;
    g_value_init(&V, G_TYPE_BOOLEAN);
    g_value_set_boolean(&V, v);
    g_object_set_property(G_OBJECT(w), p.c_str(), &V);
}

GtkCellRenderer* addCellrenderer(string ID, GtkTreeViewColumn* c, bool editable = false) {
    auto r = gtk_cell_renderer_text_new();
    VRGuiBuilder::get()->reg_widget((GtkWidget*)r, ID);
    gtk_tree_view_column_pack_start(c, r, true);
    if (editable) setBoolProperty((GtkWidget*)r, "editable", true);
    return r;
}

GtkCellRenderer* addImgCellrenderer(string ID, GtkTreeViewColumn* c) {
    auto r = gtk_cell_renderer_pixbuf_new();
    VRGuiBuilder::get()->reg_widget((GtkWidget*)r, ID);
    gtk_tree_view_column_pack_start(c, r, true);
    return r;
}

GtkCellRenderer* addToggleCellrenderer(string ID, GtkTreeViewColumn* c) {
    auto r = gtk_cell_renderer_toggle_new();
    VRGuiBuilder::get()->reg_widget((GtkWidget*)r, ID);
    gtk_tree_view_column_pack_start(c, r, true);
    return r;
}

GtkTreeViewColumn* addTreecolumn(string ID, string title) {
    auto c = gtk_tree_view_column_new();
    VRGuiBuilder::get()->reg_widget((GtkWidget*)c, ID);
    gtk_tree_view_column_set_title(c, title.c_str());
    return c;
}

void addTreeviewTextcolumn(GtkWidget* treeview, string cName, string rID, int pos, bool editable = false) {
    auto c = addTreecolumn(cName+"ID", cName);
    auto r = addCellrenderer(rID, c, editable);
    gtk_tree_view_column_add_attribute(c, r, "text", pos);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c);
}

GtkWidget* addVectorFrame(string ID, string fID) {
    auto f = gtk_frame_new("");
    VRGuiBuilder::get()->reg_widget(f, ID);
    auto F = addFixed(fID);
    gtk_container_add(GTK_CONTAINER(f), F);
    return f;
}

GtkWidget* addGLWidget() {
    //auto glarea = gtk_drawing_area_new();
    //auto glarea = gtk_gl_area_new();
    auto glarea = gl_area_new();
    VRGuiBuilder::get()->reg_widget(glarea, "glarea");
    gtk_widget_set_hexpand(glarea, true);
    gtk_widget_set_vexpand(glarea, true);
    return glarea;
}

void VRGuiBuilder::buildMinimalUI() {
    cout << "VRGuiBuilder buildMinimalUI.." << endl;
    auto window1 = addWindow("window1", "PolyVR");
    auto a_vbox = addBox("a_vbox", GTK_ORIENTATION_VERTICAL);
    auto glarea = addGLWidget();
    gtk_box_pack_start(GTK_BOX(a_vbox), glarea, false, true, 0);
    gtk_container_add(GTK_CONTAINER(window1), a_vbox);
    gtk_widget_set_hexpand(a_vbox, true);
    gtk_widget_set_vexpand(a_vbox, true);
    cout << " ..building all widgets done!" << endl;
}

gboolean on_window_expose(GtkWidget* widget, GdkEventExpose* event) {
    cout << " --- on_window_expose ---" << endl;
    return FALSE;
}

void VRGuiBuilder::buildBaseUI() {
    auto window1 = addWindow("window1", "PolyVR");
    auto main_frame = addGrid("main_frame");
    gtk_container_add(GTK_CONTAINER(window1), main_frame);
    gtk_window_set_icon_from_file(GTK_WINDOW(window1), "ressources/gui/logo_icon.png", 0);

    cout << " build head section" << endl;
    /* ---------- head section ---------------------- */
    auto table20 = addGrid("table20");
    auto hseparator1 = addSeparator("hseparator1", GTK_ORIENTATION_HORIZONTAL);
    auto hpaned1 = addPaned("hpaned1", GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(main_frame), table20, 0,0,2,1);
    gtk_grid_attach(GTK_GRID(main_frame), hseparator1, 0,1,2,1);
    gtk_grid_attach(GTK_GRID(main_frame), hpaned1, 0,2,2,1);
    gtk_widget_set_vexpand(hpaned1, true);
    gtk_paned_set_position(GTK_PANED(hpaned1), 210);
    gtk_paned_set_wide_handle(GTK_PANED(hpaned1), true);

    auto banner = addImage("banner", "ressources/gui/logo4.png");
    auto label13 = addLabel("label13", "VR Setup:");
    auto label24 = addLabel("label24", "Scene:");
    auto fixed41 = addFixed("fixed41");
    auto toolbar1 = addToolbar("toolbar1", GTK_ICON_SIZE_DIALOG, GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(table20), toolbar1, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table20), label13, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table20), fixed41, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table20), label24, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table20), banner, 2,0,1,2);
    gtk_widget_set_hexpand(label13, true);
    gtk_widget_set_hexpand(label24, true);

    auto toolbutton1 = addToolButton("toolbutton1", "gtk-new", toolbar1, "New Scene");
    auto toolbutton21 = addToolButton("toolbutton21", "gtk-open", toolbar1, "Open Scene or Model");
    auto toolbutton4 = addToolButton("toolbutton4", "gtk-save", toolbar1, "Save Scene");
    auto toolbutton5 = addToolButton("toolbutton5", "gtk-save-as", toolbar1, "Save Scene as New File");
    auto toolbutton50 = addToolButton("toolbutton50", "gtk-go-up", toolbar1, "Deploy");
    auto toolbutton28 = addToolButton("toolbutton28", "gtk-stop", toolbar1, "Close Scene");
    auto toolbutton3 = addToolButton("toolbutton3", "gtk-quit", toolbar1, "Quit PolyVR");
    auto toolbutton17 = addToolButton("toolbutton17", "gtk-about", toolbar1, "About");
    auto toolbutton18 = addToolButton("toolbutton18", "gtk-paste", toolbar1, "Profiler");
    auto toolbutton26 = addToolButton("toolbutton26", "gtk-fullscreen", toolbar1, "Fullscreen");

    cout << " build core section" << endl;
    /* ---------- core section ---------------------- */
    auto notebook1 = addNotebook("notebook1");
    auto vpaned1 = addPaned("vpaned1", GTK_ORIENTATION_VERTICAL);
    auto vbox5 = addBox("vbox5", GTK_ORIENTATION_VERTICAL);
    auto hbox15 = addBox("hbox15", GTK_ORIENTATION_HORIZONTAL);
    add2ToPaned(hpaned1, vpaned1);
    add1ToPaned(vpaned1, vbox5);
    add2ToPaned(vpaned1, hbox15);
    gtk_widget_set_hexpand(notebook1, true);
    gtk_widget_set_vexpand(vbox5, true);
    gtk_paned_set_position(GTK_PANED(vpaned1), 120);
    gtk_paned_set_wide_handle(GTK_PANED(vpaned1), true);

    GtkWidget* layout = gtk_layout_new(0,0);
    GtkWidget* viewport = gtk_viewport_new(0,0);
    gtk_container_add(GTK_CONTAINER(viewport), notebook1);
    gtk_container_add(GTK_CONTAINER(layout), viewport);
    gtk_paned_pack1(GTK_PANED(hpaned1), layout, true, true);
    g_signal_connect(hpaned1, "notify::position", (GCallback)onPanedMove, notebook1);

    /* ---------- right core section ---------------------- */
    auto hbox1 = addBox("hbox1", GTK_ORIENTATION_HORIZONTAL);
    auto glarea = addGLWidget();
    auto label5 = addLabel("label5", "Camera:");
    auto label45 = addLabel("label45", "Navigation:");
    auto combobox4 = addCombobox("combobox4", "cameras");
    auto combobox9 = addCombobox("combobox9", "nav_presets");
    auto hseparator5 = addSeparator("hseparator5", GTK_ORIENTATION_HORIZONTAL);
    auto hseparator6 = addSeparator("hseparator6", GTK_ORIENTATION_HORIZONTAL);
    auto toolbar6 = addToolbar("toolbar6", GTK_ICON_SIZE_LARGE_TOOLBAR, GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox5), hbox1, false, true, 0);
    gtk_box_pack_start(GTK_BOX(vbox5), glarea, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), label5, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), combobox4, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), hseparator5, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), label45, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), combobox9, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), hseparator6, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), toolbar6, false, true, 0);

    auto togglebutton1 = addToggleToolButton("togglebutton1", "gtk-leave-fullscreen", toolbar6, "Undock 3D View");

    auto toolbar7 = addToolbar("toolbar7", GTK_ICON_SIZE_LARGE_TOOLBAR, GTK_ORIENTATION_VERTICAL);
    auto toolbutton24 = addToolButton("toolbutton24", "gtk-clear", toolbar7, "Clear Consoles");
    auto toolbutton25 = addToolButton("toolbutton25", "gtk-go-down", toolbar7, "Go to Bottom");
    auto network_verbose = addToggleToolButton("network_verbose", "gtk-network", toolbar7, "Show Network Logs");
    auto pause_terminal = addToggleToolButton("pause_terminal", "gtk-media-pause", toolbar7, "Pause Console Printing");
    gtk_box_pack_end(GTK_BOX(hbox15), toolbar7, false, true, 0);

    /* ---------- left core section ---------------------- */
    auto vbox3 = addBox("vbox3", GTK_ORIENTATION_VERTICAL);
    auto table6 = addGrid("table6");
    auto notebook3 = addNotebook("notebook3");
    addNotebookPage(notebook1, vbox3, "Start");
    addNotebookPage(notebook1, table6, "VR Setup");
    addNotebookPage(notebook1, notebook3, "VR Scene");

    auto hbox16 = addBox("hbox16", GTK_ORIENTATION_HORIZONTAL);
    auto notebook2 = addNotebook("notebook2");
    auto label171 = addLabel("label171", "search:");
    auto appSearch = addEntry("appSearch", 35);
    auto scrolledwindow9 = addScrolledWindow("scrolledwindow9");
    auto scrolledwindow10 = addScrolledWindow("scrolledwindow10");
    auto viewport1 = addViewport("viewport1");
    auto viewport4 = addViewport("viewport4");
    auto favorites_tab = addGrid("favorites_tab");
    auto examples_tab = addGrid("examples_tab");
    gtk_widget_set_halign(appSearch, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox3), hbox16, false, true, 0);
    gtk_box_pack_start(GTK_BOX(vbox3), notebook2, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox16), label171, false, true, 5);
    gtk_box_pack_start(GTK_BOX(hbox16), appSearch, true, true, 0);
    addNotebookPage(notebook2, scrolledwindow9, "Favorites");
    addNotebookPage(notebook2, scrolledwindow10, "Examples");
    gtk_container_add(GTK_CONTAINER(scrolledwindow9), viewport1);
    gtk_container_add(GTK_CONTAINER(scrolledwindow10), viewport4);
    gtk_container_add(GTK_CONTAINER(viewport1), favorites_tab);
    gtk_container_add(GTK_CONTAINER(viewport4), examples_tab);

    cout << " build app launch dialog" << endl;
    /* ---------- app launch dialog ---------------------- */
    auto advanced_start = addDialog("advanced_start");
    auto dialog_vbox9 = gtk_dialog_get_content_area(GTK_DIALOG(advanced_start));
    auto dialog_action_area9 = gtk_dialog_get_action_area(GTK_DIALOG(advanced_start));
    auto button10 = addButton("button10", "Cancel");
    auto button26 = addButton("button26", "Start");
    auto vbox4 = addBox("vbox4", GTK_ORIENTATION_VERTICAL);
    auto label27 = addLabel("label27", "Start with..");
    auto checkbutton34 = addCheckbutton("checkbutton34", "all scripts disabled");
    auto checkbutton36 = addCheckbutton("checkbutton36", "only light ressources ( < 30 mb )");
    gtk_box_pack_start(GTK_BOX(dialog_action_area9), button10, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_action_area9), button26, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox9), vbox4, false, true, 0);
    gtk_box_pack_start(GTK_BOX(vbox4), label27, false, true, 0);
    gtk_box_pack_start(GTK_BOX(vbox4), checkbutton34, false, true, 0);
    gtk_box_pack_start(GTK_BOX(vbox4), checkbutton36, false, true, 0);
    gtk_window_set_transient_for(GTK_WINDOW(advanced_start), GTK_WINDOW(window1));

    cout << " build recorder dialog" << endl;
    /* ---------- recorder ---------------------- */  // TODO: to test!
    auto recorder = addDialog("recorder");
    auto dialog_vbox15 = gtk_dialog_get_content_area(GTK_DIALOG(recorder));
    auto dialog_action_area15 = gtk_dialog_get_action_area(GTK_DIALOG(recorder));
    auto recGrid = addGrid("recGrid");
    auto label149 = addLabel("label149", "Idle");
    auto labelRes = addLabel("labelRes", "resolution:");
    auto label151 = addLabel("label151", "codec:");
    auto label150 = addLabel("label150", "bitrate multiplier:");
    auto codecs = addCombobox("codecs", "codecList");
    auto resolutions = addCombobox("resolutions", "resList");
    auto doVSync = addCheckbutton("doVSyncCB", "VSync");
    auto entry27 = addEntry("entry27");
    gtk_box_pack_start(GTK_BOX(dialog_vbox15), label149, true, true, 5);
    gtk_box_pack_start(GTK_BOX(dialog_vbox15), recGrid, true, true, 0);
    gtk_grid_attach(GTK_GRID(recGrid), labelRes, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(recGrid), label151, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(recGrid), label150, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(recGrid), resolutions, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(recGrid), codecs, 1,1,2,1);
    gtk_grid_attach(GTK_GRID(recGrid), entry27, 1,2,2,1);
    gtk_grid_attach(GTK_GRID(recGrid), doVSync, 2,0,1,1);
    gtk_window_set_transient_for(GTK_WINDOW(recorder), GTK_WINDOW(window1));
    gtk_widget_show_all(dialog_vbox15);
    gtk_label_set_xalign(GTK_LABEL(labelRes), 0);
    gtk_label_set_xalign(GTK_LABEL(label151), 0);
    gtk_label_set_xalign(GTK_LABEL(label150), 0);

    cout << " build about dialog" << endl;
    /* ---------- about dialog ---------------------- */
    string cpyr =
    "Copyright © 2009-2020 IMI, KIT\n"
    "Copyright © 2009-2020 Dr.-Ing. Dipl. Phys. Victor Häfner\n"
    "Copyright © 2009-2020 Prof. Dr. Dr.-Ing. Dr. h. c. Jivka Ovtcharova";

    string cmnts =
    "A shortcut to virtual reality\n\n"
    "Virtual reality authoring system developed at\n"
    "the Institute for Information Management in Engineering (IMI)\n"
    "at the Karlsruhe Institute of Technology (KIT)\n"
    "Germany";

    auto aboutdialog1 = GTK_ABOUT_DIALOG(gtk_about_dialog_new());
    auto logo = gdk_pixbuf_new_from_file("ressources/gui/logo_icon.png", 0);
    reg_widget(GTK_WIDGET(aboutdialog1), "aboutdialog1");
    gtk_about_dialog_set_program_name(aboutdialog1, "PolyVR");
    gtk_about_dialog_set_version(aboutdialog1, "1.0");
    gtk_about_dialog_set_copyright(aboutdialog1, cpyr.c_str());
    gtk_about_dialog_set_comments(aboutdialog1, cmnts.c_str());
    gtk_about_dialog_set_website(aboutdialog1, "http://www.imi.kit.edu/46_2274.php");
    gtk_about_dialog_set_website_label(aboutdialog1, "http://www.imi.kit.edu");
    gtk_about_dialog_set_logo(aboutdialog1, logo);
    gtk_window_set_transient_for(GTK_WINDOW(aboutdialog1), GTK_WINDOW(window1));

    cout << " build internal monitor dialog" << endl;
    /* ---------- internal monitor ---------------------- */
    auto dialog2 = addDialog("dialog2");
    auto dialog_vbox10 = gtk_dialog_get_content_area(GTK_DIALOG(dialog2));
    auto dialog_action_area10 = gtk_dialog_get_action_area(GTK_DIALOG(dialog2));
    auto button21 = addButton("button21", "Close");
    auto label92 = addLabel("label92", "System monitor");
    auto notebook4 = addNotebook("notebook4");
    auto fixed18 = addFixed("fixed18");
    auto table31 = addGrid("table31");
    gtk_widget_set_size_request(dialog2, 1024, 600);
    gtk_box_pack_start(GTK_BOX(dialog_action_area10), button21, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox10), label92, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox10), notebook4, true, true, 0);
    gtk_window_set_transient_for(GTK_WINDOW(dialog2), GTK_WINDOW(window1));
    addNotebookPage(notebook4, fixed18, "General");
    addNotebookPage(notebook4, table31, "Profiler");

    auto liststore4 = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    auto treeview10_and_frame = addTreeview("treeview10", "liststore4", GTK_TREE_MODEL(liststore4));
    auto treeview10 = treeview10_and_frame.first;
    addNotebookPage(notebook4, treeview10_and_frame.second, "Name Dictionaries");
    addTreeviewTextcolumn(treeview10, "column", "cellrenderertext25", 0);
    addTreeviewTextcolumn(treeview10, "column", "cellrenderertext26", 1);

    auto table47 = addGrid("table47");
    auto scrolledwindow1 = addScrolledWindow("scrolledwindow1");
    auto viewport5 = addViewport("viewport5");
    auto profiler_area = addDrawingArea("profiler_area");
    auto label163 = addLabel("label163", "frame:");
    auto label164 = addLabel("label164", "duration:");
    auto label169 = addLabel("label169", "SG changes:");
    auto label170 = addLabel("label170", "SG created:");
    auto Nframe = addLabel("Nframe", "0");
    auto Tframe = addLabel("Tframe", "0 ms");
    auto Nchanges = addLabel("Nchanges", "0");
    auto Ncreated = addLabel("Ncreated", "0");
    auto prof_fkts = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);
    auto treeview15_and_frame = addTreeview("treeview15", "prof_fkts", GTK_TREE_MODEL(prof_fkts));
    gtk_grid_attach(GTK_GRID(table31), table47, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table31), scrolledwindow1, 1,0,1,2);
    gtk_grid_attach(GTK_GRID(table31), treeview15_and_frame.second, 0,1,1,1);
    gtk_container_add(GTK_CONTAINER(scrolledwindow1), viewport5);
    gtk_container_add(GTK_CONTAINER(viewport5), profiler_area);
    gtk_grid_attach(GTK_GRID(table47), label163, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table47), label164, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table47), label169, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table47), label170, 0,3,1,1);
    gtk_grid_attach(GTK_GRID(table47), Nframe, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table47), Tframe, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table47), Nchanges, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table47), Ncreated, 1,3,1,1);
    gtk_widget_set_hexpand(scrolledwindow1, true);
    gtk_widget_set_hexpand(profiler_area, true);
    gtk_widget_set_vexpand(profiler_area, true);

    auto treeview15 = treeview15_and_frame.first;
    gtk_widget_set_vexpand(treeview15, true);
    addTreeviewTextcolumn(treeview15, "function", "cellrenderertext30", 0);
    addTreeviewTextcolumn(treeview15, "time (μs)", "cellrenderertext48", 1);

    cout << " build VR Setup" << endl;
    /* ---------- VR Setup ---------------------- */
    auto setupTree = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
    auto treeview2_and_frame = addTreeview("treeview2", "setupTree", GTK_TREE_MODEL(setupTree));
    auto treeview2 = treeview2_and_frame.first;
    auto toolbar4 = addToolbar("toolbar4", GTK_ICON_SIZE_LARGE_TOOLBAR, GTK_ORIENTATION_HORIZONTAL);
    auto combobox6 = addCombobox("combobox6", "setups");
    auto scrolledwindow6 = addScrolledWindow("scrolledwindow6");
    gtk_grid_attach(GTK_GRID(table6), toolbar4, 0,0,2,1);
    gtk_grid_attach(GTK_GRID(table6), combobox6, 0,1,2,1);
    gtk_grid_attach(GTK_GRID(table6), treeview2_and_frame.second, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table6), scrolledwindow6, 1,2,1,1);

    auto toolbutton10 = addToolButton("toolbutton10", "gtk-new", toolbar4, "New Setup");
    auto toolbutton11 = addToolButton("toolbutton11", "gtk-delete", toolbar4, "Remove Component");
    auto toolbutton12 = addToolButton("toolbutton12", "gtk-save", toolbar4, "Save Setup");
    auto toolbutton19 = addToggleToolButton("toolbutton19", "gtk-orientation-portrait", toolbar4, "Mono Mode");

    GtkTreeViewColumn* treeviewcolumn2 = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(treeviewcolumn2, "Setup");
    auto cellrenderertext3 = addCellrenderer("cellrenderertext3", treeviewcolumn2, true);
    auto cellrenderertext5 = addCellrenderer("cellrenderertext5", treeviewcolumn2);
    gtk_tree_view_column_add_attribute(treeviewcolumn2, cellrenderertext3, "text", 0);
    gtk_tree_view_column_add_attribute(treeviewcolumn2, cellrenderertext5, "text", 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview2), treeviewcolumn2);

    auto viewport3 = addViewport("viewport3");
    auto vbox1 = addBox("vbox1", GTK_ORIENTATION_VERTICAL);
    auto fixed4 = addFixed("fixed4");
    gtk_container_add(GTK_CONTAINER(scrolledwindow6), viewport3);
    gtk_container_add(GTK_CONTAINER(viewport3), vbox1);
    auto table41 = appendExpander("expander28", "Displays Section", "table41", vbox1);
    auto options = appendExpander("expander3", "Window", "options", vbox1);
    auto table35 = appendExpander("expander22", "Window - Gtk", "table35", vbox1);
    auto fixed49 = appendExpander("expander23", "Window - Glut", "fixed49", vbox1);
    auto table9 = appendExpander("expander24", "Window - Remote", "table9", vbox1);
    auto vbox2 = appendExpander("expander8", "View", "vbox2", vbox1);
    auto table27 = appendExpander("expander7", "VRPN", "table27", vbox1);
    auto table3 = appendExpander("expander4", "VRPN Tracker", "table3", vbox1);
    auto table25 = appendExpander("expander6", "ART", "table25", vbox1);
    auto table26 = appendExpander("expander5", "ART Device", "table26", vbox1);
    auto table34 = appendExpander("expander21", "Device", "table34", vbox1);
    auto table43 = appendExpander("expander30", "Multitouch Device", "table43", vbox1);
    auto table44 = appendExpander("expander31", "Leap Device", "table44", vbox1);
    auto table33 = appendExpander("expander20", "Haptic Device", "table33", vbox1);
    auto table36 = appendExpander("expander25", "Network Node", "table36", vbox1);
    auto table37 = appendExpander("expander26", "Network Slave", "table37", vbox1);
    auto table42 = appendExpander("expander29", "Script", "table42", vbox1);
    gtk_box_pack_start(GTK_BOX(vbox1), fixed4, true, true, 0);

    /* ---------- VR Setup - display section ---------------------- */
    auto label153 = addLabel("label153", "Offset:");
    auto entry29 = addEntry("entry29");
    auto entry30 = addEntry("entry30");
    auto entry31 = addEntry("entry31");
    auto fixed65 = addFixed("fixed65");
    gtk_grid_attach(GTK_GRID(table41), label153, 0,0,3,1);
    gtk_grid_attach(GTK_GRID(table41), entry29, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table41), entry30, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table41), entry31, 2,1,1,1);
    gtk_grid_attach(GTK_GRID(table41), fixed65, 3,0,1,2);

    /* ---------- VR Setup - window section ---------------------- */
    auto checkbutton7 = addCheckbutton("checkbutton7", "Active");
    auto fixed3 = addFixed("fixed3");
    auto checkbutton1 = addCheckbutton("checkbutton1", "Fullscreen");
    auto label7 = addLabel("label7", "Position:");
    auto entry1 = addEntry("entry1");
    auto entry2 = addEntry("entry2");
    auto entry3 = addEntry("entry3");
    auto label8 = addLabel("label8", "[px]");
    auto label9 = addLabel("label9", "[px]");
    auto entry4 = addEntry("entry4");
    auto entry5 = addEntry("entry5");
    auto label10 = addLabel("label10", "Resolution:");
    auto fixed6 = addFixed("fixed6");
    auto label23 = addLabel("label23", "X Display:");
    auto combobox3 = addCombobox("combobox3", "xDisplays");
    auto fixed7 = addFixed("fixed7");
    auto label90 = addLabel("label90", "Mouse:");
    auto combobox13 = addCombobox("combobox13", "mouse_list");
    auto label174 = addLabel("label174", "MSAA:");
    auto combobox15 = addCombobox("combobox15", "msaa_list");
    auto msaa_info = addLabel("msaa_info", "");

    gtk_grid_attach(GTK_GRID(options), checkbutton7, 0,0,6,1);
    gtk_grid_attach(GTK_GRID(options), fixed3, 0,1,1,4);

    gtk_grid_attach(GTK_GRID(options), label23, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(options), combobox3, 2,1,2,1);
    gtk_grid_attach(GTK_GRID(options), fixed7, 4,1,2,1);

    gtk_grid_attach(GTK_GRID(options), checkbutton1, 1,2,5,1);

    gtk_grid_attach(GTK_GRID(options), label7, 1,3,1,1);
    gtk_grid_attach(GTK_GRID(options), entry1, 2,3,1,1);
    gtk_grid_attach(GTK_GRID(options), entry2, 3,3,1,1);
    gtk_grid_attach(GTK_GRID(options), entry3, 4,3,1,1);
    gtk_grid_attach(GTK_GRID(options), label8, 5,3,1,1);

    gtk_grid_attach(GTK_GRID(options), label10, 1,4,1,1);
    gtk_grid_attach(GTK_GRID(options), entry4, 2,4,1,1);
    gtk_grid_attach(GTK_GRID(options), entry5, 3,4,1,1);
    gtk_grid_attach(GTK_GRID(options), fixed6, 4,4,1,1);
    gtk_grid_attach(GTK_GRID(options), label9, 5,4,1,1);

    gtk_grid_attach(GTK_GRID(options), label90, 0,5,1,1);
    gtk_grid_attach(GTK_GRID(options), combobox13, 1,5,5,1);

    gtk_grid_attach(GTK_GRID(options), label174, 0,6,1,1);
    gtk_grid_attach(GTK_GRID(options), combobox15, 1,6,4,1);
    gtk_grid_attach(GTK_GRID(options), msaa_info, 2,6,1,1);

    /* ---------- VR Setup - window gtk section ---------------------- */
    /* ---------- VR Setup - window glut section ---------------------- */

    /* ---------- VR Setup - window remote section ---------------------- */
    auto label38 = addLabel("label38", "Nx: ");
    auto entry33 = addEntry("entry33");
    auto label39 = addLabel("label39", "Ny:");
    auto entry34 = addEntry("entry34");
    auto serverlist = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);
    auto treeview1_and_frame = addTreeview("treeview1", "serverlist", GTK_TREE_MODEL(serverlist));
    auto treeview1 = treeview1_and_frame.first;
    auto fixed8 = addFixed("fixed8");
    auto label18 = addLabel("label18", "State:");
    auto win_state = addLabel("win_state", "");
    auto fixed11 = addFixed("fixed11");
    auto button25 = addButton("button25", "connect");
    auto label135 = addLabel("label135", "Connection type:");
    auto radiobutton6 = addRadiobutton("radiobutton6", "Multicast", 0);
    auto radiobutton7 = addRadiobutton("radiobutton7", "SockPipeline", radiobutton6);
    auto radiobutton9 = addRadiobutton("radiobutton9", "StreamSock", radiobutton6);

    GtkTreeViewColumn* treeviewcolumn3 = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(treeviewcolumn3, "Server List");
    auto cellrenderertext6 = addCellrenderer("cellrenderertext6", treeviewcolumn3);
    auto cellrenderertext19 = addCellrenderer("cellrenderertext19", treeviewcolumn3);
    auto cellrenderertext21 = addCellrenderer("cellrenderertext21", treeviewcolumn3, true);
    gtk_tree_view_column_add_attribute(treeviewcolumn3, cellrenderertext6, "text", 0);
    gtk_tree_view_column_add_attribute(treeviewcolumn3, cellrenderertext19, "text", 1);
    gtk_tree_view_column_add_attribute(treeviewcolumn3, cellrenderertext21, "text", 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview1), treeviewcolumn3);


    gtk_grid_attach(GTK_GRID(table9), fixed8, 0,0,1,4);
    gtk_grid_attach(GTK_GRID(table9), label18, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table9), win_state, 2,0,2,1);
    gtk_grid_attach(GTK_GRID(table9), button25, 4,0,1,1);
    gtk_grid_attach(GTK_GRID(table9), label135, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table9), radiobutton6, 2,1,1,1);
    gtk_grid_attach(GTK_GRID(table9), radiobutton7, 3,1,1,1);
    gtk_grid_attach(GTK_GRID(table9), radiobutton9, 4,1,1,1);
    gtk_grid_attach(GTK_GRID(table9), label38, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table9), entry33, 2,2,1,1);
    gtk_grid_attach(GTK_GRID(table9), label39, 3,2,1,1);
    gtk_grid_attach(GTK_GRID(table9), entry34, 4,2,1,1);
    gtk_grid_attach(GTK_GRID(table9), treeview1_and_frame.second, 1,3,4,1);
    gtk_grid_attach(GTK_GRID(table9), fixed11, 5,0,1,4);

    /* ---------- VR Setup - view ---------------------- */
    auto label28 = addLabel("label28", "Area");
    auto label82 = addLabel("label82", "x:");
    auto label83 = addLabel("label83", "y:");
    auto entry52 = addEntry("entry52");
    auto entry53 = addEntry("entry53");
    auto entry56 = addEntry("entry56");
    auto entry57 = addEntry("entry57");
    auto checkbutton4 = addCheckbutton("checkbutton4", "statistics");
    auto checkbutton8 = addCheckbutton("checkbutton8", "Stereo");
    auto table7 = addGrid("table7");
    auto checkbutton11 = addCheckbutton("checkbutton11", "Projection");
    auto table8 = addGrid("table8");

    // table 7
    auto label25 = addLabel("label25", "Eye separation:");
    auto entry12 = addEntry("entry12");
    auto label26 = addLabel("label26", "[m]");
    auto checkbutton9 = addCheckbutton("checkbutton9", "Invert");
    auto checkbutton10 = addCheckbutton("checkbutton10", "Active stereo");

    // table 8
    auto checkbutton26 = addCheckbutton("checkbutton26", "User:");
    auto checkbutton30 = addCheckbutton("checkbutton30", "Mirror:");
    auto user_list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    VRGuiBuilder::get()->reg_object(G_OBJECT(user_list), "user_list");
    auto combobox18 = addCombobox("combobox18", "user_list");
    auto frame13 = addVectorFrame("frame13", "center_entry");
    auto frame14 = addVectorFrame("frame14", "user_entry");
    auto frame15 = addVectorFrame("frame15", "normal_entry");
    auto frame16 = addVectorFrame("frame16", "viewup_entry");
    auto frame17 = addVectorFrame("frame17", "size_entry");
    auto frame26 = addVectorFrame("frame26", "shear_entry");
    auto frame27 = addVectorFrame("frame27", "warp_entry");
    auto frame28 = addVectorFrame("frame28", "vsize_entry");
    auto frame29 = addVectorFrame("frame29", "mirror_pos_entry");
    auto frame30 = addVectorFrame("frame30", "mirror_norm_entry");

    gtk_grid_attach(GTK_GRID(vbox2), label28, 0,0,3,1);
    gtk_grid_attach(GTK_GRID(vbox2), label82, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(vbox2), entry52, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(vbox2), entry53, 2,1,1,1);
    gtk_grid_attach(GTK_GRID(vbox2), label83, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(vbox2), entry56, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(vbox2), entry57, 2,2,1,1);
    gtk_grid_attach(GTK_GRID(vbox2), frame28, 0,3,3,1);
    gtk_grid_attach(GTK_GRID(vbox2), checkbutton4, 0,4,3,1);
    gtk_grid_attach(GTK_GRID(vbox2), checkbutton8, 0,5,3,1);
    gtk_grid_attach(GTK_GRID(vbox2), table7, 0,6,3,1);
    gtk_grid_attach(GTK_GRID(vbox2), checkbutton11, 0,7,3,1);
    gtk_grid_attach(GTK_GRID(vbox2), table8, 0,8,3,1);

    gtk_grid_attach(GTK_GRID(table7), label25, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table7), entry12, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table7), label26, 2,0,1,1);
    gtk_grid_attach(GTK_GRID(table7), checkbutton9, 0,1,3,1);
    gtk_grid_attach(GTK_GRID(table7), checkbutton10, 0,2,3,1);

    gtk_grid_attach(GTK_GRID(table8), checkbutton26, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table8), combobox18, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table8), frame13, 0,1,2,1);
    gtk_grid_attach(GTK_GRID(table8), frame14, 0,2,2,1);
    gtk_grid_attach(GTK_GRID(table8), frame15, 0,3,2,1);
    gtk_grid_attach(GTK_GRID(table8), frame16, 0,4,2,1);
    gtk_grid_attach(GTK_GRID(table8), frame17, 0,5,2,1);
    gtk_grid_attach(GTK_GRID(table8), frame26, 0,6,2,1);
    gtk_grid_attach(GTK_GRID(table8), frame27, 0,7,2,1);
    gtk_grid_attach(GTK_GRID(table8), checkbutton30, 0,8,2,1);
    gtk_grid_attach(GTK_GRID(table8), frame29, 0,9,2,1);
    gtk_grid_attach(GTK_GRID(table8), frame30, 0,10,2,1);

    /* ---------- VR Setup - vrpn ---------------------- */
    auto checkbutton25 = addCheckbutton("checkbutton25", "active");
    auto label120 = addLabel("label120", "Port:");
    auto entry13 = addEntry("entry13");
    auto checkbutton39 = addCheckbutton("checkbutton39", "test server");
    auto checkbutton40 = addCheckbutton("checkbutton40", "verbose");
    gtk_grid_attach(GTK_GRID(table27), checkbutton25, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table27), label120, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table27), entry13, 2,0,1,1);
    gtk_grid_attach(GTK_GRID(table27), checkbutton39, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table27), checkbutton40, 1,1,2,1);

    /* ---------- VR Setup - vrpn tracker ---------------------- */
    auto label43 = addLabel("label43", "address:");
    auto entry50 = addEntry("entry50");
    auto label116 = addLabel("label116", "translation axis:");
    auto frame23 = addVectorFrame("frame23", "tvrpn_entry");
    auto label117 = addLabel("label117", "rotation axis:");
    auto frame24 = addVectorFrame("frame24", "rvrpn_entry");
    gtk_grid_attach(GTK_GRID(table3), label43, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table3), entry50, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table3), label116, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table3), frame23, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table3), label117, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table3), frame24, 1,2,1,1);

    /* ---------- VR Setup - art ---------------------- */
    auto checkbutton24 = addCheckbutton("checkbutton24", "active");
    auto label74 = addLabel("label74", "Port:");
    auto entry39 = addEntry("entry39");
    auto frame36 = addVectorFrame("frame36", "art_offset");
    auto frame35 = addVectorFrame("frame35", "art_axis");
    gtk_grid_attach(GTK_GRID(table25), checkbutton24, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table25), label74, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table25), entry39, 2,0,1,1);
    gtk_grid_attach(GTK_GRID(table25), frame36, 0,1,3,1);
    gtk_grid_attach(GTK_GRID(table25), frame35, 0,2,3,1);

    /* ---------- VR Setup - art device ---------------------- */
    auto label75 = addLabel("label75", "ID:");
    auto entry40 = addEntry("entry40");
    auto label76 = addLabel("label76", "type:");
    auto combobox17 = addCombobox("combobox17", "art_devices");
    gtk_grid_attach(GTK_GRID(table26), label75, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table26), entry40, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table26), label76, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table26), combobox17, 1,1,1,1);

    /* ---------- VR Setup - device ---------------------- */
    auto label41 = addLabel("label41", "name:");
    auto label93 = addLabel("label93", "dev_name");
    auto label50 = addLabel("label50", "type:");
    auto combobox26 = addCombobox("combobox26", "dev_types_list");
    auto label109 = addLabel("label109", "Intersection:");
    auto label110 = addLabel("label110", "int_obj");
    auto label113 = addLabel("label113", "int. point:");
    auto label111 = addLabel("label111", "int_pnt");
    auto label114 = addLabel("label114", "int. texel:");
    auto label112 = addLabel("label112", "int_uv");
    auto checkbutton37 = addCheckbutton("checkbutton37", "show intersection point");
    gtk_grid_attach(GTK_GRID(table34), label41, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table34), label93, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table34), label50, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table34), combobox26, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table34), label109, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table34), label110, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table34), label113, 0,3,1,1);
    gtk_grid_attach(GTK_GRID(table34), label111, 1,3,1,1);
    gtk_grid_attach(GTK_GRID(table34), label114, 0,4,1,1);
    gtk_grid_attach(GTK_GRID(table34), label112, 1,4,1,1);
    gtk_grid_attach(GTK_GRID(table34), checkbutton37, 0,5,2,1);

    /* ---------- VR Setup - multitouch ---------------------- */
    auto label61 = addLabel("label61", "name:");
    auto combobox12 = addCombobox("combobox12", "liststore11");
    gtk_grid_attach(GTK_GRID(table43), label61, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table43), combobox12, 1,0,1,1);

    /* ---------- VR Setup - leap ---------------------- */
    auto label155 = addLabel("label155", "host:");
    auto entry28 = addEntry("entry28");
    auto label156 = addLabel("label156", "status:");
    auto label157 = addLabel("label157", "not connected");
    auto label158 = addLabel("label158", "serial:");
    auto label159 = addLabel("label159", "label");
    auto button34 = addButton("button34", "start calibration");
    auto button35 = addButton("button35", "stop calibration");
    auto label160 = addLabel("label160", "transformation:");
    auto frame31 = addVectorFrame("frame31", "leap_pos_entry");
    auto frame32 = addVectorFrame("frame32", "leap_dir_entry");
    auto frame34 = addVectorFrame("frame34", "leap_up_entry");

    gtk_grid_attach(GTK_GRID(table44), label155, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table44), entry28, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table44), label156, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table44), label157, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table44), label158, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table44), label159, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table44), button34, 0,3,1,1);
    gtk_grid_attach(GTK_GRID(table44), button35, 1,3,1,1);
    gtk_grid_attach(GTK_GRID(table44), label160, 0,4,2,1);
    gtk_grid_attach(GTK_GRID(table44), frame31, 0,5,2,1);
    gtk_grid_attach(GTK_GRID(table44), frame32, 0,6,2,1);
    gtk_grid_attach(GTK_GRID(table44), frame34, 0,7,2,1);

    /* ---------- VR Setup - haptic ---------------------- */
    auto label34 = addLabel("label34", "IP:");
    auto entry8 = addEntry("entry8");
    auto label35 = addLabel("label35", "type:");
    auto combobox25 = addCombobox("combobox25", "liststore8");
    auto label57 = addLabel("label57", "status:");
    auto label63= addLabel("label63", "deamon");
    auto label64 = addLabel("label64", "no");
    auto label65 = addLabel("label65", "device");
    auto label66 = addLabel("label66", "no");

    gtk_grid_attach(GTK_GRID(table33), label34, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table33), entry8, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table33), label35, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table33), combobox25, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table33), label57, 0,2,2,1);
    gtk_grid_attach(GTK_GRID(table33), label63, 0,3,1,1);
    gtk_grid_attach(GTK_GRID(table33), label64, 1,3,1,1);
    gtk_grid_attach(GTK_GRID(table33), label65, 0,4,1,1);
    gtk_grid_attach(GTK_GRID(table33), label66, 1,4,1,1);

    /* ---------- VR Setup - network node ---------------------- */
    auto label125 = addLabel("label125", "address:");
    auto entry15 = addEntry("entry15");
    auto label130 = addLabel("label130", "status");
    auto label128 = addLabel("label128", "ssh user:");
    auto entry20 = addEntry("entry20");
    auto label129 = addLabel("label129", "status");
    auto button6 = addButton("button6", "distribute key");
    auto label126 = addLabel("label126", "status");
    auto button30 = addButton("button30", "stop slaves");
    auto label108 = addLabel("label108", "root path:");
    auto entry32 = addEntry("entry32");
    auto label161 = addLabel("label161", "status");

    gtk_grid_attach(GTK_GRID(table36), label125, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table36), entry15, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table36), label130, 2,0,1,1);
    gtk_grid_attach(GTK_GRID(table36), label128, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table36), entry20, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table36), label129, 2,1,1,1);
    gtk_grid_attach(GTK_GRID(table36), button6, 0,2,2,1);
    gtk_grid_attach(GTK_GRID(table36), label126, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table36), button30, 0,3,3,1);
    gtk_grid_attach(GTK_GRID(table36), label108, 0,4,1,1);
    gtk_grid_attach(GTK_GRID(table36), entry32, 1,4,1,1);
    gtk_grid_attach(GTK_GRID(table36), label161, 2,4,1,1);

    /* ---------- VR Setup - network slave ---------------------- */
    auto label131 = addLabel("label131", "connection identifier:");
    auto label138 = addLabel("label138", "NAME");
    auto label132 = addLabel("label132", "status");
    auto checkbutton42 = addCheckbutton("checkbutton42", "autostart");
    auto button1 = addButton("button1", "start");
    auto label136 = addLabel("label136", "status");

    auto checkbutton41 = addCheckbutton("checkbutton41", "active stereo");
    auto checkbutton29 = addCheckbutton("checkbutton29", "fullscreen");
    auto label140 = addLabel("label140", "port: ");
    auto entry22 = addEntry("entry22");

    auto label139 = addLabel("label139", "Connection type:");
    auto radiobutton10 = addRadiobutton("radiobutton10", "", 0);
    auto radiobutton11 = addRadiobutton("radiobutton11", "", radiobutton10);
    auto radiobutton12 = addRadiobutton("radiobutton12", "", radiobutton10);

    auto label133 = addLabel("label133", "local display:");
    auto entry19 = addEntry("entry19");
    auto label173 = addLabel("label173", "startup delay: ");
    auto entry37 = addEntry("entry37");

    gtk_grid_attach(GTK_GRID(table37), label131, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table37), label138, 1,0,2,1);
    gtk_grid_attach(GTK_GRID(table37), label132, 3,0,1,1);

    gtk_grid_attach(GTK_GRID(table37), checkbutton42, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table37), button1, 1,1,2,1);
    gtk_grid_attach(GTK_GRID(table37), label136, 3,1,1,1);

    gtk_grid_attach(GTK_GRID(table37), checkbutton41, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table37), checkbutton29, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table37), label140, 2,2,1,1);
    gtk_grid_attach(GTK_GRID(table37), entry22, 3,2,1,1);

    gtk_grid_attach(GTK_GRID(table37), label139, 0,3,1,1);
    gtk_grid_attach(GTK_GRID(table37), radiobutton10, 1,3,1,1);
    gtk_grid_attach(GTK_GRID(table37), radiobutton11, 2,3,1,1);
    gtk_grid_attach(GTK_GRID(table37), radiobutton12, 3,3,1,1);

    gtk_grid_attach(GTK_GRID(table37), label133, 0,4,1,1);
    gtk_grid_attach(GTK_GRID(table37), entry19, 1,4,1,1);
    gtk_grid_attach(GTK_GRID(table37), label173, 2,4,1,1);
    gtk_grid_attach(GTK_GRID(table37), entry37, 3,4,1,1);

    /* ---------- VR Setup - scripts ---------------------- */
    // TODO or deprecated?


    cout << " build VR Scene" << endl;
    /* ---------- VR Scene ---------------------- */
    auto table14 = addGrid("table14");
    auto scenegraph_tab = addGrid("scenegraph_tab");
    auto table30 = addGrid("table30");
    auto table4 = addGrid("table4");
    auto table21 = addGrid("table21");
    addNotebookPage(notebook3, table30, "General");
    addNotebookPage(notebook3, scenegraph_tab, "Scenegraph");
    addNotebookPage(notebook3, table4, "Navigation");
    addNotebookPage(notebook3, table14, "Scripting");
    addNotebookPage(notebook3, table21, "Semantics");

    /* ---------- VR Scene -scripting ---------------------- */
    auto toolbar3 = addToolbar("toolbar3", GTK_ICON_SIZE_LARGE_TOOLBAR, GTK_ORIENTATION_HORIZONTAL);
    auto script_tree = gtk_tree_store_new(9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    auto treeview5_and_frame = addTreeview("treeview5", "script_tree", GTK_TREE_MODEL(script_tree));
    auto treeview5 = treeview5_and_frame.first;
    auto table15 = addGrid("table15");
    gtk_grid_attach(GTK_GRID(table14), toolbar3, 0,0,2,1);
    gtk_grid_attach(GTK_GRID(table14), treeview5_and_frame.second, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table14), table15, 1,1,1,1);

    auto toolbutton6 = addToolButton("toolbutton6", "gtk-new", toolbar3, "New Script");
    auto toolbutton29 = addToolButton("toolbutton29", "gtk-paste", toolbar3, "Script Templates");
    auto toolbutton20 = addToolButton("toolbutton20", "gtk-indent", toolbar3, "New Group");
    auto toolbutton22 = addToolButton("toolbutton22", "gtk-open", toolbar3, "Import Script From Scene");
    auto toolbutton9 = addToolButton("toolbutton9", "gtk-delete", toolbar3, "Delete Script");
    auto toolbutton7 = addToolButton("toolbutton7", "gtk-save", toolbar3, "Save Script");
    auto toolbutton8 = addToolButton("toolbutton8", "gtk-execute", toolbar3, "Execute Script");
    auto toolbutton23 = addToolButton("toolbutton23", "gtk-find", toolbar3, "Search");
    auto toolbutton16 = addToolButton("toolbutton16", "gtk-help", toolbar3, "Documentation");
    auto toggletoolbutton1 = addToggleToolButton("toggletoolbutton1", "gtk-sort-ascending", toolbar3, "Show Performance");
    auto toggletoolbutton2 = addToggleToolButton("toggletoolbutton2", "gtk-media-pause", toolbar3, "Pause Script Execution");

    GtkTreeViewColumn* treeviewcolumn14 = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(treeviewcolumn14, "Script");
    auto cellrenderertext13 = addCellrenderer("cellrenderertext13", treeviewcolumn14, true);
    auto cellrenderertext45 = addCellrenderer("cellrenderertext45", treeviewcolumn14);
    auto cellrendererpixbuf2 = addImgCellrenderer("cellrendererpixbuf2", treeviewcolumn14);
    auto cellrenderertext47 = addCellrenderer("cellrenderertext47", treeviewcolumn14);
    gtk_tree_view_column_add_attribute(treeviewcolumn14, cellrenderertext13, "text", 0);
    gtk_tree_view_column_add_attribute(treeviewcolumn14, cellrenderertext13, "background", 2);
    gtk_tree_view_column_add_attribute(treeviewcolumn14, cellrenderertext13, "foreground", 1);
    gtk_tree_view_column_add_attribute(treeviewcolumn14, cellrenderertext45, "text", 3);
    gtk_tree_view_column_add_attribute(treeviewcolumn14, cellrenderertext45, "background", 5);
    gtk_tree_view_column_add_attribute(treeviewcolumn14, cellrenderertext45, "foreground", 4);
    gtk_tree_view_column_add_attribute(treeviewcolumn14, cellrendererpixbuf2, "stock-id", 6);
    gtk_tree_view_column_add_attribute(treeviewcolumn14, cellrenderertext47, "text", 7);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview5), treeviewcolumn14);

    auto table32 = addGrid("table32");
    auto label32 = addLabel("label32", "Type:");
    auto combobox1 = addCombobox("combobox1", "liststore6");
    auto label148 = addLabel("label148", "Group:");
    auto combobox10 = addCombobox("combobox10", "liststore10");
    auto label33 = addLabel("label33", "Server:");
    auto combobox24 = addCombobox("combobox24", "liststore7");
    gtk_grid_attach(GTK_GRID(table32), label32, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table32), combobox1, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table32), label148, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table32), combobox10, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table32), label33, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table32), combobox24, 1,2,1,1);

    auto hbox3 = addGrid("hbox3");
    auto triggers = gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);
    auto treeview14_and_frame = addTreeview("treeview14", "triggers", GTK_TREE_MODEL(triggers), false, false);
    auto treeview14 = treeview14_and_frame.first;
    auto button23 = addImgButton("button23", "gtk-add");
    auto button24 = addImgButton("button24", "gtk-remove");
    gtk_grid_attach(GTK_GRID(hbox3), treeview14_and_frame.second, 0,0,1,2);
    gtk_grid_attach(GTK_GRID(hbox3), button23, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(hbox3), button24, 1,1,1,1);
    gtk_widget_set_hexpand(treeview14, true);
    gtk_widget_set_vexpand(treeview14_and_frame.second, false);

    auto treeviewcolumn27 = addTreecolumn("treeviewcolumn27", "Trigger");
    auto treeviewcolumn28 = addTreecolumn("treeviewcolumn28", "Device");
    auto treeviewcolumn32 = addTreecolumn("treeviewcolumn32", " ");
    auto treeviewcolumn30 = addTreecolumn("treeviewcolumn30", "State");
    auto treeviewcolumn32cr = addImgCellrenderer("treeviewcolumn32cr", treeviewcolumn32);
    setStringProperty((GtkWidget*)treeviewcolumn32cr, "stock_id", "gtk-index");
    auto ScriptTrigger = gtk_list_store_new(1, G_TYPE_STRING);
    auto ScriptTriggerDevices = gtk_list_store_new(1, G_TYPE_STRING);
    auto ScriptTriggerStates = gtk_list_store_new(1, G_TYPE_STRING);
    VRGuiBuilder::reg_object(G_OBJECT(ScriptTrigger), "ScriptTrigger");
    VRGuiBuilder::reg_object(G_OBJECT(ScriptTriggerDevices), "ScriptTriggerDevices");
    VRGuiBuilder::reg_object(G_OBJECT(ScriptTriggerStates), "ScriptTriggerStates");

    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview14), treeviewcolumn27);
    addTreeviewTextcolumn(treeview14, "Parameter", "cellrenderertext16", 4, true);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview14), treeviewcolumn28);
    addTreeviewTextcolumn(treeview14, "Key", "cellrenderertext41", 2, true);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview14), treeviewcolumn32);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview14), treeviewcolumn30);

    auto hbox13 = addGrid("hbox13");
    auto liststore2 = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);
    auto treeview7_and_frame = addTreeview("treeview7", "liststore2", GTK_TREE_MODEL(liststore2), false, false);
    auto treeview7 = treeview7_and_frame.first;
    auto button12 = addImgButton("button12", "gtk-add");
    auto button13 = addImgButton("button13", "gtk-remove");
    gtk_grid_attach(GTK_GRID(hbox13), treeview7_and_frame.second, 0,0,1,2);
    gtk_grid_attach(GTK_GRID(hbox13), button12, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(hbox13), button13, 1,1,1,1);
    gtk_widget_set_hexpand(treeview7, true);
    gtk_widget_set_vexpand(treeview7_and_frame.second, false);

    auto expander19 = addExpander("expander19", "Options", table32);
    auto expander17 = addExpander("expander17", "Triggers", hbox3);
    auto expander18 = addExpander("expander18", "Arguments", hbox13);
    auto scrolledwindow4 = addScrolledWindow("scrolledwindow4");
    gtk_grid_attach(GTK_GRID(table15), expander19, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(table15), expander17, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(table15), expander18, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(table15), scrolledwindow4, 0, 3, 1, 1);

    addTreeviewTextcolumn(treeview7, "Name", "cellrenderertext2", 0, true);
    auto treeviewcolumn16 = addTreecolumn("treeviewcolumn16", "Type");
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview7), treeviewcolumn16);
    addTreeviewTextcolumn(treeview7, "Value", "cellrenderertext14", 1, true);
    auto arg_types = gtk_list_store_new(1, G_TYPE_STRING);
    VRGuiBuilder::reg_object(G_OBJECT(arg_types), "arg_types");

    /* ---------- Script templates ---------------------- */
    auto templates_docs = addDialog("scriptTemplates");
    auto tdialog_vbox = gtk_dialog_get_content_area(GTK_DIALOG(templates_docs));
    auto tdialog_area = gtk_dialog_get_action_area(GTK_DIALOG(templates_docs));
    auto tbutton1 = addButton("tbutton1", "Close");
    auto tbutton2 = addButton("tbutton2", "Import");
    auto tlabel1 = addLabel("tlabel1", "PolyVR Script Templates");
    auto thpaned1 = addPaned("thpaned1", GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(tdialog_area), tbutton1, false, true, 0);
    gtk_box_pack_start(GTK_BOX(tdialog_area), tbutton2, false, true, 0);
    gtk_box_pack_start(GTK_BOX(tdialog_vbox), tlabel1, false, true, 0);
    gtk_box_pack_start(GTK_BOX(tdialog_vbox), thpaned1, false, true, 0);
    gtk_window_set_transient_for(GTK_WINDOW(templates_docs), GTK_WINDOW(window1));
    gtk_widget_set_size_request(templates_docs, 800, 600);
    gtk_paned_set_position(GTK_PANED(thpaned1), 200);

    auto ttable1 = addGrid("ttable1");
    auto timage1 = addStockImage("timage1", "gtk-find", GTK_ICON_SIZE_SMALL_TOOLBAR);
    auto tentry1 = addEntry("tentry1");
    auto templates = gtk_tree_store_new(1, G_TYPE_STRING);
    auto ttreeview1_and_frame = addTreeview("ttreeview1", "templates", GTK_TREE_MODEL(templates));
    auto ttreeview1 = ttreeview1_and_frame.first;
    auto tscrolledwindow1 = addScrolledWindow("tscrolledwindow1");
    auto ttextview1 = addTextview("ttextview1", "scripttemplates");
    gtk_widget_set_hexpand(ttreeview1, true);
    add1ToPaned(thpaned1, ttable1);
    gtk_grid_attach(GTK_GRID(ttable1), timage1, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(ttable1), tentry1, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(ttable1), ttreeview1_and_frame.second, 0,1,2,1);
    add2ToPaned(thpaned1, tscrolledwindow1);
    gtk_container_add(GTK_CONTAINER(tscrolledwindow1), ttextview1);

    addTreeviewTextcolumn(ttreeview1, "Templates", "tcellrenderertext1", 0);

    /* ---------- Py Docs ---------------------- */
    auto pybindings_docs = addDialog("pybindings-docs");
    auto dialog_vbox6 = gtk_dialog_get_content_area(GTK_DIALOG(pybindings_docs));
    auto dialog_action_area6 = gtk_dialog_get_action_area(GTK_DIALOG(pybindings_docs));
    auto button16 = addButton("button16", "Close");
    auto label69 = addLabel("label69", "PolyVR Python Bindings");
    auto table40 = addGrid("table40");
    gtk_box_pack_start(GTK_BOX(dialog_action_area6), button16, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox6), label69, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox6), table40, false, true, 0);
    gtk_window_set_transient_for(GTK_WINDOW(pybindings_docs), GTK_WINDOW(window1));
    gtk_widget_set_size_request(pybindings_docs, 800, 600);

    auto image49 = addStockImage("image49", "gtk-find", GTK_ICON_SIZE_SMALL_TOOLBAR);
    auto entry25 = addEntry("entry25");
    auto bindings = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    auto treeview3_and_frame = addTreeview("treeview3", "bindings", GTK_TREE_MODEL(bindings), false, true);
    auto treeview3 = treeview3_and_frame.first;
    auto scrolledwindow7 = addScrolledWindow("scrolledwindow7");
    auto textview1 = addTextview("textview1", "pydoc");
    gtk_widget_set_hexpand(treeview3, true);
    gtk_widget_set_hexpand(treeview3_and_frame.second, false);
    gtk_widget_set_hexpand(scrolledwindow7, true);
    gtk_grid_attach(GTK_GRID(table40), image49, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table40), entry25, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table40), treeview3_and_frame.second, 0,1,2,1);
    gtk_grid_attach(GTK_GRID(table40), scrolledwindow7, 2,0,1,2);
    gtk_container_add(GTK_CONTAINER(scrolledwindow7), textview1);

    addTreeviewTextcolumn(treeview3, "VR Module", "cellrenderertext1", 0);

    /* ---------- Py Find ---------------------- */
    auto find_dialog = addDialog("find_dialog");
    auto dialog_vbox12 = gtk_dialog_get_content_area(GTK_DIALOG(find_dialog));
    auto dialog_action_area12 = gtk_dialog_get_action_area(GTK_DIALOG(find_dialog));
    auto button28 = addButton("button28", "Cancel");
    auto button29 = addButton("button29", "Find");
    auto hbox14 = addGrid("hbox14");
    auto label115 = addLabel("label115", "Search");
    auto entry10 = addEntry("entry10");
    auto entry11 = addEntry("entry11");
    auto checkbutton38 = addCheckbutton("checkbutton38", "all scripts");
    auto checkbutton12 = addCheckbutton("checkbutton12", "replace by:");
    gtk_box_pack_start(GTK_BOX(dialog_action_area12), button28, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_action_area12), button29, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox12), hbox14, false, true, 0);
    gtk_grid_attach(GTK_GRID(hbox14), label115, 0,0,2,1);
    gtk_grid_attach(GTK_GRID(hbox14), checkbutton38, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(hbox14), entry10, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(hbox14), checkbutton12, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(hbox14), entry11, 1,2,1,1);
    gtk_window_set_transient_for(GTK_WINDOW(find_dialog), GTK_WINDOW(window1));

    /* ---------- VR Scene - navigation ---------------------- */
    auto label16 = addLabel("label16", "Preset:");
    auto combobox5 = addCombobox("combobox5", "nav_presets");
    auto button2 = addImgButton("button2", "gtk-add");
    auto button7 = addImgButton("button7", "gtk-remove");
    auto label19 = addLabel("label19", "Target:");
    auto combobox7 = addCombobox("combobox7", "trgt_list");
    auto label47 = addLabel("label47", "Device:");
    auto combobox11 = addCombobox("combobox11", "devs_list");
    auto label48 = addLabel("label48", "Bindings:");
    auto button5 = addImgButton("button5", "gtk-add");
    auto button8 = addImgButton("button8", "gtk-remove");
    auto nav_bindings = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_FLOAT);
    auto treeview4_and_frame = addTreeview("treeview4", "nav_bindings", GTK_TREE_MODEL(nav_bindings));
    auto treeview4 = treeview4_and_frame.first;
    gtk_grid_attach(GTK_GRID(table4), label16, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table4), combobox5, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table4), button2, 2,0,1,1);
    gtk_grid_attach(GTK_GRID(table4), button7, 3,0,1,1);
    gtk_grid_attach(GTK_GRID(table4), label19, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table4), combobox7, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table4), label47, 2,1,1,1);
    gtk_grid_attach(GTK_GRID(table4), combobox11, 3,1,1,1);
    gtk_grid_attach(GTK_GRID(table4), label48, 0,2,2,1);
    gtk_grid_attach(GTK_GRID(table4), button5, 2,2,1,1);
    gtk_grid_attach(GTK_GRID(table4), button8, 3,2,1,1);
    gtk_grid_attach(GTK_GRID(table4), treeview4_and_frame.second, 0,3,4,1);

    addTreeviewTextcolumn(treeview4, "Name", "cellrenderertext11", 0, true);
    addTreeviewTextcolumn(treeview4, "State", "cellrenderertext12", 1, true);
    addTreeviewTextcolumn(treeview4, "Speed", "cellrenderertext46", 5);
    auto treeviewcolumn12 = addTreecolumn("treeviewcolumn12", "Type");
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview4), treeviewcolumn12);
    auto treeviewcolumn13 = addTreecolumn("treeviewcolumn13", "Callback");
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview4), treeviewcolumn13);

    auto binding_callbacks = gtk_list_store_new(1, G_TYPE_STRING);
    VRGuiBuilder::reg_object(G_OBJECT(binding_callbacks), "binding_callbacks");
    auto binding_types = gtk_list_store_new(1, G_TYPE_STRING);
    VRGuiBuilder::reg_object(G_OBJECT(binding_types), "binding_types");

    /* ---------- VR Scene - general ---------------------- */
    auto label87 = addLabel("label87", "Background:");
    auto bg_solid = addColorChooser("bg_solid");
    auto radiobutton3 = addRadiobutton("radiobutton3", "Solid", 0);
    auto radiobutton4 = addRadiobutton("radiobutton4", "Image", radiobutton3);
    auto radiobutton5 = addRadiobutton("radiobutton5", "Skybox", radiobutton3);
    auto radiobutton18 = addRadiobutton("radiobutton18", "Sky", radiobutton3);
    auto label88 = addLabel("label88", "Path:");
    auto entry42 = addEntry("entry42");
    auto entry14 = addEntry("entry14", 4);
    auto button18 = addImgButton("button18", "gtk-directory");
    auto spacer = addSpacer(20);

    gtk_grid_attach(GTK_GRID(table30), label87, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table30), bg_solid, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table30), radiobutton3, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table30), radiobutton4, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table30), radiobutton5, 2,1,1,1);
    gtk_grid_attach(GTK_GRID(table30), radiobutton18, 3,1,1,1);
    gtk_grid_attach(GTK_GRID(table30), label88, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table30), entry42, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table30), entry14, 2,2,1,1);
    gtk_grid_attach(GTK_GRID(table30), button18, 3,2,1,1);
    gtk_grid_attach(GTK_GRID(table30), spacer, 0,3,4,1);

    auto label_2 = addLabel("label_2", "Physics:");
    auto checkbutton_1 = addCheckbutton("checkbutton_1", "Gravity:");
    auto entry43 = addEntry("entry43", 4);
    auto entry47 = addEntry("entry47", 4);
    auto entry58 = addEntry("entry58", 4);
    auto spacer2 = addSpacer(20);
    auto label_01 = addLabel("label_01", "Rendering:");
    auto tfpsLabel = addLabel("tfpsLabel", "Target FPS:");
    auto tfpsCombobox = addCombobox("tfpsCombobox", "tfps");
    auto checkbutton_01 = addCheckbutton("checkbutton_01", "Frustum culling");
    auto checkbutton_02 = addCheckbutton("checkbutton_02", "Occlusion culling");
    auto checkbutton_2 = addCheckbutton("checkbutton_2", "Two sided");
    auto checkbutton_3 = addCheckbutton("checkbutton_3", "Deferred rendering");
    auto hbuttonbox7 = addGrid("hbuttonbox7");
    auto radiobutton13 = addRadiobutton("radiobutton13", "rendered", 0);
    auto radiobutton14 = addRadiobutton("radiobutton14", "positions", radiobutton13);
    auto radiobutton15 = addRadiobutton("radiobutton15", "normals", radiobutton13);
    auto radiobutton16 = addRadiobutton("radiobutton16", "diffuse", radiobutton13);
    auto radiobutton17 = addRadiobutton("radiobutton17", "ambient", radiobutton13);

    gtk_widget_set_sensitive(entry42, false);
    gtk_widget_set_sensitive(entry14, false);
    gtk_widget_set_sensitive(button18, false);

    gtk_grid_attach(GTK_GRID(table30), label_2, 0,4,1,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_1, 0,5,1,1);
    gtk_grid_attach(GTK_GRID(table30), entry43, 1,5,1,1);
    gtk_grid_attach(GTK_GRID(table30), entry47, 2,5,1,1);
    gtk_grid_attach(GTK_GRID(table30), entry58, 3,5,1,1);
    gtk_grid_attach(GTK_GRID(table30), spacer2, 0,6,4,1);
    gtk_grid_attach(GTK_GRID(table30), label_01, 0,7,1,1);
    gtk_grid_attach(GTK_GRID(table30), tfpsLabel, 0,8,2,1);
    gtk_grid_attach(GTK_GRID(table30), tfpsCombobox, 2,8,2,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_01, 0,9,2,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_02, 2,9,2,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_2, 0,10,2,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_3, 2,10,2,1);
    gtk_grid_attach(GTK_GRID(table30), hbuttonbox7, 0,11,4,1);
    gtk_grid_attach(GTK_GRID(hbuttonbox7), radiobutton13, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(hbuttonbox7), radiobutton14, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(hbuttonbox7), radiobutton15, 2,0,1,1);
    gtk_grid_attach(GTK_GRID(hbuttonbox7), radiobutton16, 3,0,1,1);
    gtk_grid_attach(GTK_GRID(hbuttonbox7), radiobutton17, 4,0,1,1);

    auto checkbutton_4 = addCheckbutton("checkbutton_4", "SSAO:");
    auto label121 = addLabel("label121", "radius:");
    auto hscale1 = addScale("hscale1", 0, 1, 0.01);
    auto label122 = addLabel("label122", "kernel:");
    auto hscale2 = addScale("hscale2", 0, 32, 1);
    auto label123 = addLabel("label123", "noise:");
    auto hscale3 = addScale("hscale3", 0, 32, 1);
    auto checkbutton_5 = addCheckbutton("checkbutton_5", "Calibration screen");
    auto checkbutton_7 = addCheckbutton("checkbutton_7", "Marker");
    auto checkbutton_6 = addCheckbutton("checkbutton_6", "HMD distortion");
    auto checkbutton_8 = addCheckbutton("checkbutton_8", "FXAA");
    auto spacer3 = addSpacer(20);
    auto label_1 = addLabel("label_1", "Export OSG:");
    auto button22 = addButton("button22", "dump");

    gtk_grid_attach(GTK_GRID(table30), checkbutton_4, 0,12,4,1);
    gtk_grid_attach(GTK_GRID(table30), label121, 0,13,2,1);
    gtk_grid_attach(GTK_GRID(table30), hscale1, 2,13,2,1);
    gtk_grid_attach(GTK_GRID(table30), label122, 0,14,2,1);
    gtk_grid_attach(GTK_GRID(table30), hscale2, 2,14,2,1);
    gtk_grid_attach(GTK_GRID(table30), label123, 0,15,2,1);
    gtk_grid_attach(GTK_GRID(table30), hscale3, 2,15,2,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_5, 0,16,2,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_7, 2,16,2,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_6, 0,17,2,1);
    gtk_grid_attach(GTK_GRID(table30), checkbutton_8, 2,17,2,1);
    gtk_grid_attach(GTK_GRID(table30), spacer3, 0,18,4,1);
    gtk_grid_attach(GTK_GRID(table30), label_1, 0,19,2,1);
    gtk_grid_attach(GTK_GRID(table30), button22, 2,19,2,1);

    /* ---------- VR Scene - scenegraph ---------------------- */
    auto hpaned3 = addPaned("hpaned3", GTK_ORIENTATION_HORIZONTAL);
    auto scenegraph = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    auto treeview6_and_frame = addTreeview("treeview6", "scenegraph", GTK_TREE_MODEL(scenegraph), true);
    auto treeview6 = treeview6_and_frame.first;
    auto table11 = addBox("table11", GTK_ORIENTATION_VERTICAL);
    auto button17 = addImgButton("button17", "gtk-refresh");
    auto current_object_lab = addLabel("current_object_lab", "obj");
    auto checkbutton16 = addCheckbutton("checkbutton16", "live");
    gtk_grid_attach(GTK_GRID(scenegraph_tab), button17, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(scenegraph_tab), current_object_lab, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(scenegraph_tab), checkbutton16, 2,0,1,1);
    gtk_grid_attach(GTK_GRID(scenegraph_tab), hpaned3, 0,1,3,1);
    add1ToPaned(hpaned3, treeview6_and_frame.second);
    add2ToPaned(hpaned3, table11);
    gtk_widget_set_hexpand(treeview6, true);
    gtk_widget_set_vexpand(treeview6, true);
    gtk_widget_set_size_request(treeview6, 300, -1);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview6), true);
    gtk_widget_set_hexpand(current_object_lab, true);
    gtk_paned_set_position(GTK_PANED(hpaned3), 300);

    auto treeviewcolumn10 = addTreecolumn("treeviewcolumn10", "Object");
    auto cellrenderertext8 = addCellrenderer("cellrenderertext8", treeviewcolumn10);
    auto cellrenderertext7 = addCellrenderer("cellrenderertext7", treeviewcolumn10, true);
    gtk_tree_view_column_add_attribute(treeviewcolumn10, cellrenderertext8, "text", 1);
    gtk_tree_view_column_add_attribute(treeviewcolumn10, cellrenderertext8, "background", 4);
    gtk_tree_view_column_add_attribute(treeviewcolumn10, cellrenderertext8, "foreground", 3);
    gtk_tree_view_column_add_attribute(treeviewcolumn10, cellrenderertext7, "markup", 0);
    gtk_tree_view_column_add_attribute(treeviewcolumn10, cellrenderertext7, "background", 4);
    gtk_tree_view_column_add_attribute(treeviewcolumn10, cellrenderertext7, "foreground", 3);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview6), treeviewcolumn10);

    /* ---------- VR Scene - scenegraph properties ---------------------- */
    auto table13 = appendExpander("expander2", "Object", "table13", table11);
    auto table24 = appendExpander("expander9", "Group", "table24", table11);
    auto table12 = appendExpander("expander1", "Transform", "table12", table11);
    auto table16 = appendExpander("expander10", "LoD", "table16", table11);
    auto table17 = appendExpander("expander11", "Geometry", "table17", table11);
    auto table23 = appendExpander("expander14", "Material", "table23", table11);
    auto table29 = appendExpander("expander16", "Primitive", "table29", table11);
    auto table18 = appendExpander("expander12", "Camera", "table18", table11);
    auto table19 = appendExpander("expander13", "Light", "table19", table11);
    auto table28 = appendExpander("expander15", "CSG", "table28", table11);
    auto table38 = appendExpander("expander27", "Entity", "table38", table11);

    /* ---------- VR Scene - scenegraph  object ---------------------- */
    auto label44 = addLabel("label44", "parent:");
    auto label172 = addLabel("label172", "persistency:");
    auto entry17 = addEntry("entry17");
    auto entry21 = addEntry("entry21");
    auto checkbutton6 = addCheckbutton("checkbutton6", "visible");
    auto checkbutton43 = addCheckbutton("checkbutton43", "throw shadow");
    auto checkbutton15 = addCheckbutton("checkbutton15", "pickable");

    gtk_grid_attach(GTK_GRID(table13), label44, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table13), entry17, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table13), label172, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table13), entry21, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table13), checkbutton6, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table13), checkbutton43, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table13), checkbutton15, 0,3,2,1);

    /* ---------- VR Scene - scenegraph group ---------------------- */
    auto checkbutton23 = addCheckbutton("checkbutton23", "group:");
    auto combobox14 = addCombobox("combobox14", "liststore3");
    auto label85 = addLabel("label85", "new:");
    auto entry41 = addEntry("entry41");
    auto button19 = addButton("button19", "sync");
    auto button20 = addButton("button20", "apply");

    gtk_grid_attach(GTK_GRID(table24), checkbutton23, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table24), combobox14, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table24), label85, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table24), entry41, 1,1,1,1);
    gtk_grid_attach(GTK_GRID(table24), button19, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table24), button20, 1,2,1,1);

    /* ---------- VR Scene - scenegraph transform ---------------------- */
    auto frame9 = addVectorFrame("frame9", "pos_entry");
    auto frame10 = addVectorFrame("frame10", "at_entry");
    auto frame11 = addVectorFrame("frame11", "up_entry");
    auto frame12 = addVectorFrame("frame12", "ct_entry");
    auto frame19 = addVectorFrame("frame19", "dir_entry");
    auto frame25 = addVectorFrame("frame25", "scale_entry");
    auto radiobutton19 = addRadiobutton("radiobutton19", "local", 0);
    auto radiobutton20 = addRadiobutton("radiobutton20", "global", radiobutton19);
    auto checkbutton14 = addCheckbutton("checkbutton14", "target:");
    auto entry16 = addEntry("entry16");
    auto label20 = addLabel("label20", "scale:");
    auto entry18 = addEntry("entry18");
    auto button11 = addButton("button11", "Identity");
    auto button4 = addButton("button4", "Focus");
    auto label70 = addLabel("label70", "Constraints:");
    auto checkbutton21 = addCheckbutton("checkbutton21", "Translate:");
    auto radiobutton1 = addRadiobutton("radiobutton1", "axis", 0);
    auto radiobutton2 = addRadiobutton("radiobutton2", "plane", radiobutton1);
    auto checkbutton22 = addCheckbutton("checkbutton22", "Rotate:");
    auto checkbutton18 = addCheckbutton("checkbutton18", "x");
    auto checkbutton19 = addCheckbutton("checkbutton19", "y");
    auto checkbutton20 = addCheckbutton("checkbutton20", "z");
    auto label102 = addLabel("label102", "Physics:");
    auto checkbutton13 = addCheckbutton("checkbutton13", "Physicalized:");
    auto combobox8 = addCombobox("combobox8", "phys_shapes");
    auto checkbutton33 = addCheckbutton("checkbutton33", "dynamic");
    auto label103 = addLabel("label103", "mass:");
    auto entry59 = addEntry("entry59");

    gtk_grid_attach(GTK_GRID(table12), frame9, 0,1,4,1);
    gtk_grid_attach(GTK_GRID(table12), frame10, 0,2,4,1);
    gtk_grid_attach(GTK_GRID(table12), frame19, 0,3,4,1);
    gtk_grid_attach(GTK_GRID(table12), frame11, 0,4,4,1);
    gtk_grid_attach(GTK_GRID(table12), frame25, 0,5,4,1);
    gtk_grid_attach(GTK_GRID(table12), radiobutton19, 0,6,2,1);
    gtk_grid_attach(GTK_GRID(table12), radiobutton20, 2,6,2,1);
    gtk_grid_attach(GTK_GRID(table12), checkbutton14, 0,7,2,1);
    gtk_grid_attach(GTK_GRID(table12), entry16, 2,7,2,1);
    gtk_grid_attach(GTK_GRID(table12), label20, 0,8,2,1);
    gtk_grid_attach(GTK_GRID(table12), entry18, 2,8,2,1);
    gtk_grid_attach(GTK_GRID(table12), button11, 0,9,2,1);
    gtk_grid_attach(GTK_GRID(table12), button4, 2,9,2,1);
    gtk_grid_attach(GTK_GRID(table12), label70, 0,10,4,1);
    gtk_grid_attach(GTK_GRID(table12), checkbutton21, 0,11,2,1);
    gtk_grid_attach(GTK_GRID(table12), radiobutton1, 2,11,1,1);
    gtk_grid_attach(GTK_GRID(table12), radiobutton2, 3,11,1,1);
    gtk_grid_attach(GTK_GRID(table12), frame12, 0,12,4,1);
    gtk_grid_attach(GTK_GRID(table12), checkbutton22, 0,13,1,1);
    gtk_grid_attach(GTK_GRID(table12), checkbutton18, 1,13,1,1);
    gtk_grid_attach(GTK_GRID(table12), checkbutton19, 2,13,1,1);
    gtk_grid_attach(GTK_GRID(table12), checkbutton20, 3,13,1,1);
    gtk_grid_attach(GTK_GRID(table12), label102, 0,14,4,1);
    gtk_grid_attach(GTK_GRID(table12), checkbutton13, 0,15,2,1);
    gtk_grid_attach(GTK_GRID(table12), combobox8, 2,15,2,1);
    gtk_grid_attach(GTK_GRID(table12), checkbutton33, 0,16,2,1);
    gtk_grid_attach(GTK_GRID(table12), label103, 2,16,1,1);
    gtk_grid_attach(GTK_GRID(table12), entry59, 3,16,1,1);

    /* ---------- VR Scene - scenegraph lod ---------------------- */
    auto checkbutton35 = addCheckbutton("checkbutton35", "Decimate");
    auto entry9 = addEntry("entry9");
    auto frame22 = addVectorFrame("frame22", "lod_center");
    auto label49 = addLabel("label49", "add children to this node\nto add distances");
    auto liststore5 = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_BOOLEAN);
    auto treeview8_and_frame = addTreeview("treeview8", "liststore5", GTK_TREE_MODEL(liststore5));
    auto treeview8 = treeview8_and_frame.first;

    gtk_grid_attach(GTK_GRID(table16), checkbutton35, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table16), entry9, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table16), frame22, 0,1,2,1);
    gtk_grid_attach(GTK_GRID(table16), label49, 0,2,2,1);
    gtk_grid_attach(GTK_GRID(table16), treeview8_and_frame.second, 0,3,2,1);

    auto treeviewcolumn6 = addTreecolumn("treeviewcolumn6", "active");
    auto treeviewcolumn7 = addTreecolumn("treeviewcolumn7", "child");
    auto treeviewcolumn17 = addTreecolumn("treeviewcolumn17", "distance");
    auto cellrenderertoggle1 = addToggleCellrenderer("cellrenderertoggle1", treeviewcolumn6);
    auto cellrenderertext27 = addCellrenderer("cellrenderertext27", treeviewcolumn7);
    auto cellrenderertext4 = addCellrenderer("cellrenderertext4", treeviewcolumn17, true);
    gtk_tree_view_column_add_attribute(treeviewcolumn6, cellrenderertoggle1, "activatable", 2);
    gtk_tree_view_column_add_attribute(treeviewcolumn7, cellrenderertext27, "text", 0);
    gtk_tree_view_column_add_attribute(treeviewcolumn17, cellrenderertext4, "text", 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview8), treeviewcolumn6);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview8), treeviewcolumn7);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview8), treeviewcolumn17);

    /* ---------- VR Scene - scenegraph geometry ---------------------- */
    auto label55 = addLabel("label55", "Data:");
    auto geodata = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
    auto treeview9_and_frame = addTreeview("treeview9", "geodata", GTK_TREE_MODEL(geodata), false, false);
    auto treeview9 = treeview9_and_frame.first;
    addTreeviewTextcolumn(treeview9, "Datatype", "cellrenderertext22", 0);
    addTreeviewTextcolumn(treeview9, "N", "cellrenderertext52", 1);
    gtk_grid_attach(GTK_GRID(table17), label55, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table17), treeview9_and_frame.second, 0,1,1,1);
    gtk_widget_set_vexpand(treeview9_and_frame.second, false);

    /* ---------- VR Scene - scenegraph material ---------------------- */
    auto label59 = addLabel("label59", "Name:");
    auto label60 = addLabel("label60", "matName");
    auto checkbutton3 = addCheckbutton("checkbutton3", "Lit");
    auto label67 = addLabel("label67", "Diffuse:");
    auto mat_diffuse = addColorChooser("mat_diffuse");
    auto label71 = addLabel("label71", "Specular:");
    auto mat_specular = addColorChooser("mat_specular");
    auto label72 = addLabel("label72", "Ambient:");
    auto mat_ambient = addColorChooser("mat_ambient");
    auto label58 = addLabel("label58", "Pointsize:");
    auto entry35 = addEntry("entry35");
    auto checkbutton5 = addCheckbutton("checkbutton5", "Texture:");
    auto label201 = addLabel("label201", " "); // TODO: set texture name or path
    auto label165 = addLabel("label165", "Size:");
    auto label168 = addLabel("label168", " ");
    auto label166 = addLabel("label166", "Channels:");
    auto label167 = addLabel("label167", " ");

    gtk_grid_attach(GTK_GRID(table23), label59, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table23), label60, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table23), checkbutton3, 2,0,1,1);
    gtk_grid_attach(GTK_GRID(table23), label67, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table23), mat_diffuse, 1,1,2,1);
    gtk_grid_attach(GTK_GRID(table23), label71, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table23), mat_specular, 1,2,2,1);
    gtk_grid_attach(GTK_GRID(table23), label72, 0,3,1,1);
    gtk_grid_attach(GTK_GRID(table23), mat_ambient, 1,3,2,1);
    gtk_grid_attach(GTK_GRID(table23), label58, 0,4,1,1);
    gtk_grid_attach(GTK_GRID(table23), entry35, 1,4,2,1);
    gtk_grid_attach(GTK_GRID(table23), checkbutton5, 0,5,1,1);
    gtk_grid_attach(GTK_GRID(table23), label201, 1,5,2,1);
    gtk_grid_attach(GTK_GRID(table23), label165, 0,6,1,1);
    gtk_grid_attach(GTK_GRID(table23), label168, 1,6,2,1);
    gtk_grid_attach(GTK_GRID(table23), label166, 0,7,1,1);
    gtk_grid_attach(GTK_GRID(table23), label167, 1,7,2,1);

    /* ---------- VR Scene - scenegraph primitive ---------------------- */
    auto checkbutton28 = addCheckbutton("checkbutton28", "Primitive:");
    auto combobox21 = addCombobox("combobox21", "prim_list");
    auto primitive_opts = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    auto treeview12_and_frame = addTreeview("treeview12", "primitive_opts", GTK_TREE_MODEL(primitive_opts), false, false);
    auto treeview12 = treeview12_and_frame.first;
    addTreeviewTextcolumn(treeview12, "Parameter", "cellrenderertext32", 0);
    addTreeviewTextcolumn(treeview12, "Value", "cellrenderertext33", 1, true);
    gtk_grid_attach(GTK_GRID(table29), checkbutton28, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table29), combobox21, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table29), treeview12_and_frame.second, 0,1,2,1);
    gtk_widget_set_vexpand(treeview12_and_frame.second, false);


    /* ---------- VR Scene - scenegraph camera ---------------------- */
    auto checkbutton17 = addCheckbutton("checkbutton17", "accept setup root");
    auto label104 = addLabel("label104", "Projection:");
    auto combobox23 = addCombobox("combobox23", "cam_proj");
    auto label105 = addLabel("label105", "Aspect:");
    auto entry60 = addEntry("entry60");
    auto label106 = addLabel("label106", "Fov:");
    auto entry61 = addEntry("entry61");
    auto label29 = addLabel("label29", "Near:");
    auto entry6 = addEntry("entry6");
    auto label30 = addLabel("label30", "Far:");
    auto entry7 = addEntry("entry7");

    gtk_grid_attach(GTK_GRID(table18), checkbutton17, 0,0,4,1);
    gtk_grid_attach(GTK_GRID(table18), label104, 0,1,2,1);
    gtk_grid_attach(GTK_GRID(table18), combobox23, 2,1,2,1);
    gtk_grid_attach(GTK_GRID(table18), label105, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table18), entry60, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table18), label106, 2,2,1,1);
    gtk_grid_attach(GTK_GRID(table18), entry61, 3,2,1,1);
    gtk_grid_attach(GTK_GRID(table18), label29, 0,3,1,1);
    gtk_grid_attach(GTK_GRID(table18), entry6, 1,3,1,1);
    gtk_grid_attach(GTK_GRID(table18), label30, 2,3,1,1);
    gtk_grid_attach(GTK_GRID(table18), entry7, 3,3,1,1);

    /* ---------- VR Scene - scenegraph light ---------------------- */
    auto label100 = addLabel("label100", "Light type:");
    auto combobox2 = addCombobox("combobox2", "light_types");
    auto checkbutton31 = addCheckbutton("checkbutton31", "on");
    auto label37 = addLabel("label37", "Light beacon:");
    auto button27 = addLabel("button27", "button");
    auto label96 = addLabel("label96", "Attenuation: A(C,L,Q) = C + L.r + Q.r²");

    auto hbox8 = addBox("hbox8", GTK_ORIENTATION_HORIZONTAL);
    auto label97 = addLabel("label97", "C:");
    auto entry44 = addEntry("entry44");
    auto label98 = addLabel("label98", "L:");
    auto entry45 = addEntry("entry45");
    auto label99 = addLabel("label99", "Q:");
    auto entry46 = addEntry("entry46");
    gtk_box_pack_start(GTK_BOX(hbox8), label97, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox8), entry44, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox8), label98, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox8), entry45, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox8), label99, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox8), entry46, true, true, 0);

    auto label101 = addLabel("label101", "Light Colors (D,A,S):");

    auto hbox9 = addBox("hbox9", GTK_ORIENTATION_HORIZONTAL);
    auto light_diff = addColorChooser("light_diff");
    auto light_amb = addColorChooser("light_amb");
    auto light_spec = addColorChooser("light_spec");
    gtk_box_pack_start(GTK_BOX(hbox9), light_diff, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox9), light_amb, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox9), light_spec, false, true, 0);

    auto checkbutton32 = addCheckbutton("checkbutton32", "shadow");
    auto combobox22 = addCombobox("combobox22", "shadow_types");
    auto label95 = addLabel("label95", "Shadow color:");
    auto shadow_col = addColorChooser("shadow_col");
    auto checkbutton2 = addCheckbutton("checkbutton2", "shadow volume:");
    auto entry36 = addEntry("entry36");
    gtk_entry_set_text(GTK_ENTRY(entry36), "10");
    auto light_params = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    auto treeview13_and_frame = addTreeview("treeview13", "light_params", GTK_TREE_MODEL(light_params));
    auto treeview13 = treeview13_and_frame.first;

    gtk_grid_attach(GTK_GRID(table19), label100, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table19), combobox2, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table19), checkbutton31, 0,1,2,1);
    gtk_grid_attach(GTK_GRID(table19), label37, 0,2,1,1);
    gtk_grid_attach(GTK_GRID(table19), button27, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(table19), label96, 0,3,2,1);
    gtk_grid_attach(GTK_GRID(table19), hbox8, 0,4,2,1);
    gtk_grid_attach(GTK_GRID(table19), label101, 0,5,2,1);
    gtk_grid_attach(GTK_GRID(table19), hbox9, 0,6,2,1);
    gtk_grid_attach(GTK_GRID(table19), checkbutton32, 0,7,1,1);
    gtk_grid_attach(GTK_GRID(table19), combobox22, 1,7,1,1);
    gtk_grid_attach(GTK_GRID(table19), label95, 0,8,1,1);
    gtk_grid_attach(GTK_GRID(table19), shadow_col, 1,8,1,1);
    gtk_grid_attach(GTK_GRID(table19), checkbutton2, 0,9,1,1);
    gtk_grid_attach(GTK_GRID(table19), entry36, 1,9,1,1);
    gtk_grid_attach(GTK_GRID(table19), treeview13_and_frame.second, 0,10,2,1);

    /* ---------- VR Scene - scenegraph csg ---------------------- */
    auto checkbutton27 = addCheckbutton("checkbutton27", "edit mode");
    auto label79 = addLabel("label79", "operation:");
    auto combobox19 = addCheckbutton("combobox19", "csg_operations");
    gtk_grid_attach(GTK_GRID(table28), checkbutton27, 0,0,2,1);
    gtk_grid_attach(GTK_GRID(table28), label79, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(table28), combobox19, 1,1,1,1);

    /* ---------- VR Scene - scenegraph entity ---------------------- */
    auto label141 = addLabel("label141", "Concept:");
    auto label145 = addLabel("label145", "NONE");
    auto properties = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    auto treeview11_and_frame = addTreeview("treeview11", "properties", GTK_TREE_MODEL(properties));
    auto treeview11 = treeview11_and_frame.first;
    addTreeviewTextcolumn(treeview11, "property", "cellrenderertoggle2", 0);
    addTreeviewTextcolumn(treeview11, "value", "cellrenderertext49", 1);
    addTreeviewTextcolumn(treeview11, "type", "cellrenderertext50", 2, true);
    gtk_grid_attach(GTK_GRID(table38), label141, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table38), label145, 1,0,1,1);
    gtk_grid_attach(GTK_GRID(table38), treeview11_and_frame.second, 0,1,2,1);

    cout << " ..building all widgets done!" << endl;
}








