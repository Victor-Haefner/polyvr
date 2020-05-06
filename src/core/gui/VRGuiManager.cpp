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
#include <gtkmm/uimanager.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <gtk/gtkglinit.h>

#include <boost/bind.hpp>
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
Gtk::Main* GtkMain;

VRGuiManager::VRGuiManager() {
    cout << "Init VRGuiManager..";
    mtx = new boost::recursive_mutex();
    standalone = VROptions::get()->getOption<bool>("standalone");

    int argc = 0;
    GtkMain = new Gtk::Main(&argc, NULL, false);
    gtk_gl_init(&argc, NULL);
    getGUIBuilder(standalone);

    if (standalone) {
        cout << " start in standalone mode\n";
        VRSetupManager::get()->load("Desktop", "setup/Desktop.xml");

        updatePtr = VRUpdateCb::create("GUI_updateManager", boost::bind(&VRGuiManager::update, this) );
        VRSceneManager::get()->addUpdateFkt(updatePtr, 1);

        Gtk::Window* top = 0;
        getGUIBuilder()->get_widget("window1", top);
        top->maximize();
        top->show_all();
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
    editor->addKeyBinding("wipe", VRUpdateCb::create("wipeCb", boost::bind(&VRGuiBits::wipeConsoles, g_bits)));

    VRDeviceCbPtr fkt;
    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateSceneViewer", boost::bind(&VRGuiScene::updateTreeView, g_scene) );
    VRGuiSignals::get()->getSignal("scene_modified")->add( fkt );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("camera_changed")->add(fkt);
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateBits", boost::bind(&VRGuiBits::update, g_bits) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("camera_added")->add( fkt );
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateNav", boost::bind(&VRGuiNav::update, g_nav) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateSem", boost::bind(&VRGuiSemantics::updateOntoList, g_sem) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateScripts", boost::bind(&VRGuiScripts::updateList, g_sc) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("scriptlist_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateBackground", boost::bind(&VRGuiGeneral::updateScene, g_gen) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    updatePtr = VRUpdateCb::create("GUI_updateManager", boost::bind(&VRGuiManager::update, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr, 1);

    Gtk::Window* top = 0;
    getGUIBuilder()->get_widget("window1", top);
    top->maximize();
    top->show_all();

    gtkUpdateCb = VRThreadCb::create( "gtk update", boost::bind(&VRGuiManager::updateGtkThreaded, this, _1) );
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
    setVPanedSensitivity("vpaned1", true);
    setNotebookSensitivity("notebook3", true);
}

VRConsoleWidgetPtr VRGuiManager::getConsole(string t) {
    if (standalone) return 0;
    return g_bits->getConsole(t);
}

void VRGuiManager::updateGtk() {
    PLock( guiMutex() );
    while( Gtk::Main::events_pending() ) Gtk::Main::iteration();
}

void VRGuiManager::updateGtkThreaded(VRThreadWeakPtr t) {
    updateGtk();
}

void VRGuiManager::update() {
    g_scene->update();
    if (!standalone) g_bits->update_terminals();
}

Gtk::WindowPtr VRGuiManager::newWindow() {
    auto w = Gtk::WindowPtr( new Gtk::Window() );
    w->set_default_size(200, 200);
    w->show_all();
    windows[w.get()] = w;
    return w;
}

void VRGuiManager::remWindow(Gtk::WindowPtr w) {
    if (!windows.count(w.get())) return;
    windows.erase(w.get());
}

OSG_END_NAMESPACE;








