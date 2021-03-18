#include <gtk/gtk.h>
#include "VRGuiFile.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"

#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"

#include <boost/filesystem.hpp>

GtkWidget* VRGuiFile::dialog = 0;
GtkListStore* VRGuiFile::fileOpenPresets = 0;
GtkWidget* VRGuiFile::button3 = 0;
GtkWidget* VRGuiFile::button9 = 0;
GtkWidget* VRGuiFile::treeview = 0;
GtkListStore* VRGuiFile::filesStore;
GtkWidget* VRGuiFile::pathEntry = 0;
GtkWidget* VRGuiFile::fileEntry = 0;
GtkTable* VRGuiFile::addon = 0;
GtkTable* VRGuiFile::geoImportWidget = 0;
GtkTable* VRGuiFile::saveasWidget = 0;
function<void()> VRGuiFile::sigApply = function<void()>();
function<void()> VRGuiFile::sigClose = function<void()>();
function<void()> VRGuiFile::sigSelect = function<void()>();
string VRGuiFile::currentFolder = "";
string VRGuiFile::selection = "";
bool VRGuiFile::cache_override = 0;
float VRGuiFile::scale = 1;
string VRGuiFile::preset = "SOLIDWORKS-VRML2";

typedef boost::filesystem::path path;

#ifdef _WIN32
const bool useCustomWidget = true;
#else
const bool useCustomWidget = false;
#endif

void VRGuiFile::init() {
    cout << " build file open dialog" << endl;
    auto window1 = VRGuiBuilder::get()->get_widget("window1");
    fileOpenPresets = gtk_list_store_new(1, G_TYPE_STRING);

    if (useCustomWidget) {
        dialog = gtk_dialog_new();
        button3 = gtk_button_new_with_label("Cancel");
        button9 = gtk_button_new_with_label("Open");

        gtk_widget_set_size_request(dialog, 800, 600);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window1));
        auto dialog_action_area = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
        gtk_box_pack_start(GTK_BOX(dialog_action_area), button3, false, true, 0);
        gtk_box_pack_start(GTK_BOX(dialog_action_area), button9, false, true, 0);

        auto dialog_vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        pathEntry = gtk_entry_new();
        fileEntry = gtk_entry_new();
        filesStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
        treeview = gtk_tree_view_new_with_model( GTK_TREE_MODEL(filesStore) );
        auto scrolledWindow = gtk_scrolled_window_new(0, 0);
        gtk_widget_set_hexpand(scrolledWindow, true);
        gtk_widget_set_vexpand(scrolledWindow, true);
        gtk_widget_set_hexpand(treeview, true);
        gtk_widget_set_vexpand(treeview, true);
        gtk_container_add(GTK_CONTAINER(scrolledWindow), treeview);
        gtk_box_pack_start(GTK_BOX(dialog_vbox), pathEntry, false, true, 0);
        gtk_box_pack_start(GTK_BOX(dialog_vbox), fileEntry, false, true, 0);
        gtk_box_pack_start(GTK_BOX(dialog_vbox), scrolledWindow, true, true, 0);

        auto c = gtk_tree_view_column_new();
        auto r = gtk_cell_renderer_text_new();
        auto p = gtk_cell_renderer_pixbuf_new();
        gtk_tree_view_column_pack_start(c, p, false);
        gtk_tree_view_column_pack_start(c, r, true);
        gtk_tree_view_column_add_attribute(c, r, "text", 0);
        gtk_tree_view_column_add_attribute(c, p, "stock-id", 1);
        gtk_tree_view_column_set_title(c, "Name");
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c);
    } else {
        dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(window1), GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel", 0, "Open", 0, 0);
        auto dialog_action_area1 = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
        auto buttons = gtk_container_get_children(GTK_CONTAINER(dialog_action_area1));
        if (!buttons) {
            auto header = gtk_dialog_get_header_bar(GTK_DIALOG(dialog));
            buttons = gtk_container_get_children(GTK_CONTAINER(header));
            button3 = GTK_WIDGET(g_list_nth_data(buttons, 1));
            button9 = GTK_WIDGET(g_list_nth_data(buttons, 2));
        } else {
            button3 = GTK_WIDGET(g_list_nth_data(buttons, 0));
            button9 = GTK_WIDGET(g_list_nth_data(buttons, 1));
        }
    }

    if (!useCustomWidget) {
        connect_signal<void>(dialog, bind(VRGuiFile::select), "selection_changed");
        connect_signal<void>(dialog, bind(VRGuiFile::apply), "file_activated");
        gtk_file_chooser_set_action((GtkFileChooser*)dialog, GTK_FILE_CHOOSER_ACTION_OPEN);
        connect_signal<bool, GdkEvent*>(dialog, bind(VRGuiFile::keyApply, placeholders::_1), "event");
    } else {
        connect_signal<void>(treeview, bind(VRGuiFile::select), "cursor_changed");
        connect_signal<void, GtkTreePath*, GtkTreeViewColumn*>(treeview, bind(VRGuiFile::activate, placeholders::_1, placeholders::_2), "row_activated");
        connect_signal<void>(pathEntry, bind(VRGuiFile::on_edit_path_entry, (_GtkEntry*)pathEntry), "activate");
        setEntryCallback(fileEntry, bind(VRGuiFile::on_filename_edited), false, true, true);
    }

    connect_signal<void>(button3, bind(&VRGuiFile::close), "clicked");
    connect_signal<void>(button9, bind(&VRGuiFile::apply), "clicked");
    disableDestroyDiag(GTK_WIDGET(VRGuiFile::dialog));
}

void VRGuiFile::open(string button, int action, string title) {
    if (dialog == 0) init();
    gtk_widget_show_all(GTK_WIDGET(VRGuiFile::dialog));

    gtk_button_set_label(GTK_BUTTON(button9), button.c_str());
    gtk_button_set_label(GTK_BUTTON(button3), "Cancel");

    gtk_window_set_title((GtkWindow*)dialog, title.c_str());
    if (!useCustomWidget) {
        gtk_file_chooser_set_action((GtkFileChooser*)dialog, GtkFileChooserAction(action));
    }
}

void VRGuiFile::close() {
    //OSG::VRSetup::getCurrent()->pauseRendering(false);
    if (dialog == 0) init();
    setWidget(0);
    gtk_widget_hide(GTK_WIDGET(VRGuiFile::dialog));
    if (sigClose) sigClose();
    clearFilter();
}

void VRGuiFile::setWidget(GtkTable* table, bool expand, bool fill) {
    if (addon == table) return;
    auto vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    if (!vbox) cout << "Error in VRGuiFile::setWidget: no file dialog vbox!" << endl;

    // sub
    if (addon) {
        g_object_ref(addon); // increase ref count
        gtk_container_remove((GtkContainer*)vbox, (GtkWidget*)addon);
    }
    addon = table;
    if (table == 0) return;

    // add
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(table), expand, fill, 0);
    gtk_widget_show_all(GTK_WIDGET(vbox));
}

void VRGuiFile::on_toggle_cache_override(GtkCheckButton* b) {
    cache_override = gtk_toggle_button_get_active((GtkToggleButton*)b);
}

void VRGuiFile::on_edit_path_entry(GtkEntry* e) {
    auto p = gtk_entry_get_text(e);
    gotoPath(p?p:"");
}

void VRGuiFile::on_filename_edited() {
    auto p = gtk_entry_get_text(GTK_ENTRY(fileEntry));
    selection = p?p:"";
}

void VRGuiFile::on_edit_import_scale(GtkEntry* e) {
    scale = toFloat( gtk_entry_get_text(e) );
}

void VRGuiFile::on_change_preset(GtkComboBox* b) {
    string name = getComboboxPtrText(b);
    if (name == "") return;
    preset = name;
}

void VRGuiFile::setGeoLoadWidget() {
    if (geoImportWidget == 0) {
        geoImportWidget = (GtkTable*)gtk_table_new(0,0,false);
        auto fixed = gtk_fixed_new();
        auto cache_override = gtk_check_button_new_with_label("ignore cache");
        auto label163 = gtk_label_new("Loader:");
        auto openFileWarning1 = gtk_label_new("Scale:");
        auto entry21 = gtk_entry_new();

        auto store = VRGuiBuilder::get()->get_object("fileOpenPresets");
        auto combobox15 = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
        gtk_combo_box_set_id_column(GTK_COMBO_BOX(combobox15), 0);
        gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(combobox15), 0);
        gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(combobox15), GTK_SENSITIVITY_ON);

        gtk_misc_set_alignment((GtkMisc*)openFileWarning1, 1, 0);

        GtkAttachOptions opts = GtkAttachOptions(GTK_FILL|GTK_EXPAND);
        GtkAttachOptions opts3 = GTK_FILL;
        GtkAttachOptions opts2 = GtkAttachOptions(0);

        gtk_table_attach(geoImportWidget, (GtkWidget*)fixed, 0,3,0,1, opts, opts2, 0, 0);
        gtk_table_attach(geoImportWidget, (GtkWidget*)cache_override, 3,4,0,1, opts3, opts2, 0, 0);
        gtk_table_attach(geoImportWidget, (GtkWidget*)label163, 0,1,1,2, opts, opts2, 0, 0);
        gtk_table_attach(geoImportWidget, (GtkWidget*)combobox15, 1,2,1,2, opts, opts2, 0, 0);
        gtk_table_attach(geoImportWidget, (GtkWidget*)openFileWarning1, 2,3,1,2, opts, opts2, 10, 0);
        gtk_table_attach(geoImportWidget, (GtkWidget*)entry21, 3,4,1,2, opts3, opts2, 0, 0);

        connect_signal<void>(entry21, bind(VRGuiFile::on_edit_import_scale, (_GtkEntry*)entry21), "activate");
        connect_signal<void>(cache_override, bind(VRGuiFile::on_toggle_cache_override, (_GtkCheckButton*)cache_override), "toggled");
        connect_signal<void>(combobox15, bind(VRGuiFile::on_change_preset, (_GtkComboBox*)combobox15), "changed");

        vector<string> presets = { "SOLIDWORKS-VRML2", "OSG", "COLLADA", "PVR" };
        fillStringListstore("fileOpenPresets", presets);
        gtk_combo_box_set_model((GtkComboBox*)combobox15, (GtkTreeModel*)store);
        gtk_combo_box_set_active((GtkComboBox*)combobox15, getListStorePos("fileOpenPresets", "SOLIDWORKS-VRML2"));
    }

    setWidget(geoImportWidget);
}

void VRGuiFile::setSaveasWidget( function<void(GtkCheckButton*)> sig ) {
    if (saveasWidget == 0) {
        saveasWidget = (GtkTable*)gtk_table_new(0,0,false);
        auto fixed = gtk_fixed_new();
        auto encrypt = gtk_check_button_new_with_label("encrypt");

        GtkAttachOptions opts = GtkAttachOptions(GTK_FILL|GTK_EXPAND);
        GtkAttachOptions opts3 = GTK_FILL;
        GtkAttachOptions opts2 = GtkAttachOptions(0);

        gtk_table_attach(saveasWidget, (GtkWidget*)fixed, 0,3,0,1, opts, opts2, 0, 0);
        gtk_table_attach(saveasWidget, (GtkWidget*)encrypt, 3,4,0,1, opts3, opts2, 0, 0);

        connect_signal((GtkWidget*)encrypt, sig, "toggled");
    }

    setWidget(saveasWidget);
}


void VRGuiFile::addFilter(string name, int N, ...) {
    if (dialog == 0) init();

    auto filter = (GtkFileFilter*)gtk_file_filter_new();
    gtk_file_filter_set_name(filter, name.c_str());

    va_list ap;
    va_start(ap, N);
    for (int i=0; i<N; i++) {
        string pattern = string( va_arg(ap, const char*) );
        gtk_file_filter_add_pattern(filter, pattern.c_str());
    }
    va_end(ap);

    if (!useCustomWidget) {
        gtk_file_chooser_add_filter((GtkFileChooser*)dialog, filter);
    }
}

void VRGuiFile::clearFilter() {
    if (dialog == 0) init();
    if (!useCustomWidget) {
        GSList* filters = gtk_file_chooser_list_filters(GTK_FILE_CHOOSER(dialog));
        for (GSList* elem = filters; elem != NULL; elem = g_slist_next(elem))
            gtk_file_chooser_remove_filter((GtkFileChooser*)dialog, (GtkFileFilter*)elem->data);
        g_slist_free (filters);
    }
}

void VRGuiFile::setFile(string file) {
    if (dialog == 0) init();
    if (!useCustomWidget) {
        gtk_file_chooser_set_current_name((GtkFileChooser*)dialog, file.c_str());
    } else {
        gtk_entry_set_text(GTK_ENTRY(fileEntry), file.c_str());
        selection = file;
    }
}

void VRGuiFile::gotoPath(string path) {
    cout << "VRGuiFile::gotoPath " << path << endl;
    if (!exists(path)) path = "./";
    if (dialog == 0) init();
    if (!useCustomWidget) {
        gtk_file_chooser_set_current_folder((GtkFileChooser*)dialog, path.c_str());
    } else {
        currentFolder = canonical( path );
        gtk_entry_set_text(GTK_ENTRY(pathEntry), path.c_str());
        gtk_list_store_clear(filesStore);
        GtkTreeIter iter;

        vector<string> folders = { ".." };
        vector<string> files;

        for (auto f : openFolder(path)) {
            if (f[0] == '.') continue; // ignore hidden files
            if (isFolder(path+"/"+f)) folders.push_back(f);
            if (isFile(path+"/"+f)) files.push_back(f);
        }

        for (auto f : folders) {
            gtk_list_store_append(filesStore, &iter);
            gtk_list_store_set(filesStore, &iter, 0, f.c_str(), -1);
            gtk_list_store_set(filesStore, &iter, 1, "gtk-directory", -1);
        }

        for (auto f : files) {
            gtk_list_store_append(filesStore, &iter);
            gtk_list_store_set(filesStore, &iter, 0, f.c_str(), -1);
            gtk_list_store_set(filesStore, &iter, 1, "gtk-file", -1);
        }
    }
}

void VRGuiFile::select() {
    if (sigSelect) sigSelect();

    if (useCustomWidget) {
        GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
        GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));

        GtkTreeIter selected;
        gchar* new_text;
        if (!gtk_tree_selection_get_selected(sel, &model, &selected)) return;
        gtk_tree_model_get(model, &selected, 0, &new_text, -1);
        selection = new_text?new_text:"";
        gtk_entry_set_text(GTK_ENTRY(fileEntry), selection.c_str());
    }
}

void VRGuiFile::activate(GtkTreePath* path, GtkTreeViewColumn* column) {
    string p = currentFolder+"/"+selection;
    if (currentFolder == "/") p = "/"+selection;
    if (isFile(p)) apply();
    if (isFolder(p)) gotoPath(p);
}

void VRGuiFile::apply() {
    if (dialog == 0) init();
    gtk_widget_hide((GtkWidget*)dialog);
    if (sigApply) sigApply();
    setWidget(0);
}

bool VRGuiFile::keyApply(GdkEvent* k) {
    if (k->key.keyval != 65293 || k->key.type != GDK_KEY_RELEASE) return false;
    apply();
    return true;
}

void VRGuiFile::setCallbacks(function<void()> sa, function<void()> sc, function<void()> ss) {
    sigApply = sa;
    sigClose = sc;
    sigSelect = ss;
}

string VRGuiFile::getPath() {
    if (!useCustomWidget) {
        gchar* filename = gtk_file_chooser_get_filename((GtkFileChooser*)dialog);
        string res = filename?filename:"";
        g_free(filename);
        return res;
    } else {
        return currentFolder+"/"+selection;
    }
}

// Return path when appended to a_From will resolve to same as a_To
path make_relative( path a_From, path a_To ) {
    a_From = boost::filesystem::absolute( a_From );
    a_To = boost::filesystem::absolute( a_To );
    path ret;
    path::const_iterator itrFrom( a_From.begin() ), itrTo( a_To.begin() );
    // Find common base
    for( path::const_iterator toEnd( a_To.end() ), fromEnd( a_From.end() ) ;
        itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo;
        ++itrFrom, ++itrTo );
    // Navigate backwards in directory to reach previously found base
    for( path::const_iterator fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom ) {
        if( (*itrFrom) != "." ) ret /= "..";
    }
    // Now navigate down the directory branch
	for (; itrTo != a_To.end(); ++itrTo) ret /= *itrTo;
    return ret;
}

string VRGuiFile::getRelativePath_toOrigin() {
    path a(getPath());
    path b(OSG::VRSceneManager::get()->getOriginalWorkdir());
    return make_relative( b, a ).string();
}

string VRGuiFile::getRelativePath_toWorkdir() {
    path a(getPath());
	path b = boost::filesystem::current_path();
    return make_relative( b, a ).string();
}

bool VRGuiFile::exists(string path) { return boost::filesystem::exists(path); }
bool VRGuiFile::isDir(string path) { return boost::filesystem::is_directory(path); }
bool VRGuiFile::isFile(string path) {
    bool b = boost::filesystem::is_regular_file( boost::filesystem::path(path) );
    cout << "isFile " << path << " : " << b << endl;
    return b;
}

class directory {
    path p_;
    public:
        inline directory(path p):p_(p) {;}
        boost::filesystem::directory_iterator begin() { return boost::filesystem::directory_iterator(p_); }
        boost::filesystem::directory_iterator end() { return boost::filesystem::directory_iterator(); }
};

vector<string> VRGuiFile::listDir(string dir) {
    vector<string> res;

    if (!exists(dir)) return res;
    if (!isDir(dir)) return res;

    namespace fs = boost::filesystem;
    fs::path path(dir);

    for( auto f : directory(path) ) {
        string fpath = f.path().filename().string();
        //if ( !isFile(dir+"/"+fpath) ) continue; //does not work???
        res.push_back(fpath);
    }

    return res;
}


