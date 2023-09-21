#include "VRAppPanel.h"
#include "VRAppLauncher.h"

#include "core/gui/VRGuiManager.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/toString.h"
#include "core/scene/VRSceneManager.h"

#define signal(m,...) VRGuiManager::trigger(m,##__VA_ARGS__)

using namespace OSG;

VRAppPanel::VRAppPanel(string name) {
    signal("newAppPanel", {{"name",name}});
    setNameSpace("__system_apps__");
    setName(name);
}

VRAppPanel::~VRAppPanel() {}

VRAppPanelPtr VRAppPanel::create(string name) { return VRAppPanelPtr( new VRAppPanel(name) ); }
VRAppPanelPtr VRAppPanel::ptr() { return shared_from_this(); }

VRAppLauncherPtr VRAppPanel::addLauncher(string path, string timestamp, VRAppManager* mgr, bool write_protected, bool favorite, string table) {
    if (!exists(path)) return 0;
    if (auto l = getLauncher(path)) return l;
    auto app = VRAppLauncher::create(ptr(), path, timestamp);
    string filename = getFileName(path);
    string foldername = getFolderName(path);
    app->pxm_path = foldername + "/.local_" + filename.substr(0,filename.size()-4) + "/snapshot.png";
    app->write_protected = write_protected;
    app->favorite = favorite;
    app->table = table;
    apps[path] = app;
    app->setup(mgr);
    return app;
}

int VRAppPanel::getSize() { return apps.size(); }

void VRAppPanel::fillTable(string t, int& i) {
    for (auto d : apps) {
        if (d.second->table != t) continue;
        int x = i%2;
        int y = i/2;
        signal("addAppLauncher", {{"name",name},{"ID",d.second->ID},{"table",t},{"position",toString(x)+":"+toString(y)}});
        //gtk_grid_attach(GTK_GRID(table), w, x, y, 1, 1);
        i++;
    }
}

void VRAppPanel::clearTable(string t) {
    signal("clearAppPanel", {{"name",name}});
}

void VRAppPanel::setGuiState(VRAppLauncherPtr e, bool running, bool noLauncherScene) {
    for (auto i : apps) {
        if (noLauncherScene) i.second->setState(1); // disable all launchers
        else if (running) {
            if (i.second == e) i.second->setState(2); // running scene
            else               i.second->setState(1); // disable other scenes
        } else i.second->setState(0); // all launchers standby
    }
}

void VRAppPanel::remLauncher(string path) {
    if (auto l = getLauncher(path)) apps.erase(l->path);
}

VRAppLauncherPtr VRAppPanel::getLauncher(string path) {
    for (auto a : apps) {
        if (isSamePath(a.first, path)) return a.second;
    }
    return 0;
}

map<string, VRAppLauncherPtr> VRAppPanel::getLaunchers() { return apps; }
