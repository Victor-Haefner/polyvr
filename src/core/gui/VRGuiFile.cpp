#include "VRGuiFile.h"
#include "VRGuiUtils.h"
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <boost/filesystem.hpp>

Gtk::FileChooserDialog* VRGuiFile::dialog = 0;
sigc::slot<void> VRGuiFile::sigApply = sigc::slot<void>();
sigc::slot<void> VRGuiFile::sigClose = sigc::slot<void>();

void VRGuiFile::init() {
    VRGuiBuilder()->get_widget("file_dialog", VRGuiFile::dialog);
    setButtonCallback("button3", sigc::ptr_fun(VRGuiFile::close));
    setButtonCallback("button9", sigc::ptr_fun(VRGuiFile::apply));
//<string, sigc::slot<void> >
    dialog->signal_key_release_event().connect( sigc::bind( sigc::ptr_fun(keySignalProxy), "Return", sigc::ptr_fun(VRGuiFile::apply) ) );
    dialog->set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
}

void VRGuiFile::open(bool folder, string b1, string b2) {
    if (dialog == 0) init();
    setLabel("openFileWarning", "");
    dialog->show();

    Gtk::Button *bt1, *bt2;
    VRGuiBuilder()->get_widget("button9", bt1);
    VRGuiBuilder()->get_widget("button3", bt2);
    bt1->set_label(b1);
    bt2->set_label(b2);
}

void VRGuiFile::addFilter(string name, string pattern) {
    if (dialog == 0) init();
    dialog->set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);

    Gtk::FileFilter* filter = new Gtk::FileFilter();
    filter->set_name(name);
    filter->add_pattern(pattern);

    dialog->add_filter(*filter);
}

void VRGuiFile::setFile(string file) {
    if (dialog == 0) init();
    dialog->set_action(Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog->set_current_name(file);
}

void VRGuiFile::gotoPath(string path) {
    if (dialog == 0) init();
    dialog->set_current_folder(path);
}

void VRGuiFile::apply() {
    if (dialog == 0) init();
    dialog->hide();
    sigApply();
}

void VRGuiFile::close() {
    if (dialog == 0) init();
    dialog->hide();
    sigClose();
    dialog->set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);

    for (auto f : dialog->list_filters()) {
        dialog->remove_filter(*f);
        delete &(*f);
    }
}

void VRGuiFile::setCallbacks(sigc::slot<void> sa, sigc::slot<void> sc) {
    sigApply = sa;
    sigClose = sc;
}

string VRGuiFile::getPath() {
    return string( dialog->get_filename() );
}

typedef boost::filesystem::path path;
namespace boost {
namespace filesystem {
template < >
path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt) {
    for( ; begin != end ; ++begin ) *this /= *begin;
    return *this;
}}}

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
        if( (*itrFrom) != "." )
            ret /= "..";
    }
    // Now navigate down the directory branch
    ret.append( itrTo, a_To.end() );
    return ret;
}

string VRGuiFile::getRelativePath() {
    string p = getPath();
    char cCurrentPath[FILENAME_MAX];
    getcwd(cCurrentPath, sizeof(cCurrentPath) );
    string workdir = string(cCurrentPath);

    path a(p), b(workdir);
    string rel = make_relative( b, a ).string();
    //cout << "relative path from " << p << " is " << rel << endl;
    return rel;
}
