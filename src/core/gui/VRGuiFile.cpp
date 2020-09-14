#include "VRGuiFile.h"
#include "VRGuiUtils.h"

#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"

#include <gtk/gtkfilechooserdialog.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtktable.h>
#include <gtk/gtkfixed.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcelllayout.h>

#include <boost/filesystem.hpp>

GtkFileChooserDialog* VRGuiFile::dialog = 0;
GtkTable* VRGuiFile::addon = 0;
GtkTable* VRGuiFile::geoImportWidget = 0;
GtkTable* VRGuiFile::saveasWidget = 0;
function<void()> VRGuiFile::sigApply = function<void()>();
function<void()> VRGuiFile::sigClose = function<void()>();
function<void()> VRGuiFile::sigSelect = function<void()>();
bool VRGuiFile::cache_override = 0;
float VRGuiFile::scale = 1;
string VRGuiFile::preset = "SOLIDWORKS-VRML2";

typedef boost::filesystem::path path;

void VRGuiFile::init() {
    VRGuiFile::dialog = (GtkFileChooserDialog*)getGUIBuilder()->get_widget("file_dialog");
    setButtonCallback("button3", bind(&VRGuiFile::close));
    setButtonCallback("button9", bind(&VRGuiFile::apply));
    connect_signal<void>(dialog, bind(VRGuiFile::select), "selection_changed");
    connect_signal<void>(dialog, bind(VRGuiFile::apply), "file_activated");
    connect_signal<bool, GdkEvent*>(dialog, bind(VRGuiFile::keyApply, placeholders::_1), "event");
    gtk_file_chooser_set_action((GtkFileChooser*)dialog, GTK_FILE_CHOOSER_ACTION_OPEN);
    disableDestroyDiag("file_dialog");
}

void VRGuiFile::open(string button, int action, string title) {
    if (dialog == 0) init();
    setWidgetVisibility("file_dialog", true);

    setButtonText("button9", button);
    setButtonText("button3", "Cancel");

    gtk_window_set_title((GtkWindow*)dialog, title.c_str());
    gtk_file_chooser_set_action((GtkFileChooser*)dialog, GtkFileChooserAction(action));
}

void VRGuiFile::close() {
    //OSG::VRSetup::getCurrent()->pauseRendering(false);
    if (dialog == 0) init();
    setWidget(0);
    setWidgetVisibility("file_dialog", false);
    if (sigClose) sigClose();
    clearFilter();
}

void VRGuiFile::setWidget(GtkTable* table, bool expand, bool fill) {
    if (addon == table) return;
    auto vbox = getGUIBuilder()->get_widget("dialog-vbox1");

    // sub
    if (addon) {
        g_object_ref(addon); // increase ref count
        gtk_container_remove((GtkContainer*)vbox, (GtkWidget*)addon);
    }
    addon = table;
    if (table == 0) return;

    // add
    gtk_box_pack_start((GtkBox*)vbox, (GtkWidget*)table, expand, fill, 0);
    gtk_widget_show_all((GtkWidget*)vbox);
}

void VRGuiFile::on_toggle_cache_override(GtkCheckButton* b) {
    cache_override = gtk_toggle_button_get_active((GtkToggleButton*)b);
}

void VRGuiFile::on_edit_import_scale(GtkEntry* e) {
    scale = toFloat( gtk_entry_get_text(e) );
}

void VRGuiFile::on_change_preset(GtkComboBox* b) {
    char* name = gtk_combo_box_get_active_text(b);
    if (name == 0) return;
    preset = string(name);
}

void VRGuiFile::setGeoLoadWidget() {
    if (geoImportWidget == 0) {
        geoImportWidget = (GtkTable*)gtk_table_new(0,0,false);
        auto fixed = gtk_fixed_new();
        auto cache_override = gtk_check_button_new_with_label("ignore cache");
        auto label163 = gtk_label_new("Loader:");
        auto openFileWarning1 = gtk_label_new("Scale:");
        auto entry21 = gtk_entry_new();
        auto combobox15 = gtk_combo_box_new();
        auto cellrenderertext54 = gtk_cell_renderer_text_new();

        gtk_misc_set_alignment((GtkMisc*)openFileWarning1, 1, 0);

        gtk_box_pack_start((GtkBox*)combobox15, (GtkWidget*)cellrenderertext54, false, false, 0);
        gtk_cell_layout_add_attribute((GtkCellLayout*)combobox15, (GtkCellRenderer*)cellrenderertext54, "text", 0 );

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

        auto store = getGUIBuilder()->get_object("fileOpenPresets");
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

    gtk_file_chooser_add_filter((GtkFileChooser*)dialog, filter);
}

void VRGuiFile::clearFilter() {
    GSList* filters = gtk_file_chooser_list_filters(GTK_FILE_CHOOSER(dialog));
    for (GSList* elem = filters; elem != NULL; elem = g_slist_next(elem))
        gtk_file_chooser_remove_filter((GtkFileChooser*)dialog, (GtkFileFilter*)elem->data);
    g_slist_free (filters);
}

void VRGuiFile::setFile(string file) {
    if (dialog == 0) init();
    gtk_file_chooser_set_current_name((GtkFileChooser*)dialog, file.c_str());
}

void VRGuiFile::gotoPath(string path) {
    if (dialog == 0) init();
    gtk_file_chooser_set_current_folder((GtkFileChooser*)dialog, path.c_str());
}

void VRGuiFile::select() {
    if (sigSelect) sigSelect();
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
    gchar* filename = gtk_file_chooser_get_filename((GtkFileChooser*)dialog);
    string res = filename?filename:"";
    g_free(filename);
    return res;
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


