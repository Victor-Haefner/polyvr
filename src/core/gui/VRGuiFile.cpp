#include "VRGuiFile.h"
#include "VRGuiUtils.h"
#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/builder.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/combobox.h>
#include <gtkmm/cellrenderertext.h>
#include <gtkmm/fixed.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodelcolumn.h>
#include <boost/filesystem.hpp>

#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"

Gtk::FileChooserDialog* VRGuiFile::dialog = 0;
Gtk::Table* VRGuiFile::addon = 0;
Gtk::Table* VRGuiFile::geoImportWidget = 0;
Gtk::Table* VRGuiFile::saveasWidget = 0;
sigc::slot<void> VRGuiFile::sigApply = sigc::slot<void>();
sigc::slot<void> VRGuiFile::sigClose = sigc::slot<void>();
sigc::slot<void> VRGuiFile::sigSelect = sigc::slot<void>();
bool VRGuiFile::cache_override = 0;
float VRGuiFile::scale = 1;
string VRGuiFile::preset = "SOLIDWORKS-VRML2";

typedef boost::filesystem::path path;

void VRGuiFile::init() {
    VRGuiBuilder()->get_widget("file_dialog", VRGuiFile::dialog);
    setButtonCallback("button3", sigc::ptr_fun(VRGuiFile::close));
    setButtonCallback("button9", sigc::ptr_fun(VRGuiFile::apply));
    dialog->signal_selection_changed().connect( sigc::ptr_fun( VRGuiFile::select ));
    dialog->signal_file_activated().connect( sigc::ptr_fun(VRGuiFile::apply) );
    dialog->signal_event().connect( sigc::ptr_fun(VRGuiFile::keyApply) );
    dialog->set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
}

void VRGuiFile::open(string button, int action, string title) {
    //OSG::VRSetup::getCurrent()->pauseRendering(true);

    if (dialog == 0) init();
    dialog->show();

    Gtk::Button *bt1, *bt2;
    VRGuiBuilder()->get_widget("button9", bt1);
    VRGuiBuilder()->get_widget("button3", bt2);
    bt1->set_label(button);
    bt2->set_label("Cancel");

    dialog->set_title(title);
    dialog->set_action( Gtk::FileChooserAction(action) );
}

void VRGuiFile::close() {
    //OSG::VRSetup::getCurrent()->pauseRendering(false);
    if (dialog == 0) init();
    setWidget(0);
    dialog->hide();
    sigClose();

    for (auto f : dialog->list_filters()) {
        dialog->remove_filter(*f);
        delete &(*f);
    }
}

void VRGuiFile::setWidget(Gtk::Table* table) {
    if (addon == table) return;
    Gtk::VBox* vbox;
    VRGuiBuilder()->get_widget("dialog-vbox1", vbox);

    // sub
    if (addon) vbox->remove(*addon);
    addon = table;
    if (table == 0) return;

    // add
    vbox->pack_start(*table, Gtk::PACK_SHRINK);
    vbox->show_all();
}

void VRGuiFile::on_toggle_cache_override(Gtk::CheckButton* b) {
    cache_override = b->get_active();
}

void VRGuiFile::on_edit_import_scale(Gtk::Entry* e) {
    scale = toFloat( e->get_text() );
}

void VRGuiFile::on_change_preset(Gtk::ComboBox* b) {
    char* name = gtk_combo_box_get_active_text(b->gobj());
    if (name == 0) return;
    preset = string(name);
}

void VRGuiFile::setGeoLoadWidget() {
    if (geoImportWidget == 0) {
        geoImportWidget = manage( new Gtk::Table() );
        auto fixed = manage( new Gtk::Fixed() );
        auto cache_override = manage( new Gtk::CheckButton("ignore cache") );
        auto label163 = manage( new Gtk::Label("label") );
        auto openFileWarning1 = manage( new Gtk::Label("scale:", Gtk::ALIGN_RIGHT) );
        auto entry21 = manage( new Gtk::Entry() );
        auto combobox15 = manage( new Gtk::ComboBox() );
        auto cellrenderertext54 = manage( new Gtk::CellRendererText() );

        combobox15->pack_start( *cellrenderertext54 );
        combobox15->add_attribute( *cellrenderertext54, "text", 0 );

        Gtk::AttachOptions opts = Gtk::FILL|Gtk::EXPAND;
        Gtk::AttachOptions opts3 = Gtk::FILL;
        Gtk::AttachOptions opts2 = Gtk::AttachOptions(0);

        geoImportWidget->attach(*fixed, 0,3,0,1, opts, opts2);
        geoImportWidget->attach(*cache_override, 3,4,0,1, opts3, opts2);
        geoImportWidget->attach(*label163, 0,1,1,2, opts, opts2);
        geoImportWidget->attach(*combobox15, 1,2,1,2, opts, opts2);
        geoImportWidget->attach(*openFileWarning1, 2,3,1,2, opts, opts2, 10);
        geoImportWidget->attach(*entry21, 3,4,1,2, opts3, opts2);

        entry21->signal_activate().connect( sigc::bind(&VRGuiFile::on_edit_import_scale, entry21) );
        cache_override->signal_toggled().connect( sigc::bind(&VRGuiFile::on_toggle_cache_override, cache_override) );
        combobox15->signal_changed().connect( sigc::bind(&VRGuiFile::on_change_preset, combobox15) );

        Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("fileOpenPresets"));
        vector<string> presets = { "SOLIDWORKS-VRML2", "OSG", "COLLADA" };
        fillStringListstore("fileOpenPresets", presets);
        combobox15->set_model( store );
        combobox15->set_active( getListStorePos("fileOpenPresets", "SOLIDWORKS-VRML2") );
    }

    setWidget(geoImportWidget);
}

void VRGuiFile::setSaveasWidget( sigc::slot<void, Gtk::CheckButton*> sig ) {
    if (saveasWidget == 0) {
        saveasWidget = manage( new Gtk::Table() );
        auto fixed = manage( new Gtk::Fixed() );
        auto encrypt = manage( new Gtk::CheckButton("encrypt") );

        Gtk::AttachOptions opts = Gtk::FILL|Gtk::EXPAND;
        Gtk::AttachOptions opts3 = Gtk::FILL;
        Gtk::AttachOptions opts2 = Gtk::AttachOptions(0);

        saveasWidget->attach(*fixed, 0,3,0,1, opts, opts2);
        saveasWidget->attach(*encrypt, 3,4,0,1, opts3, opts2);
        encrypt->signal_toggled().connect( sigc::bind( sig, encrypt) );
    }

    setWidget(saveasWidget);
}


void VRGuiFile::addFilter(string name, int N, ...) {
    if (dialog == 0) init();

    Gtk::FileFilter* filter = new Gtk::FileFilter();
    filter->set_name(name);

    va_list ap;
    va_start(ap, N);
    for (int i=0; i<N; i++) {
        string pattern = string( va_arg(ap, const char*) );
        filter->add_pattern(pattern);
    }
    va_end(ap);

    dialog->add_filter(*filter);
}

void VRGuiFile::clearFilter() {
    if (dialog == 0) return;
    for (auto f : dialog->list_filters()) {
        dialog->remove_filter(*f);
        delete f;
    }
}

void VRGuiFile::setFile(string file) {
    if (dialog == 0) init();
    dialog->set_current_name(file);
}

void VRGuiFile::gotoPath(string path) {
    if (dialog == 0) init();
    dialog->set_current_folder(path);
}

void VRGuiFile::select() {
    sigSelect();
}

void VRGuiFile::apply() {
    if (dialog == 0) init();
    dialog->hide();
    sigApply();
    setWidget(0);
}

bool VRGuiFile::keyApply(GdkEvent* k) {
    if (k->key.keyval != 65293 || k->key.type != GDK_KEY_RELEASE) return false;
    apply();
    return true;
}

void VRGuiFile::setCallbacks(sig sa, sig sc, sig ss) {
    sigApply = sa;
    sigClose = sc;
    sigSelect = ss;
}

string VRGuiFile::getPath() {
    return string( dialog->get_filename() );
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


