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
    scanFolder("examples");
    updateTable("examples_tab");


    loadCfg();

    setToolButtonCallback("toolbutton1", sigc::mem_fun(*this, &VRDemos::on_new_clicked));
    setToolButtonCallback("toolbutton5", sigc::mem_fun(*this, &VRDemos::on_saveas_clicked));
    setToolButtonCallback("toolbutton21", sigc::mem_fun(*this, &VRDemos::on_load_clicked));
}

void VRDemos::scanFolder(string folder) {
    DIR* dir = opendir(folder.c_str());
    if (dir == NULL) { perror("Error: no local directory scene"); return; }

    struct dirent *entry;
    while ( (entry = readdir(dir)) ) {
        string file = string(entry->d_name);
        int N = file.size(); if (N < 6) continue;

        string path = folder+"/"+file;
        string name = file.substr(0,N-4);
        string ending = file.substr(N-4, N-1);
        if (ending != ".xml") continue;

        demos[path] = new demoEntry();
        demos[path]->path = path;
        demos[path]->pxm_path = folder+"/"+name+".png";
        demos[path]->write_protected = true;
    }

    for (auto d : demos) setButton(d.second);
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

    //tab->resize(0,0);
    int N = 4;
    for (auto d : demos) if(d.second->favorite) N++;
    tab->resize(N*0.5+1, 2);

    int i = 0;
    for (auto d : demos) {
        demoEntry* e = d.second;
        if(!e->favorite) continue;

        Gtk::Widget* w = e->button;
        if (w == 0) continue;
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
        demoEntry* e = d.second;
        if(!e->favorite) continue;

        Gtk::Widget* w = e->button;
        if (w == 0) continue;
        tab->remove(*w);
    }
}

void VRDemos::setGuiState(demoEntry* e) {
    setVPanedSensivity("vpaned1", e->running);
    setNotebookSensivity("notebook3", e->running);

    for (auto i : demos) {
        demoEntry* d = i.second;
        if (d->button) d->button->set_sensitive(!e->running);
        if (d->img) d->img->set(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON);
        if (d != e) d->running = false;
    }

    e->button->set_sensitive(true);
    if (e->running) e->img->set(Gtk::Stock::MEDIA_STOP, Gtk::ICON_SIZE_BUTTON);
    else e->img->set(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON);
}

void VRDemos::addEntry(string path, bool running) {
    if (demos.count(path)) return;

    clearTable("examples_tab");

    demoEntry* e = new demoEntry();
    e->path = path;
    demos[path] = e;
    e->running = running;
    e->pxm_path = path.substr(0,path.size()-4)+".png";
    setButton(e);

    updateTable("examples_tab");
    setGuiState(e);
}

void VRDemos::initMenu() {
    menu = new VRGuiContextMenu("DemoMenu");
    menu->appendItem("DemoMenu", "Delete", sigc::mem_fun(*this, &VRDemos::on_menu_delete));
    menu->appendItem("DemoMenu", "Advanced..", sigc::mem_fun(*this, &VRDemos::on_menu_advanced));

    setButtonCallback("button10", sigc::mem_fun(*this, &VRDemos::on_advanced_cancel));
    setButtonCallback("button26", sigc::mem_fun(*this, &VRDemos::on_advanced_start));
}

void VRDemos::on_menu_delete() {
    demoEntry* d = current_demo;
    if (d == 0) return;

    if (!askUser("Delete scene " + d->path + " (this will remove it completely from disk!)", "Are you sure you want to delete this scene?")) return;

    if (d->running) toggleDemo(d); // close demo if it is running

    clearTable("examples_tab");
    demos.erase(d->path);
    remove(d->path.c_str());
    delete d;
    updateTable("examples_tab");
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

    if (no_scripts) VRSceneManager::get()->getActiveScene()->disableAllScripts();
}

void VRDemos::saveCfg() {
    ofstream file("scene/.cfg");
    for (auto d : demos) file << d.second->path << " " << d.second->favorite << endl;
    file.close();
}

void VRDemos::loadCfg() {
    ifstream file("scene/.cfg");
    if (!file.is_open()) return;
    string line, path;
    bool fav;
    map<string, bool> cfg;
    while ( getline (file,line) ) {
        stringstream ss(line);
        ss >> path;
        ss >> fav;
        cfg[path] = fav;
    }
    file.close();

    for (auto d : demos ) {
        demoEntry* e = d.second;
        if (cfg.count(e->path)== 0 ) e->favorite = true;
        else e->favorite = cfg[e->path];
    }
}

void VRDemos::on_toggle_scene_fav(string path) { // TODO
    /*clearTable("favorites_tab");
    demoEntry* e = (demoEntry*)row.get_value(cols.obj);
    e->favorite = b;
    updateTable("favorites_tab");
    saveCfg();*/
}

void VRDemos::on_diag_save_clicked() {
    string path = VRGuiFile::getRelativePath();
    addEntry(path, true);
    saveScene(path);
}

void VRDemos::on_saveas_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRDemos::on_diag_save_clicked) );
    VRGuiFile::open(false, "Save");
}

void VRDemos::on_diag_load_clicked() {
    string path = VRGuiFile::getRelativePath();
    if (current_demo) if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    if (demos.count(path) == 0) addEntry(path, false);
    toggleDemo(demos[path]);
}

void VRDemos::on_load_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRDemos::on_diag_load_clicked) );
    VRGuiFile::open(false, "Load");
}

void VRDemos::on_diag_new_clicked() {
    string path = VRGuiFile::getRelativePath();
    if (path == "") return;

    // new scene
    VRSceneManager::get()->removeScene(VRSceneManager::get()->getActiveScene());
    VRSceneManager::get()->newScene(path);
    VRGuiSignals::get()->getSignal("scene_changed")->trigger();
    addEntry(path, true);
}

void VRDemos::on_new_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRDemos::on_diag_new_clicked) );
    VRGuiFile::open(false, "Create");
}

void VRDemos::toggleDemo(demoEntry* e) {
    for (auto d : demos) if (d.second->button) d.second->button->set_sensitive(e->running); // toggle all buttons
    setVPanedSensivity("vpaned1", !e->running);
    setNotebookSensivity("notebook3", !e->running);
    VRSceneManager::get()->removeScene(VRSceneManager::get()->getActiveScene());

    e->running = !e->running;
    if (e->running) {
        VRSceneLoader::get()->loadScene(e->path);
        VRSceneManager::get()->getActiveScene()->setFlag(SCENE_WRITE_PROTECTED, e->write_protected);
    }

    current_demo = e->running ? e : 0;
    setGuiState(e);
    VRGuiSignals::get()->getSignal("scene_changed")->trigger(); // update gui
}

OSG_END_NAMESPACE;
