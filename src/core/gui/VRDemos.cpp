#include "VRDemos.h"

#include <gtkmm/table.h>
#include <gtkmm/stock.h>
#include <gtkmm/settings.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/paned.h>
#include <gtkmm/liststore.h>
#include <gtkmm/cellrenderertoggle.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/builder.h>
#include <gtkmm/frame.h>
#include <gtkmm/eventbox.h>
#include <string>
#include <iostream>
#include <boost/filesystem.hpp>

#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/devices/VRSignal.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "VRGuiContextMenu.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

// TODO:
// rename && delete scenes
// switch to a liststore || something!

class scenes_columns : public Gtk::TreeModelColumnRecord {
    public:
        scenes_columns() { add(name); add(favs); add(obj); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<bool> favs;
        Gtk::TreeModelColumn<gpointer> obj;
};

VRDemos::VRDemos() {
    initMenu();

    for (string path : VRSceneManager::get()->getExamplePaths() ) {
        demos[path] = new demoEntry();
        demos[path]->path = path;
        demos[path]->pxm_path = path.substr(0,path.size()-4) + ".png";
        demos[path]->write_protected = true;
        demos[path]->favorite = false;
        demos[path]->table = "examples_tab";
    }

    for (auto d : demos) setButton(d.second);
    updateTable("examples_tab");

    vector<string> favorites = VRSceneManager::get()->getFavoritePaths();
    for (auto f : favorites) addEntry(f, "favorites_tab", false);
    if (favorites.size() == 0) setNotebookPage("notebook2", 1);

    auto fkt = new VRDevCb("GUI_updateDemos", boost::bind(&VRDemos::update, this) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );

    setToolButtonCallback("toolbutton1", sigc::mem_fun(*this, &VRDemos::on_new_clicked));
    setToolButtonCallback("toolbutton5", sigc::mem_fun(*this, &VRDemos::on_saveas_clicked));
    setToolButtonCallback("toolbutton21", sigc::mem_fun(*this, &VRDemos::on_load_clicked));
}

void VRDemos::updatePixmap(demoEntry* e, Gtk::Image* img, int w, int h) {
    if (e == 0) return;
    if (img == 0) return;
    if ( !boost::filesystem::exists( e->pxm_path ) ) return;
    Glib::RefPtr<Gdk::Pixbuf> pxb = Gdk::Pixbuf::create_from_file (e->pxm_path);
    img->set(pxb);
    img->set_size_request(w, h);
}

Gtk::Image* VRDemos::loadGTKIcon(Gtk::Image* img, string path, int w, int h) {
    if ( !boost::filesystem::exists( path ) ) {
        cout << "Warning (loadGTKIcon): " << path << " not found!" << endl;
        return img;
    }
    if (img == 0) img = Gtk::manage(new Gtk::Image());
    img->set(path);
    img->set_size_request(w, h);
    return img;
}

void VRDemos::setButton(demoEntry* e) {
    Gtk::Settings::get_default()->property_gtk_button_images() = true;

    string rpath = VRSceneManager::get()->getOriginalWorkdir();

    // prep icons
    e->imgPlay = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON));
    e->imgOpts = loadGTKIcon(0, rpath+"/ressources/gui/opts20.png", 20, 20);
    e->imgScene = loadGTKIcon(0, rpath+"/ressources/gui/default_scene.png", 100, 75);
    e->imgLock = loadGTKIcon(0, rpath+"/ressources/gui/lock20.png", 20, 20);
    e->imgUnlock = loadGTKIcon(0, rpath+"/ressources/gui/unlock20.png", 20, 20);

    // prep other widgets
    e->widget = Gtk::manage(new Gtk::Frame());
    Gtk::EventBox* ebox = Gtk::manage(new Gtk::EventBox());
    Gtk::HBox* hb = Gtk::manage(new Gtk::HBox(false, 0));
    Gtk::VBox* vb = Gtk::manage(new Gtk::VBox(false, 0));
    Gtk::VBox* vb2 = Gtk::manage(new Gtk::VBox(false, 0));
    e->label = Gtk::manage(new Gtk::Label(e->path, true));
    e->butPlay = Gtk::manage(new Gtk::Button());
    e->butOpts = Gtk::manage(new Gtk::Button());
    e->butLock = Gtk::manage(new Gtk::Button());
    e->label->set_alignment(0.5, 0.5);
    e->label->set_ellipsize(Pango::ELLIPSIZE_START);
    e->label->set_max_width_chars(20);
    e->butPlay->set_tooltip_text("Play/Stop");
    e->butOpts->set_tooltip_text("Options");
    e->butLock->set_tooltip_text("Write protection");
    e->label->set_tooltip_text(e->path);

    // build widget
    vb2->pack_start(*e->butOpts, false, false, 0);
    vb2->pack_end(*e->butLock, false, false, 0);
    hb->pack_start(*e->imgScene, false, false, 10);
    hb->pack_end(*e->butPlay, false, false, 5);
    hb->pack_end(*vb2, false, false, 5);
    vb->pack_start(*e->label, false, false, 5);
    vb->pack_end(*hb, false, false, 5);
    e->widget->add(*ebox);
    ebox->add(*vb);
    e->butPlay->add(*e->imgPlay);
    e->butOpts->add(*e->imgOpts);
    if (e->write_protected) e->butLock->add(*e->imgLock);
    else e->butLock->add(*e->imgUnlock);

    updatePixmap(e, e->imgScene, 100, 75);

    // events
    VRDevCb* fkt = new VRDevCb("GUI_addDemoEntry", boost::bind(&VRDemos::updatePixmap, this, e, e->imgScene, 100, 75) );
    VRGuiSignals::get()->getSignal("onSaveScene")->add(fkt);

    menu->connectWidget("DemoMenu", ebox);
    ebox->signal_event().connect( sigc::bind<demoEntry*>( sigc::mem_fun(*this, &VRDemos::on_any_event), e) );

    e->butPlay->signal_clicked().connect( sigc::bind<demoEntry*>( sigc::mem_fun(*this, &VRDemos::toggleDemo), e) );
    e->butOpts->signal_clicked().connect( sigc::bind<demoEntry*>( sigc::mem_fun(*this, &VRDemos::on_menu_advanced), e) );
    e->butLock->signal_clicked().connect( sigc::bind<demoEntry*>( sigc::mem_fun(*this, &VRDemos::on_lock_toggle), e) );
    e->widget->show_all();
}

bool VRDemos::on_any_event(GdkEvent* event, demoEntry* entry) {
    if (event->type == GDK_BUTTON_PRESS) current_demo = entry;
    return false;
}

void VRDemos::on_lock_toggle(demoEntry* e) {
    e->write_protected = !e->write_protected;
    e->butLock->remove();
    if (e->write_protected) e->butLock->add(*e->imgLock);
    else e->butLock->add(*e->imgUnlock);
    e->butLock->show_all();

    if (VRSceneManager::getCurrent())
        VRSceneManager::getCurrent()->setFlag("write_protected", e->write_protected);
}

void VRDemos::updateTable(string t) {
    Gtk::Table* tab;
    VRGuiBuilder()->get_widget(t, tab);

    int x,y;

    Gtk::AttachOptions opts = Gtk::FILL|Gtk::EXPAND;
    Gtk::AttachOptions opts2 = Gtk::AttachOptions(0);

    int N = 4;
    for (auto d : demos) if(d.second->favorite) N++;
    tab->resize(N*0.5+1, 2);

    int i = 0;
    for (auto d : demos) {
        if (d.second->table != t) continue;
        if (d.second->widget == 0) continue;

        Gtk::Widget* w = d.second->widget;
        x = i%2;
        y = i/2;
        tab->attach( *w, x, x+1, y, y+1, opts, opts2, 10, 10);
        i++;
    }

    //Gtk::AttachOptions opts = Gtk::FILL|Gtk::EXPAND;
    //Gtk::Widget* w = Gtk::manage(new Gtk::Fixed());
    //tab->attach( *w, 0, 1, y, y+1, opts, opts, 0, 0);
    tab->show();
}

void VRDemos::clearTable(string t) {
    Gtk::Table* tab;
    VRGuiBuilder()->get_widget(t, tab);
    for (auto d : demos) {
        if (d.second->table != t) continue;

        Gtk::Widget* w = d.second->widget;
        if (w == 0) continue;
        tab->remove(*w);
    }
}

void VRDemos::setGuiState(demoEntry* e) {
    bool running = (e == 0) ? 0 : e->running;
    setVPanedSensitivity("vpaned1", running);
    setNotebookSensitivity("notebook3", running);

    for (auto i : demos) {
        demoEntry* d = i.second;
        if (d->widget) d->widget->set_sensitive(!running);
        if (d->imgPlay) d->imgPlay->set(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON);
        if (d != e) d->running = false;
    }

    if (e) if (e->widget) e->widget->set_sensitive(true);
    if (running) { if (e->imgPlay) e->imgPlay->set(Gtk::Stock::MEDIA_STOP, Gtk::ICON_SIZE_BUTTON); }
    else if (e) { if (e->imgPlay) e->imgPlay->set(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON); }
}

void VRDemos::addEntry(string path, string table, bool running) {
    if (demos.count(path)) return;

    clearTable("favorites_tab");

    demoEntry* e = new demoEntry();
    e->path = path;
    demos[path] = e;
    e->running = running;
    e->table = table;
    e->pxm_path = path.substr(0,path.size()-4)+".png";
    setButton(e);

    updateTable("favorites_tab");
    setGuiState(e);
    setNotebookPage("notebook2", 0);
}

void VRDemos::initMenu() {
    menu = new VRGuiContextMenu("DemoMenu");
    menu->appendItem("DemoMenu", "Unpin", sigc::mem_fun(*this, &VRDemos::on_menu_unpin));
    menu->appendItem("DemoMenu", "Delete", sigc::mem_fun(*this, &VRDemos::on_menu_delete));
    menu->appendItem("DemoMenu", "Advanced..", sigc::bind<demoEntry*>( sigc::mem_fun(*this, &VRDemos::on_menu_advanced), 0));

    setButtonCallback("button10", sigc::mem_fun(*this, &VRDemos::on_advanced_cancel));
    setButtonCallback("button26", sigc::mem_fun(*this, &VRDemos::on_advanced_start));
}

void VRDemos::on_menu_delete() {
    demoEntry* d = current_demo;
    if (d == 0) return;
    if (d->write_protected == true) return;

    string path = d->path;
    if (!askUser("Delete scene " + path + " (this will remove it completely from disk!)", "Are you sure you want to delete this scene?")) return;
    if (d->running) toggleDemo(d); // close demo if it is running

    clearTable("favorites_tab");
    demos.erase(path);
    remove(path.c_str());
    delete d;
    updateTable("favorites_tab");
    VRSceneManager::get()->remFavorite(path);
}

void VRDemos::on_menu_unpin() {
    demoEntry* d = current_demo;
    if (d == 0) return;
    if (d->write_protected == true) return;

    string path = d->path;
    if (!askUser("Forget about " + path + " ?", "")) return;
    if (d->running) toggleDemo(d); // close demo if it is running

    clearTable("favorites_tab");
    demos.erase(path);
    delete d;
    updateTable("favorites_tab");
    VRSceneManager::get()->remFavorite(path);
}

void VRDemos::on_menu_advanced(demoEntry* e) {
    if (e) current_demo = e;
    setCheckButton("checkbutton34", false);
    setCheckButton("checkbutton36", false);
    showDialog("advanced_start");
}

void VRDemos::on_advanced_cancel() {
    hideDialog("advanced_start");
}

void VRDemos::on_advanced_start() {
    bool no_scripts = getCheckButtonState("checkbutton34");
    bool lightweight = getCheckButtonState("checkbutton36");
    hideDialog("advanced_start");
    if (current_demo == 0) return;

    if (lightweight) VRSceneLoader::get()->ingoreHeavyRessources(); // just for the next scene

    if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    toggleDemo(current_demo); // start demo

    if (no_scripts) VRSceneManager::getCurrent()->disableAllScripts();
}

void VRDemos::on_diag_save_clicked() {
    string path = VRGuiFile::getRelativePath_toWorkdir();
    addEntry(path, "favorites_tab", false);
    VRSceneManager::get()->addFavorite(path);
    saveScene(path);
}

void VRDemos::on_saveas_clicked() {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRGuiFile::gotoPath( scene->getWorkdir() );
    VRGuiFile::setFile( scene->getFile() );
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRDemos::on_diag_save_clicked) );
    VRGuiFile::clearFilter();
    VRGuiFile::open( "Save", Gtk::FILE_CHOOSER_ACTION_SAVE, "Save project as.." );
}

void VRDemos::on_diag_load_clicked() {
    string path = VRGuiFile::getPath();
    if (current_demo) if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    addEntry(path, "favorites_tab", false);
    VRSceneManager::get()->addFavorite(path);
    toggleDemo(demos[path]);
}

void VRDemos::on_load_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRDemos::on_diag_load_clicked) );
    VRGuiFile::gotoPath( g_get_home_dir() );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Project", "*.xml");
    VRGuiFile::addFilter("All", "*");
    VRGuiFile::open( "Load", Gtk::FILE_CHOOSER_ACTION_OPEN, "Load project" );
}

void VRDemos::on_diag_new_clicked() {
    //string path = VRGuiFile::getRelativePath_toOrigin();
    string path = VRGuiFile::getPath();
    if (path == "") return;
    VRSceneManager::get()->newScene(path);
    addEntry(path, "favorites_tab", true);
    VRSceneManager::get()->addFavorite(path);
    saveScene(path);
}

void VRDemos::on_new_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRDemos::on_diag_new_clicked) );
    VRGuiFile::gotoPath( g_get_home_dir() );
    VRGuiFile::setFile( "myApp.xml" );
    VRGuiFile::clearFilter();
    VRGuiFile::open( "Create", Gtk::FILE_CHOOSER_ACTION_SAVE, "Create new project" );
}

void VRDemos::update() {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) {
        if (current_demo) {
            current_demo->running = false;
            setGuiState(current_demo);
        }
        return;
    }

    string sPath = scene->getPath();
    if (current_demo) {
        if (current_demo->path == sPath) {
            current_demo->running = true;
            setGuiState(current_demo);
            return;
        }
        current_demo->running = false;
        setGuiState(current_demo);
    }

    if (demos.count(sPath)) {
        current_demo = demos[sPath];
        current_demo->running = true;
        setGuiState(current_demo);
        return;
    }

    current_demo = new demoEntry();
    current_demo->running = true;
    demos[sPath] = current_demo;
    setGuiState(current_demo);
}

void VRDemos::toggleDemo(demoEntry* e) {
    bool run = !e->running;
    VRSceneManager::get()->removeScene(VRSceneManager::getCurrent());
    if (run) VRSceneManager::get()->loadScene(e->path, e->write_protected);
}

OSG_END_NAMESPACE;
