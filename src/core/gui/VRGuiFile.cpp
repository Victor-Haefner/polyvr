#include "VRGuiFile.h"
#include "VRGuiUtils.h"
#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/table.h>
#include <gtkmm/builder.h>
#include <boost/filesystem.hpp>

#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"

Gtk::FileChooserDialog* VRGuiFile::dialog = 0;
Gtk::Table* VRGuiFile::addon = 0;
sigc::slot<void> VRGuiFile::sigApply = sigc::slot<void>();
sigc::slot<void> VRGuiFile::sigClose = sigc::slot<void>();
sigc::slot<void> VRGuiFile::sigSelect = sigc::slot<void>();

typedef boost::filesystem::path path;

void VRGuiFile::init() {
    VRGuiBuilder()->get_widget("file_dialog", VRGuiFile::dialog);
    setButtonCallback("button3", sigc::ptr_fun(VRGuiFile::close));
    setButtonCallback("button9", sigc::ptr_fun(VRGuiFile::apply));
    dialog->signal_selection_changed().connect( sigc::ptr_fun( VRGuiFile::select ));
    dialog->signal_file_activated().connect( sigc::ptr_fun(VRGuiFile::apply) );
    dialog->set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
}

void VRGuiFile::open(string button, int action, string title) {
    //OSG::VRSetupManager::getCurrent()->pauseRendering(true);

    if (dialog == 0) init();
    setLabel("openFileWarning", "");
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
    //OSG::VRSetupManager::getCurrent()->pauseRendering(false);
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
    vbox->pack_start(*table);
    vbox->show_all();
}

void VRGuiFile::addFilter(string name, string pattern) {
    if (dialog == 0) init();

    Gtk::FileFilter* filter = new Gtk::FileFilter();
    filter->set_name(name);
    filter->add_pattern(pattern);

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
