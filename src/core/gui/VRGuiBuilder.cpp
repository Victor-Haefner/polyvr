#include <gtk/gtk.h>

#include "VRGuiBuilder.h"
#include "VRGuiFile.h"

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
        b->buildBaseUI();
        /*string path = getDefaultPath(standalone);
        if (!VRGuiFile::exists(path)) cerr << "FATAL ERROR: " << path << " not found\n";
        else b->read(path);*/
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

GtkWidget* VRGuiBuilder::get_widget(string name) {
    /*auto w = (GtkWidget*)gtk_builder_get_object(builder, name.c_str());
    if (!w) cout << " Error in VRGuiBuilder::get_widget, no widget called " << name << endl;
    return w;*/

    if (widgets.count(name)) return widgets[name];
    else {
        cout << " Error in VRGuiBuilder::get_widget, no object called " << name << endl;
        return 0;
    }
}

_GtkObject* VRGuiBuilder::get_object(string name) {
    /*auto o = (_GtkObject*)gtk_builder_get_object(builder, name.c_str());
    if (!o) cout << " Error in VRGuiBuilder::get_object, no object called " << name << endl;
    return o;*/

    if (objects.count(name)) return objects[name];
    else {
        cout << " Error in VRGuiBuilder::get_object, no object called " << name << endl;
        return 0;
    }
}

GtkWidget* addWindow(string ID, string name) {
    GtkWidget* w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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

GtkWidget* addLabel(string ID, string label) {
    auto p = gtk_label_new(label.c_str());
    VRGuiBuilder::get()->reg_widget(p, ID);
    return p;
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

GtkToolItem* addToolButton(string ID, string stock, GtkWidget* bar) {
    auto item = gtk_tool_button_new_from_stock(stock.c_str());
    VRGuiBuilder::get()->reg_widget(GTK_WIDGET(item), ID);
    gtk_toolbar_insert(GTK_TOOLBAR(bar), item, 0);
    return item;
}

GtkToolItem* addToggleToolButton(string ID, string stock, GtkWidget* bar) {
    auto item = gtk_toggle_tool_button_new_from_stock(stock.c_str());
    VRGuiBuilder::get()->reg_widget(GTK_WIDGET(item), ID);
    gtk_toolbar_insert(GTK_TOOLBAR(bar), item, 0);
    return item;
}

GtkWidget* addNotebook(string ID) {
    auto n = gtk_notebook_new();
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
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

GtkWidget* addEntry(string ID) {
    auto n = gtk_entry_new();
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addDrawingArea(string ID) {
    auto n = gtk_drawing_area_new();
    VRGuiBuilder::get()->reg_widget(n, ID);
    return n;
}

GtkWidget* addCombobox(string ID, string mID) {
    auto m = gtk_list_store_new(1, G_TYPE_STRING);
    auto n = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(m));
    VRGuiBuilder::get()->reg_widget(n, ID);
    VRGuiBuilder::get()->reg_object(G_OBJECT(m), mID);
    return n;
}

GtkWidget* addScrolledWindow(string ID) {
    auto n = gtk_scrolled_window_new(0, 0);
    VRGuiBuilder::get()->reg_widget(n, ID);
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

GtkWidget* addTreeview(string ID, string mID, GtkTreeModel* m) {
    auto n = gtk_tree_view_new_with_model(m);
    VRGuiBuilder::get()->reg_widget(n, ID);
    VRGuiBuilder::get()->reg_object(G_OBJECT(m), mID);
    return n;
}

void addNotebookPage(GtkWidget* notebook, GtkWidget* content, string label) {
    auto lbl = gtk_label_new(label.c_str());
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content, lbl);

    /*auto i = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content, lbl);
    auto page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
    //gtk_widget_set_hexpand(page, true);
    //gtk_widget_set_vexpand(page, true);
    GValue v = G_VALUE_INIT;
    g_value_init(&v, G_TYPE_BOOLEAN);
    g_value_set_boolean(&v, true);
    g_object_set_property(G_OBJECT(page), "tab-fill", &v);*/
}

void addTreeviewTextcolumn(GtkWidget* treeview, string cName, int pos) {
    auto renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, cName.c_str(), renderer, "text", pos, NULL);
}

void VRGuiBuilder::buildBaseUI() {
    auto window1 = addWindow("window1", "PolyVR");
    auto main_frame = addGrid("main_frame");
    gtk_container_add(GTK_CONTAINER(window1), main_frame);

    /* ---------- head section ---------------------- */
    auto table20 = addGrid("table20");
    auto hseparator1 = addSeparator("hseparator1", GTK_ORIENTATION_HORIZONTAL);
    auto hpaned1 = addPaned("hpaned1", GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(main_frame), table20, 0,0,2,1);
    gtk_grid_attach(GTK_GRID(main_frame), hseparator1, 0,1,2,1);
    gtk_grid_attach(GTK_GRID(main_frame), hpaned1, 0,2,2,1);
    gtk_widget_set_vexpand(hpaned1, true);

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

    auto toolbutton18 = addToolButton("toolbutton18", "gtk-paste", toolbar1);
    auto toolbutton17 = addToolButton("toolbutton17", "gtk-about", toolbar1);
    auto toolbutton3 = addToolButton("toolbutton3", "gtk-quit", toolbar1);
    auto toolbutton28 = addToolButton("toolbutton28", "gtk-stop", toolbar1);
    auto toolbutton50 = addToolButton("toolbutton50", "gtk-go-up", toolbar1);
    auto toolbutton5 = addToolButton("toolbutton5", "gtk-save-as", toolbar1);
    auto toolbutton4 = addToolButton("toolbutton4", "gtk-save", toolbar1);
    auto toolbutton21 = addToolButton("toolbutton21", "gtk-open", toolbar1);
    auto toolbutton1 = addToolButton("toolbutton1", "gtk-new", toolbar1);

    /* ---------- core section ---------------------- */
    auto notebook1 = addNotebook("notebook1");
    auto vpaned1 = addPaned("vpaned1", GTK_ORIENTATION_VERTICAL);
    auto vbox5 = addBox("vbox5", GTK_ORIENTATION_VERTICAL);
    auto hbox15 = addBox("hbox15", GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_add1(GTK_PANED(hpaned1), notebook1);
    gtk_paned_add2(GTK_PANED(hpaned1), vpaned1);
    gtk_paned_add1(GTK_PANED(vpaned1), vbox5);
    gtk_paned_add2(GTK_PANED(vpaned1), hbox15);
    gtk_widget_set_hexpand(notebook1, true);

    /* ---------- right core section ---------------------- */
    auto hbox1 = addBox("hbox1", GTK_ORIENTATION_HORIZONTAL);
    auto glarea = addDrawingArea("glarea");
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


    auto togglebutton1 = addToggleToolButton("togglebutton1", "gtk-leave-fullscreen", toolbar6);

    auto toolbar7 = addToolbar("toolbar7", GTK_ICON_SIZE_LARGE_TOOLBAR, GTK_ORIENTATION_VERTICAL);
    auto pause_terminal = addToggleToolButton("pause_terminal", "gtk-media-pause", toolbar7);
    auto network_verbose = addToggleToolButton("network_verbose", "gtk-network", toolbar7);
    auto toolbutton25 = addToolButton("toolbutton25", "gtk-go-down", toolbar7);
    auto toolbutton24 = addToolButton("toolbutton24", "gtk-clear", toolbar7);
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
    auto appSearch = addEntry("appSearch");
    auto scrolledwindow9 = addScrolledWindow("scrolledwindow9");
    auto scrolledwindow10 = addScrolledWindow("scrolledwindow10");
    auto viewport1 = addViewport("viewport1");
    auto viewport4 = addViewport("viewport4");
    auto favorites_tab = addGrid("favorites_tab");
    auto examples_tab = addGrid("examples_tab");
    gtk_box_pack_start(GTK_BOX(vbox3), hbox16, false, true, 0);
    gtk_box_pack_start(GTK_BOX(vbox3), notebook2, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox16), label171, false, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox16), appSearch, true, true, 0);
    addNotebookPage(notebook2, scrolledwindow9, "Favorites");
    addNotebookPage(notebook2, scrolledwindow10, "Examples");
    gtk_container_add(GTK_CONTAINER(scrolledwindow9), viewport1);
    gtk_container_add(GTK_CONTAINER(scrolledwindow10), viewport4);
    gtk_container_add(GTK_CONTAINER(viewport1), favorites_tab);
    gtk_container_add(GTK_CONTAINER(viewport4), examples_tab);

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

    /* ---------- new project dialog ---------------------- */ // deprecated
    /*auto NewProject = addDialog("NewProject");
    auto dialog_vbox3 = gtk_dialog_get_content_area(GTK_DIALOG(NewProject));
    auto dialog_action_area3 = gtk_dialog_get_action_area(GTK_DIALOG(NewProject));
    auto label52 = addLabel("label52", "New Poject:");
    auto entry49 = addEntry("entry49");
    auto button14 = addButton("button14", "Cancel");
    auto button15 = addButton("button15", "Start");
    gtk_box_pack_start(GTK_BOX(dialog_vbox3), label52, true, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox3), entry49, true, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_action_area3), button14, false, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_action_area3), button15, false, true, 0);
    gtk_window_set_transient_for(GTK_WINDOW(NewProject), GTK_WINDOW(window1));*/

    /* ---------- recorder ---------------------- */  // TODO: to test!
    auto recorder = addDialog("recorder");
    auto dialog_vbox15 = gtk_dialog_get_content_area(GTK_DIALOG(recorder));
    auto dialog_action_area15 = gtk_dialog_get_action_area(GTK_DIALOG(recorder));
    auto hbox21 = addBox("hbox21", GTK_ORIENTATION_HORIZONTAL);
    auto hbox20 = addBox("hbox20", GTK_ORIENTATION_HORIZONTAL);
    auto label149 = addLabel("label149", "Idle");
    auto label151 = addLabel("label151", "codec:");
    auto label150 = addLabel("label150", "bitrate multiplier:");
    auto codecs = addCombobox("codecs", "codecList");
    auto entry27 = addEntry("entry27");
    gtk_box_pack_start(GTK_BOX(dialog_vbox15), label149, true, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox15), hbox21, true, true, 0);
    gtk_box_pack_start(GTK_BOX(dialog_vbox15), hbox20, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox21), label151, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox21), codecs, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox20), label150, true, true, 0);
    gtk_box_pack_start(GTK_BOX(hbox20), entry27, true, true, 0);
    gtk_window_set_transient_for(GTK_WINDOW(recorder), GTK_WINDOW(window1));

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

    /* ---------- file open dialog ---------------------- */
    auto file_dialog = GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(window1), GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel", 0, "Open", 0, 0));
    auto dialog_action_area1 = gtk_dialog_get_action_area(GTK_DIALOG(file_dialog));
    auto buttons = gtk_container_get_children(GTK_CONTAINER(dialog_action_area1));
    auto button3 = g_list_nth_data(buttons, 0);
    auto button9 = g_list_nth_data(buttons, 1);
    reg_widget(GTK_WIDGET(file_dialog), "file_dialog");
    reg_widget(GTK_WIDGET(button3), "button3");
    reg_widget(GTK_WIDGET(button9), "button9");

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
    auto treeview10 = addTreeview("treeview10", "liststore4", GTK_TREE_MODEL(liststore4));
    addNotebookPage(notebook4, treeview10, "Name Dictionaries");
    addTreeviewTextcolumn(treeview10, "column", 0);
    addTreeviewTextcolumn(treeview10, "column", 1);

    auto table47 = addGrid("table47");
    auto scrolledwindow1 = addScrolledWindow("scrolledwindow1");
    auto scrolledwindow13 = addScrolledWindow("scrolledwindow13");
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
    gtk_grid_attach(GTK_GRID(table31), table47, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(table31), scrolledwindow1, 1,0,1,2);
    gtk_grid_attach(GTK_GRID(table31), scrolledwindow13, 0,1,1,1);
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

    auto prof_fkts = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    auto treeview15 = addTreeview("treeview15", "prof_fkts", GTK_TREE_MODEL(prof_fkts));
    gtk_container_add(GTK_CONTAINER(scrolledwindow13), treeview15);
    gtk_widget_set_vexpand(treeview15, true);
    addTreeviewTextcolumn(treeview15, "function", 0);
    addTreeviewTextcolumn(treeview15, "time (μs)", 1);
}








