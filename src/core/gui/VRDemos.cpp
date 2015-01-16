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
#include <dirent.h>
#include <string>
#include <iostream>
#include <boost/filesystem.hpp>

#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRSceneManager.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

// TODO:
// rename and delete scenes
// switch to a liststore or something!

VRSignal* on_scene_loaded;
demoEntry* current_demo;

// override button released event handler to access right click events
class VRButton : public Gtk::Button {
    private:
        demoEntry* demo;

    public:
        VRButton(demoEntry* demo) : demo(demo) { ; }

        bool on_button_release_event(GdkEventButton* event) {
            if (event->button == 1) return Gtk::Button::on_button_release_event(event);
            current_demo = demo;
            return false;
        }
};

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

void VRDemos::updatePixmap(demoEntry* e, Gtk::Image* img_pxb) {
    if (e == 0) return;
    if (img_pxb == 0) return;
    //cout << "pxmap path " << e->pxm_path << endl;
    if ( !boost::filesystem::exists( e->pxm_path ) ) return;
    Glib::RefPtr<Gdk::Pixbuf> pxb = Gdk::Pixbuf::create_from_file (e->pxm_path);
    img_pxb->set(pxb);
}

void VRDemos::setButton(demoEntry* e) {
    Gtk::Settings::get_default()->property_gtk_button_images() = true;

    Gtk::Image* imgPlay = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON));

    Gtk::Label* lbl = Gtk::manage(new Gtk::Label(e->path, true));
    Gtk::HBox* hb = Gtk::manage(new Gtk::HBox(false, 0));
    Gtk::VBox* vb = Gtk::manage(new Gtk::VBox(false, 0));

    Glib::RefPtr<Gdk::Pixbuf> pxb;
    try { pxb = Gdk::Pixbuf::create_from_file ("ressources/gui/default_scene.png"); }
    catch(Glib::Error e) {;}
    Gtk::Image* img_pxb = Gtk::manage(new Gtk::Image(pxb));
    img_pxb->set_size_request(-1, 50);
    hb->pack_start(*img_pxb, true, true, 10);

    vb->pack_start(*lbl, false, false, 5);
    vb->pack_end(*hb, true, true, 5);
    hb->pack_end(*imgPlay, true, true, 5);

    lbl->set_alignment(0.5, 0.5);

    Gtk::Button* b = Gtk::manage(new VRButton(e));
    b->add(*vb);

    e->button = b;
    e->button_label = lbl;
    e->img = imgPlay;

    updatePixmap(e, img_pxb);
    VRDevCb* fkt;
    fkt = new VRDevCb("GUI_addDemoEntry", boost::bind(&VRDemos::updatePixmap, this, e, img_pxb) );
    VRGuiSignals::get()->getSignal("onSaveScene")->add(fkt);


    b->add_events((Gdk::EventMask)GDK_BUTTON_PRESS_MASK);
    b->add_events((Gdk::EventMask)GDK_BUTTON_RELEASE_MASK);
    menu->connectWidget("DemoMenu", b);
    b->signal_clicked().connect( sigc::bind<demoEntry*>( sigc::mem_fun(*this, &VRDemos::toggleDemo), e) );
    b->show_all();
}

void VRDemos::updateTable(string t) {
    Gtk::Table* tab;
    VRGuiBuilder()->get_widget(t, tab);

    int x,y;

    Gtk::AttachOptions opts = Gtk::FILL|Gtk::EXPAND;

    int N = 4;
    for (auto d : demos) if(d.second->favorite) N++;
    tab->resize(N*0.5+1, 2);

    int i = 0;
    for (auto d : demos) {
        if (d.second->table != t) continue;
        if (d.second->button == 0) continue;

        Gtk::Widget* w = d.second->button;
        x = i%2;
        y = i/2;
        tab->attach( *w, x, x+1, y, y+1, opts, opts, 10, 10);
        i++;
    }

    tab->show();
}

void VRDemos::clearTable(string t) {
    Gtk::Table* tab;
    VRGuiBuilder()->get_widget(t, tab);
    for (auto d : demos) {
        if (d.second->table != t) continue;

        Gtk::Widget* w = d.second->button;
        if (w == 0) continue;
        tab->remove(*w);
    }
}

void VRDemos::setGuiState(demoEntry* e) {
    bool running = (e == 0) ? 0 : e->running;
    setVPanedSensivity("vpaned1", running);
    setNotebookSensivity("notebook3", running);

    for (auto i : demos) {
        demoEntry* d = i.second;
        if (d->button) d->button->set_sensitive(!running);
        if (d->img) d->img->set(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON);
        if (d != e) d->running = false;
    }

    if (e) e->button->set_sensitive(true);
    if (running) e->img->set(Gtk::Stock::MEDIA_STOP, Gtk::ICON_SIZE_BUTTON);
    else if (e) e->img->set(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON);
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
    menu->appendItem("DemoMenu", "Advanced..", sigc::mem_fun(*this, &VRDemos::on_menu_advanced));

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

void VRDemos::on_menu_advanced() {
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

    if (lightweight) VRSceneLoader::get()->ingoreHeavyRessources(); // just for the next scene

    if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    toggleDemo(current_demo); // start demo

    if (no_scripts) VRSceneManager::getCurrent()->disableAllScripts();
}

void VRDemos::on_diag_save_clicked() {
    string path = VRGuiFile::getRelativePath_toWorkdir();
    if (demos.count(path) == 0) {
        addEntry(path, "favorites_tab", false);
        VRSceneManager::get()->addFavorite(path);
    }
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
    string path = VRGuiFile::getRelativePath_toWorkdir();
    if (current_demo) if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    if (demos.count(path) == 0) {
        addEntry(path, "favorites_tab", false);
        VRSceneManager::get()->addFavorite(path);
    }
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
    string path = VRGuiFile::getRelativePath_toWorkdir();
    if (path == "") return;
    VRSceneManager::get()->newScene(path);
    if (demos.count(path) == 0) {
        addEntry(path, "favorites_tab", true);
        VRSceneManager::get()->addFavorite(path);
    }
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
    if (scene == 0 and current_demo == 0) return;
    if (scene == 0 and current_demo != 0) {
        current_demo->running = false;
        setGuiState(current_demo);
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

    if (demos.count(sPath) == 0) return;
    current_demo = demos[sPath];
    current_demo->running = true;
    setGuiState(current_demo);
}

void VRDemos::toggleDemo(demoEntry* e) {
    bool run = !e->running;
    VRSceneManager::get()->removeScene(VRSceneManager::getCurrent());
    if (run) VRSceneManager::get()->loadScene(e->path, e->write_protected);
}

OSG_END_NAMESPACE;
