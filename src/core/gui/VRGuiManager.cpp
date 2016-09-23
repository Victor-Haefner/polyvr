#include "VRGuiManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
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

OSG_BEGIN_NAMESPACE;
using namespace std;

VRGuiScene* g_scene;
VRGuiBits* g_bits;
VRDemos* g_demos;
VRGuiNav* g_nav;
VRGuiScripts* g_sc;
VRGuiSetup* g_di;
VRGuiSemantics* g_sem;
VRGuiGeneral* g_gen;
VRGuiMonitor* g_mon;
Gtk::Main* GtkMain;

VRGuiManager::VRGuiManager() {
    standalone = VROptions::get()->getOption<bool>("standalone");

    int argc = 0;
    GtkMain = new Gtk::Main(&argc, NULL, false);
    gtk_gl_init(&argc, NULL);
    VRGuiBuilder(standalone);

    if (standalone) {
        cout << " start in standalone mode\n";
        VRSetupManager::get()->load("Desktop", "setup/Desktop.xml");

        updatePtr = VRFunction<int>::create("GUI_updateManager", boost::bind(&VRGuiManager::update, this) );
        VRSceneManager::get()->addUpdateFkt(updatePtr, 1);

        Gtk::Window* top = 0;
        VRGuiBuilder()->get_widget("window1", top);
        top->maximize();
        top->show_all();
        return;
    }

    //gtk_rc_parse("gui/gtkrc");
    g_demos = new VRDemos();
    g_scene = new VRGuiScene();
    g_bits = new VRGuiBits();
    g_nav = new VRGuiNav();
    g_sc = new VRGuiScripts();
    g_sem = new VRGuiSemantics();
    g_di = new VRGuiSetup();
    g_gen = new VRGuiGeneral();
    g_mon = new VRGuiMonitor();
    g_scene->updateTreeView();

    VRDeviceCb fkt;
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
    guiSignalCbs.push_back(fkt);

    fkt = VRFunction<VRDeviceWeakPtr>::create("GUI_updateBackground", boost::bind(&VRGuiGeneral::updateScene, g_gen) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    updatePtr = VRFunction<int>::create("GUI_updateManager", boost::bind(&VRGuiManager::update, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr, 1);

    Gtk::Window* top = 0;
    VRGuiBuilder()->get_widget("window1", top);
    top->maximize();
    top->show_all();
}

VRGuiManager::~VRGuiManager() {
    delete g_scene;
    delete g_bits;
    delete g_demos;
    delete g_nav;
    delete g_sem;
    delete g_sc;
    delete g_di;
}

VRGuiManager* VRGuiManager::get() {
    static VRGuiManager* instance = new VRGuiManager();
    return instance;
}

void VRGuiManager::broadcast(string sig) {
    VRGuiSignals::get()->getSignal(sig)->triggerPtr<VRDevice>();
}

void VRGuiManager::wakeWindow() {
    setVPanedSensitivity("vpaned1", true);
    setNotebookSensitivity("notebook3", true);
}

void VRGuiManager::printToConsole(string t, string s) {
    if (standalone) return;
    g_bits->write_to_terminal(t, s);
}

void VRGuiManager::updateGtk() {
    int N = 0;
    while( Gtk::Main::events_pending() ) {
        N++;
        Gtk::Main::iteration();
    }
    //cout << "N " << N << endl;
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








