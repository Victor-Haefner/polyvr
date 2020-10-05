#include "VRGuiManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/scripting/VRScript.h"
#include "VRGuiUtils.h"
#include "VRDemos.h"
#include "VRGuiScene.h"
#include "VRGuiBits.h"
#include "VRGuiNav.h"
#include "VRGuiSignals.h"
#include "VRGuiScripts.h"
#include "VRGuiSetup.h"
#include "VRGuiGeneral.h"
#include "VRGuiMonitor.h"
#include "VRGuiSemantics.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRFunction.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRSignalT.h"

#include <gtk/gtk.h>
#if GTK_MAJOR_VERSION == 2
#include <gtk/gtkglinit.h>
#else
#include "core/gui/gtkglext/gtk/gtkglinit.h"
#endif

#include <boost/thread/recursive_mutex.hpp>

typedef boost::recursive_mutex::scoped_lock PLock;

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGuiScene* g_scene;
VRGuiBits* g_bits;
VRAppManager* g_demos;
VRGuiNav* g_nav;
VRGuiScripts* g_sc;
VRGuiSetup* g_di;
VRGuiSemantics* g_sem;
VRGuiGeneral* g_gen;
VRGuiMonitor* g_mon;

void addIconsPath(string p) {
    string icons = VRSceneManager::get()->getOriginalWorkdir() + "/" + p;
    gtk_icon_theme_add_resource_path(gtk_icon_theme_get_default(), icons.c_str());
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), icons.c_str());
}

void addSchemaPath(string p) {
    string schemas = VRSceneManager::get()->getOriginalWorkdir() + "/" + p;
    // TODO;
}

VRGuiManager::VRGuiManager() {
    cout << "Init VRGuiManager..";
    mtx = new boost::recursive_mutex();
    standalone = VROptions::get()->getOption<bool>("standalone");

    int argc = 0;
    gtk_disable_setlocale();
    gtk_init(&argc, 0);

    addIconsPath("ressources/gui/icons");
    addSchemaPath("ressources/gui/schemas");

    //gtk_gl_init(&argc, NULL);
    getGUIBuilder(standalone);

    if (standalone) {
        cout << " start in standalone mode\n";
        VRSetupManager::get()->load("Desktop", "setup/Desktop.xml");

        updatePtr = VRUpdateCb::create("GUI_updateManager", bind(&VRGuiManager::update, this) );
        VRSceneManager::get()->addUpdateFkt(updatePtr, 1);

        GtkWindow* top = (GtkWindow*)getGUIBuilder()->get_widget("window1");
        gtk_window_maximize(top);
        gtk_widget_show_all((GtkWidget*)top);
        return;
    }    

    //gtk_rc_parse("gui/gtkrc");
    g_demos = new VRAppManager();
    g_scene = new VRGuiScene();
    g_bits = new VRGuiBits();
    g_nav = new VRGuiNav();
    g_sc = new VRGuiScripts();
    g_sem = new VRGuiSemantics();
    g_di = new VRGuiSetup();
    g_gen = new VRGuiGeneral();
    g_mon = new VRGuiMonitor();
    g_scene->updateTreeView();

    auto editor = g_sc->getEditor();
    editor->addKeyBinding("wipe", VRUpdateCb::create("wipeCb", bind(&VRGuiBits::wipeConsoles, g_bits)));

    VRDeviceCbPtr fkt;
    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateSceneViewer", bind(&VRGuiScene::updateTreeView, g_scene) );
    VRGuiSignals::get()->getSignal("scene_modified")->add( fkt );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("camera_changed")->add(fkt);
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateBits", bind(&VRGuiBits::update, g_bits) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("camera_added")->add( fkt );
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateNav", bind(&VRGuiNav::update, g_nav) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateSem", bind(&VRGuiSemantics::updateOntoList, g_sem) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateScripts", bind(&VRGuiScripts::updateList, g_sc) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("scriptlist_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateBackground", bind(&VRGuiGeneral::updateScene, g_gen) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    updatePtr = VRUpdateCb::create("GUI_updateManager", bind(&VRGuiManager::update, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr, 1);

    GtkWindow* top = (GtkWindow*)getGUIBuilder()->get_widget("window1");
    gtk_window_maximize(top);
    gtk_widget_show_all((GtkWidget*)top);

    gtkUpdateCb = VRThreadCb::create( "gtk update", bind(&VRGuiManager::updateGtkThreaded, this, placeholders::_1) );
    cout << " done" << endl;
}

VRGuiManager::~VRGuiManager() {
    delete g_scene;
    delete g_bits;
    delete g_demos;
    delete g_nav;
    delete g_sem;
    delete g_sc;
    delete g_di;
    delete mtx;
}

void VRGuiManager::startThreadedUpdate() {
    if (gtkUpdateThreadID != -1) return;
    updateGtk();
    gtkUpdateThreadID = VRSceneManager::get()->initThread(gtkUpdateCb, "gtk update", true, 1);
}

boost::recursive_mutex& VRGuiManager::guiMutex() { return *mtx; }

VRGuiManager* VRGuiManager::get(bool init) {
    static VRGuiManager* instance = 0;
    if (instance == 0 && init) instance = new VRGuiManager();
    return instance;
}

void VRGuiManager::focusScript(string name, int line, int column) {
    g_sc->focusScript(name, line, column);
}

void VRGuiManager::getScriptFocus(VRScriptPtr& script, int& line, int& column) {
    script = g_sc->getSelectedScript();
    g_sc->getLineFocus(line, column);
}

void VRGuiManager::broadcast(string sig) {
    VRGuiSignals::get()->getSignal(sig)->triggerPtr<VRDevice>();
}

void VRGuiManager::wakeWindow() {
    setWidgetSensitivity("vpaned1", true);
    setWidgetSensitivity("notebook3", true);
}

VRConsoleWidgetPtr VRGuiManager::getConsole(string t) {
    if (standalone) return 0;
    return g_bits->getConsole(t);
}

void VRGuiManager::updateGtk() {
    PLock( guiMutex() );
    while( gtk_events_pending() ) gtk_main_iteration_do(false);
}

void VRGuiManager::updateGtkThreaded(VRThreadWeakPtr t) {
    updateGtk();
}

void VRGuiManager::update() {
    g_scene->update();
    if (!standalone) g_bits->update_terminals();
}

GtkWindow* VRGuiManager::newWindow() {
    GtkWindow* w = (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(w, 200, 200);
    gtk_widget_show_all((GtkWidget*)w);
    windows[w] = w;
    return w;
}

void VRGuiManager::remWindow(GtkWindow* w) {
    if (!windows.count(w)) return;
    windows.erase(w);
}

OSG_END_NAMESPACE;








