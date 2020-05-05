#include "VRAppPanel.h"
#include "VRAppLauncher.h"

#include "core/utils/system/VRSystem.h"
#include "core/scene/VRSceneManager.h"

#include <gtk/gtktable.h>

using namespace OSG;

VRAppPanel::VRAppPanel(string name) {
    setNameSpace("__system_apps__");
    setName(name);
}

VRAppPanel::~VRAppPanel() {}

VRAppPanelPtr VRAppPanel::create(string name) { return VRAppPanelPtr( new VRAppPanel(name) ); }
VRAppPanelPtr VRAppPanel::ptr() { return shared_from_this(); }

VRAppLauncherPtr VRAppPanel::addLauncher(string path, string timestamp, VRGuiContextMenu* menu, VRAppManager* mgr, bool write_protected, bool favorite, string table) {
    if (!exists(path)) return 0;
    if (apps.count(path)) return apps[path];
    auto app = VRAppLauncher::create(ptr());
    app->path = path;
    app->lastStarted = timestamp;
    string filename = getFileName(path);
    string foldername = getFolderName(path);
    app->pxm_path = foldername + "/.local_" + filename.substr(0,filename.size()-4) + "/snapshot.png";
    app->write_protected = write_protected;
    app->favorite = favorite;
    app->table = table;
    apps[path] = app;
    app->setup(menu, mgr);
    return app;
}

int VRAppPanel::getSize() { return apps.size(); }

void VRAppPanel::fillTable(string t, GtkTable* tab, int& i) {
    int x,y;
    GtkAttachOptions optsH = GtkAttachOptions(GTK_FILL|GTK_EXPAND);
    GtkAttachOptions optsV = GTK_SHRINK;
    //GtkAttachOptions optsV = GtkAttachOptions(0);

    for (auto d : apps) {
        if (d.second->table != t) continue;
        if (d.second->widget == 0) continue;

        GtkWidget* w = (GtkWidget*)d.second->widget;
        x = i%2;
        y = i/2;
        gtk_table_attach(tab, w, x, x+1, y, y+1, optsH, optsV, 10, 10);
        i++;
    }
}

void VRAppPanel::clearTable(string t, GtkTable* tab) {
    for (auto d : apps) {
        if (d.second->table != t) continue;

        GtkWidget* w = (GtkWidget*)d.second->widget;
        if (w == 0) continue;
        gtk_container_remove((GtkContainer*)tab, w);
    }
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

void VRAppPanel::remLauncher(string path) { apps.erase(path); }
VRAppLauncherPtr VRAppPanel::getLauncher(string path) { return apps.count(path) ? apps[path] : 0; }

map<string, VRAppLauncherPtr> VRAppPanel::getLaunchers() { return apps; }
