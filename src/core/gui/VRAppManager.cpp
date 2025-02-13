#include <OpenSG/OSGRenderAction.h>

#include "VRAppManager.h"
#include "widgets/VRAppPanel.h"
#include "widgets/VRAppLauncher.h"

#include <string>
#include <iostream>

#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/scene/import/VRImport.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/scene/VRProjectsList.h"
#include "core/setup/devices/VRSignal.h"
#include "core/utils/system/VRSystem.h"
#include "core/gui/VRGuiManager.h"

#include "VRGuiSignals.h"
#include "widgets/VRAppLauncher.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRAppManager::VRAppManager() {
    //cout << " - - - VRAppManager" << endl;
    auto examplesSection = addSection("examples");
    auto favoritesSection = addSection("favorites");
    auto recentsSection = addSection("recents");

    auto examples = VRSceneManager::get()->getExamplePaths();
    for (auto p : examples->getPaths() ) {
        //cout << "   - - - examples path " << p << endl;
        examplesSection->addLauncher(p, "", this, true, false, "examples_tab");
    }
    updateTable("examples_tab");

    auto favorites = VRSceneManager::get()->getFavoritePaths();

    int i=0;
    for (auto p : favorites->getEntriesByTimestamp()) {
        long tsi = p->getTimestamp();
        time_t ts = tsi;
        string t = "";
        if (ts > 0) {
            //t = asctime( localtime(&ts) );
            t = toString( tsi );
        }
        addEntry(p->getPath(), "favorites_tab", false, t, i<2);
        i++;
    }

    //if (favorites->size() == 0) setNotebookPage("notebook2", 1);

    updateCb = VRFunction<VRDeviceWeakPtr, bool>::create("GUI_updateDemos", bind(&VRAppManager::update, this) );

    auto mgr = VRGuiSignals::get();
    mgr->getSignal("scene_changed")->add( updateCb );
    mgr->addCallback("ui_new_file", [&](OSG::VRGuiSignals::Options o) { on_diag_new_clicked(o["fileName"]); return true; }, true );
    mgr->addCallback("ui_open_file", [&](OSG::VRGuiSignals::Options o) { on_diag_load_clicked(o["fileName"]); return true; } );
    mgr->addCallback("ui_saveas_file", [&](OSG::VRGuiSignals::Options o) { on_diag_save_clicked(o["fileName"]); return true; } );
    mgr->addCallback("toolbar_close", [&](OSG::VRGuiSignals::Options o) { on_stop_clicked(); return true; } );

    //menu->appendItem("DemoMenu", "Unpin", bind(&VRAppManager::on_launcher_unpin, this));
    //menu->appendItem("DemoMenu", "Delete", bind(&VRAppManager::on_launcher_delete, this));
    //menu->appendItem("DemoMenu", "Advanced..", bind(&VRAppManager::on_menu_advanced, this, VRAppLauncherPtr(0)));
    //setEntryCallback("appSearch", bind(&VRAppManager::on_search, this), true); // app search
}

VRAppManager::~VRAppManager() {}

VRAppManagerPtr VRAppManager::create() { return VRAppManagerPtr( new VRAppManager() ); }

VRAppPanelPtr VRAppManager::addSection(string name) {
    auto s = VRAppPanel::create(name);
    sections[name] = s;
    return s;
}

void VRAppManager::setCurrentApp(VRAppLauncherPtr e) {
    current_demo = e;
}

void VRAppManager::on_lock_toggle(VRAppLauncherPtr e) {
    e->toggle_lock();
}

void VRAppManager::updateTable(string t) {
#if GTK_MAJOR_VERSION == 2
    int N = 4;
    if (t == "examples_tab") N += sections["examples"]->getSize();
    if (t == "favorites_tab") N += sections["recents"]->getSize() + sections["favorites"]->getSize();
    gtk_table_resize(tables[t], N*0.5+1, 2);
#endif

    int i = 0;
    if (t == "examples_tab") sections["examples"]->fillTable(t, i);
    if (t == "favorites_tab") {
        sections["recents"]->fillTable(t, i);
        sections["favorites"]->fillTable(t, i);
    }
}

void VRAppManager::clearTable(string t) {
    if (t == "examples_tab") sections["examples"]->clearTable(t);
    if (t == "favorites_tab") {
        sections["recents"]->clearTable(t);
        sections["favorites"]->clearTable(t);
    }
}

void VRAppManager::setGuiState(VRAppLauncherPtr e) {
    bool running = (e == 0) ? noLauncherScene : e->running;
    //setWidgetSensitivity("vpaned1", running);
    //setWidgetSensitivity("notebook3", running);

    for (auto section : sections) section.second->setGuiState(e, running, noLauncherScene);

    if (e) {
        if (running) e->setState(2);
        else e->setState(0);
    }

    //setWidgetSensitivity("toolbutton4", running); // toggle 'save' button availability
    //setWidgetSensitivity("toolbutton5", running); // toggle 'save as' button availability
    //setWidgetSensitivity("toolbutton50", running); // toggle 'web export' button availability
    //setWidgetSensitivity("toolbutton28", running); // toggle 'stop' button availability
}

VRAppLauncherPtr VRAppManager::addEntry(string path, string table, bool running, string timestamp, bool recent) {
    if (!exists(path)) return 0;
    clearTable(table);

    VRAppLauncherPtr e = 0;
    if (table == "examples_tab") e = sections["examples"]->addLauncher(path, timestamp, this, true, false, table);
    if (table == "favorites_tab" &&  recent) e = sections["recents"]->addLauncher(path, timestamp, this, false, true, table);
    if (table == "favorites_tab" && !recent) e = sections["favorites"]->addLauncher(path, timestamp, this, false, true, table);
    if (!e) return 0;

    e->running = running;

    updateTable(table);
    setGuiState(e);
    //setNotebookPage("notebook2", 0);
    return e;
}

void VRAppManager::on_launcher_delete() {
    VRAppLauncherPtr d = current_demo;
    if (!d) return;
    if (d->write_protected == true) return;
    string table = d->table;

    string path = d->path;
    //if (!askUser("Delete scene " + path + " (this will remove it completely from disk!)", "Are you sure you want to delete this scene?")) return;
    if (d->running) toggleDemo(d); // close demo if it is running

    clearTable(table);
    if (table == "examples_tab") sections["examples"]->remLauncher(path);
    if (table == "recents_tab") sections["recents"]->remLauncher(path);
    if (table == "favorites_tab") sections["favorites"]->remLauncher(path);

    current_demo.reset();
    remove(path.c_str());
    updateTable(table);
    VRSceneManager::get()->remFavorite(path);
}

void VRAppManager::on_launcher_unpin() {
    VRAppLauncherPtr d = current_demo;
    if (!d) return;
    if (d->write_protected == true) return;
    string table = d->table;

    string path = d->path;
    //if (!askUser("Forget about " + path + " ?", "")) return;
    if (d->running) toggleDemo(d); // close demo if it is running

    clearTable(table);
    if (table == "examples_tab") sections["examples"]->remLauncher(path);
    if (table == "recents_tab") sections["recents"]->remLauncher(path);
    if (table == "favorites_tab") sections["favorites"]->remLauncher(path);

    current_demo.reset();
    updateTable(table);
    VRSceneManager::get()->remFavorite(path);
}

void VRAppManager::on_advanced_start() {
    /*bool no_scripts = getCheckButtonState("checkbutton34");
    bool lightweight = getCheckButtonState("checkbutton36");
    hideDialog("advanced_start");
    if (current_demo == 0) return;

    if (lightweight) VRImport::get()->ingoreHeavyRessources(); // just for the next scene

    if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    toggleDemo(current_demo); // start demo

    auto scene = VRScene::getCurrent();
    if (no_scripts && scene) scene->pauseScripts(true);*/
}

void VRAppManager::normFileName(string& path) {
    string e = path.substr(path.size()-4, path.size());
    if (e == ".xml" || e == ".pvr") return;
    path += ".pvr";
}

string encryptionKey;
void VRAppManager::on_toggle_encryption(bool b) {
    bool doEncryption = b;
    encryptionKey = "";
    if (!doEncryption) return;
    //encryptionKey = askUserPass("Please enter an encryption key");
}

void VRAppManager::on_diag_save_clicked(string path) { // TODO: check if ending is .pvr
    encryptionKey = "";
    saveScene(path, true, encryptionKey);
    VRSceneManager::get()->addFavorite(path);
    addEntry(path, "favorites_tab", true);
}

void VRAppManager::toggleDemo(VRAppLauncherPtr e, bool pausedScripts) {
    bool run = !e->running;
    VRSceneManager::get()->closeScene();
    if (run) {
        string encryptionKey;
        //if (endsWith(e->path, ".pvc")) encryptionKey = askUserPass("Please insert encryption key");
        VRSceneManager::get()->loadScene(e->path, e->write_protected, encryptionKey);

        if (pausedScripts) {
            auto scene = VRScene::getCurrent();
            if (scene) scene->pauseScripts(true);
        }
    }
}

void VRAppManager::on_stop_clicked() {
    if (noLauncherScene) {
        VRSceneManager::get()->closeScene();
        noLauncherScene = false;
        update();
    }

    if (current_demo && current_demo->running) toggleDemo(current_demo); // close demo if it is running
}

void VRAppManager::on_diag_load_clicked(string path) {
    if (!exists(path)) return;
    if (current_demo) if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    bool loadProject = bool( endsWith(path, ".xml") || endsWith(path, ".pvr") || endsWith(path, ".pvc") );

    if (loadProject) {
        cout << "load project '" << path << "'" << endl;
        auto e = addEntry(path, "favorites_tab", false);
        VRSceneManager::get()->addFavorite(path);
        if (e) toggleDemo(e);
    } else {
        cout << "load model '" << path << "'" << endl;
        VRSceneManager::get()->setWorkdir( getFolderName(path) );
        VRSceneManager::get()->newScene("PolyVR Model Viewer");

        noLauncherScene = true;
        update();

        auto scene = VRScene::getCurrent();
        if (!scene) return;

        scene->setBackground(VRBackground::SKY);

        auto cam = dynamic_pointer_cast<VRCamera>(scene->get("Default"));
        auto light = scene->get("light");
        auto model = VRImport::get()->load(path, light);

        auto bb = model->getBoundingbox();
        if (bb->volume() < 1e-4) return;
        double h = bb->center()[1];
        Vec3d p = Vec3d(0,h,0);
        cam->setTransform(p, Vec3d(0,0,-1), Vec3d(0,1,0));
        cam->focusObject(model);
    }
}

void VRAppManager::writeGitignore(string path) {
    ofstream f(path);
    f << ".local_*" << endl;
    f << "core" << endl;
    f << "*.blend1" << endl;
    f << "*~" << endl;
    f << ".treeLods" << endl;
    f << "blob_storage" << endl;
    f << "GPUCache" << endl;
    f << "imgui.ini" << endl;
    f << ".*.osb" << endl;
}

void VRAppManager::on_diag_new_clicked(string path) {
    if (path == "") return;
    normFileName(path);
    VRSceneManager::get()->newScene(path);
    string gitIgnorePath = getFolderName(path) + "/.gitignore";
    if (!exists(gitIgnorePath)) writeGitignore(gitIgnorePath);
    saveScene(path);
    string ts = toString(time(0));
    addEntry(path, "favorites_tab", true, ts);
    VRSceneManager::get()->addFavorite(path, ts);
}

void VRAppManager::on_search() {
    /*string s = getTextEntry("appSearch");
    for (auto section : sections) {
        for (auto launcher : section.second->getLaunchers()) {
            if (s == "") { launcher.second->show(); continue; }
            string name = launcher.first;
            if (contains(name, s, false)) launcher.second->show(); // TODO: extend contains with a case sensitive flag
            else launcher.second->hide();
        }
    }*/
}

VRAppLauncherPtr VRAppManager::getEntry(string sPath) {
    auto e = sections["recents"]->getLauncher(sPath);
    if (!e) e = sections["examples"]->getLauncher(sPath);
    if (!e) e = sections["favorites"]->getLauncher(sPath);
    return e;
}

void VRAppManager::setCurrentGuiState(bool b) {
    if (current_demo) current_demo->running = b;
    setGuiState(current_demo);
    if (!b) current_demo = 0;
}

bool VRAppManager::update() {
	cout << "VRAppManager::update " << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) {
        cout << " .. no scene" << endl;
        setCurrentGuiState(false);
        return true;
    }

    string sPath = scene->getPath();

    if (current_demo) {
        cout << " .. current_demo is set" << endl;
        if (isSamePath(current_demo->path, sPath)) {
            cout << "  .. to running, set ui state accordingly" << endl;
            setCurrentGuiState(true);
        } else {
            cout << "  .. to not running, set ui state accordingly" << endl;
            setCurrentGuiState(false);
        }
        return true;
    }

    if (noLauncherScene) {
        cout << " .. noLauncherScene set, set ui state accordingly" << endl;
        setGuiState(0);
        return true;
    }

    if ( auto e = getEntry(sPath) ) {
        cout << " .. found launcher, set to current and to running, set ui state accordingly" << endl;
        current_demo = e;
    } else {
        cout << " .. create launcher, set to current and to running, set ui state accordingly" << endl;
        current_demo = addEntry(sPath, "favorites_tab", true);
    }
    setCurrentGuiState(true);
    return true;
}

OSG_END_NAMESPACE;


