#include "VRGuiManager.h"
#include "PolyVR.h"
#ifndef WITHOUT_IMGUI
#include "imgui/VRImguiManager.h"
#endif
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/scripting/VRScript.h"
#include "VRAppManager.h"
#include "VRGuiScene.h"
#include "VRGuiBits.h"
#include "VRGuiNav.h"
#include "VRGuiSignals.h"
#include "VRGuiScripts.h"
#include "VRGuiSetup.h"
#include "VRGuiGeneral.h"
#include "VRGuiMonitor.h"
#include "VRGuiNetwork.h"
#include "VRGuiSemantics.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRMutex.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRSignalT.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void saveScene(string path, bool saveas, string encryptionKey) {
    auto scene = OSG::VRScene::getCurrent();
    if (scene == 0) return;
    if (scene->getFlag("write_protected") && !saveas) return;
    scene->setFlag("write_protected", false);
    if (path == "") path = scene->getPath();
    OSG::VRSceneLoader::get()->saveScene(path, 0, encryptionKey);
    //saveSnapshot( scene->getIcon() );
    OSG::VRGuiSignals::get()->getSignal("onSaveScene")->triggerAll<OSG::VRDevice>();
}

VRGuiScene* g_scene = 0;
VRGuiBits* g_bits = 0;
VRAppManager* g_demos = 0;
VRGuiNav* g_nav = 0;
VRGuiScripts* g_sc = 0;
VRGuiSetup* g_di = 0;
VRGuiNetwork* g_net = 0;
VRGuiSemantics* g_sem = 0;
VRGuiGeneral* g_gen = 0;
VRGuiMonitor* g_mon = 0;

#ifndef WITHOUT_IMGUI
VRImguiManager* imguiMgr = 0;
#endif

VRGuiManager::VRGuiManager() {}

VRGuiManager::~VRGuiManager() {
    cout << "VRGuiManager::~VRGuiManager" << endl;
#ifndef WITHOUT_IMGUI
    uiCloseStore();
#endif
    if (g_scene) delete g_scene;
    if (g_bits) delete g_bits;
    if (g_demos) delete g_demos;
    if (g_nav) delete g_nav;
    if (g_net) delete g_net;
    if (g_sem) delete g_sem;
    if (g_sc) delete g_sc;
    if (g_di) delete g_di;
    if (mtx) delete mtx;
}

string VRGuiManager::genUUID() {
    int len = 16;
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i) s += alphanum[rand() % (sizeof(alphanum) - 1)];
    return s;
}

void VRGuiManager::init() {
    cout << "Init VRGuiManager.." << endl;
    mtx = new VRMutex();
    standalone = VROptions::get()->getOption<bool>("standalone");
    headless = VROptions::get()->getOption<bool>("headless");

    if (headless) {
        return;
    }

    //VRGuiBuilder::get(standalone);
    if (standalone) {
        cout << " start in standalone mode\n";
        VRSetupManager::get()->load("Desktop", "setup/DesktopMin.xml");
        updatePtr = VRUpdateCb::create("GUI_updateManager", bind(&VRGuiManager::update, this) );
        VRSceneManager::get()->addUpdateFkt(updatePtr, 1);
        //g_bits = new VRGuiBits();
        return;
    }

#ifndef WITHOUT_IMGUI
    imguiMgr = new VRImguiManager();
#endif

    string setupFile = "Desktop";
    ifstream f1("setup/.local");
    ifstream f2("setup/.default");
    if (f1.good()) getline(f1, setupFile);
    else if (f2.good()) getline(f2, setupFile);
    VRSetupManager::get()->load("Desktop", "setup/"+setupFile+".xml");

    g_demos = new VRAppManager();
    g_bits = new VRGuiBits();
    g_sc = new VRGuiScripts();

    VRDeviceCbPtr fkt; // TODO: all those signals are not properly connected to, the fkt binding is destroyed when going out of scope

    auto mgr = VRGuiSignals::get();
    mgr->addCallback("glutCloseWindow", [&](VRGuiSignals::Options o) { onWindowClose(); return true; });

    fkt = VRDeviceCb::create("GUI_updateBits", bind(&VRGuiBits::update, g_bits) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("camera_added")->add( fkt );
    guiSignalCbs.push_back(fkt);

    updatePtr = VRUpdateCb::create("GUI_updateManager", bind(&VRGuiManager::update, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr, 1);

    fkt = VRDeviceCb::create("GUI_updateScripts", bind(&VRGuiScripts::updateList, g_sc) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("scriptlist_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    g_scene = new VRGuiScene();
    g_gen = new VRGuiGeneral();

    fkt = VRDeviceCb::create("GUI_updateRendering", bind(&VRGuiGeneral::updateScene, g_gen) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    g_di = new VRGuiSetup();
    g_net = new VRGuiNetwork();

    return;

    //gtk_rc_parse("gui/gtkrc");
    g_mon = new VRGuiMonitor();
    g_nav = new VRGuiNav();
    g_sem = new VRGuiSemantics();



    auto editor = g_sc->getEditor();
    editor->addKeyBinding("wipe", VRUpdateCb::create("wipeCb", bind(&VRGuiBits::wipeConsoles, g_bits)));

    fkt = VRDeviceCb::create("GUI_updateSceneViewer", bind(&VRGuiScene::updateTreeView, g_scene) );
    VRGuiSignals::get()->getSignal("scene_modified")->add( fkt );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("camera_changed")->add(fkt);
    guiSignalCbs.push_back(fkt);

    fkt = VRDeviceCb::create("GUI_updateNav", bind(&VRGuiNav::update, g_nav) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
    VRGuiSignals::get()->getSignal("navpresets_changed")->add( fkt );
    guiSignalCbs.push_back(fkt);

    if (g_sem) {
        fkt = VRDeviceCb::create("GUI_updateSem", bind(&VRGuiSemantics::updateOntoList, g_sem) );
        VRGuiSignals::get()->getSignal("scene_changed")->add( fkt );
        guiSignalCbs.push_back(fkt);
    }


    /*GtkWindow* top = (GtkWindow*)VRGuiBuilder::get()->get_widget("window1");
    gtk_window_maximize(top);
    gtk_widget_show_all((GtkWidget*)top);

    // hide overlays
    setWidgetVisibility("navOverlay", false);

#ifdef _WIN32
    disableBlur(gtk_widget_get_window(GTK_WIDGET(top)));
#endif*/
    cout << " done" << endl;
}

void VRGuiManager::initImgui() {
#ifndef WITHOUT_IMGUI
    imguiMgr->setupCallbacks();
    imguiMgr->initImgui();
#endif
}

void VRGuiManager::initImguiPopup() {
#ifndef WITHOUT_IMGUI
    imguiMgr->initImguiPopup();
#endif
}

void VRGuiManager::onWindowClose() {
    cout << "VRGuiManager::onWindowClose" << endl;
    PolyVR::shutdown();
}

void VRGuiManager::setWindowTitle(string title) {
    //GtkWindow* top = (GtkWindow*)VRGuiBuilder::get()->get_widget("window1");
    //gtk_window_set_title(top, title.c_str());
}

void VRGuiManager::selectObject(VRObjectPtr obj) {
    if (g_scene) g_scene->selectObject(obj);
}

void VRGuiManager::openHelp(string search) {
    if (g_sc) g_sc->openHelp(search);
}

void VRGuiManager::updateSystemInfo() {
    if (g_mon) g_mon->updateSystemInfo();
}

VRMutex& VRGuiManager::guiMutex() { return *mtx; }

VRGuiManager* VRGuiManager::get(bool init) {
    static VRGuiManager* instance = 0;
    if (instance == 0 && init) instance = new VRGuiManager();
    return instance;
}

void VRGuiManager::focusScript(string name, int line, int column) {
    if (g_sc) g_sc->focusScript(name, line, column);
}

void VRGuiManager::getScriptFocus(VRScriptPtr& script, int& line, int& column) {
    if (g_sc) {
        script = g_sc->getSelectedScript();
        g_sc->getLineFocus(line, column);
    }
}

void VRGuiManager::broadcast(string sig) {
    VRGuiSignals::get()->getSignal(sig)->triggerAll<VRDevice>();
}

bool VRGuiManager::trigger(string name, VRGuiSignals::Options options) {
    return VRGuiSignals::get()->trigger(name, options);
}

bool VRGuiManager::triggerResize(string name, int x, int y, int w, int h) {
    return VRGuiSignals::get()->triggerResize(name, x,y,w,h);
}

void VRGuiManager::wakeWindow() {
    /*setWidgetSensitivity("vpaned1", true);
    setWidgetSensitivity("notebook3", true);*/
}

VRConsoleWidgetPtr VRGuiManager::getConsole(string t) {
    if (standalone || !g_bits) return 0;
    return g_bits->getConsole(t);
}

void VRGuiManager::update() {
    if (g_scene) g_scene->update();
    if (!standalone && g_bits) g_bits->update_terminals();
}

/*GtkWindow* VRGuiManager::newWindow() {
    GtkWindow* w = (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(w, 200, 200);
    gtk_widget_show_all((GtkWidget*)w);
    return w;
}

void VRGuiManager::remWindow(GtkWindow* w) {
}*/

OSG_END_NAMESPACE;
