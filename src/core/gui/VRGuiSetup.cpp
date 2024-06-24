#include <OpenSG/OSGRenderAction.h>

#include "VRGuiSetup.h"
#include "VRGuiSignals.h"
#include "VRGuiManager.h"
#include "PolyVR.h"

#include "core/scripting/VRScript.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRNetwork.h"
#include "core/setup/windows/VRWindow.h"
#include "core/setup/windows/VRMultiWindow.h"
#include "core/utils/system/VRSystem.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/setup/devices/VRFlystick.h"
#include "core/setup/tracking/VRPN.h"
#include "core/setup/tracking/ART.h"
#ifndef WITHOUT_VIRTUOSE
#include "core/setup/devices/VRHaptic.h"
#endif
#include "core/setup/devices/VRServer.h"
#include "core/setup/devices/VRMouse.h"
#ifndef WITHOUT_PRESENTER
#include "core/setup/devices/VRPresenter.h"
#endif
#ifndef WITHOUT_MTOUCH
#include "core/setup/devices/VRMultiTouch.h"
#endif
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/utils/VRManager.cpp"
#include "addons/LeapMotion/VRLeap.h"

#include "core/objects/VRCamera.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void VRGuiSetup::updateObjectData() {
    //cout << "VRGuiSetup::updateObjectData " << selected_type << ", " << selected_name << endl;
    bool isDevice = false;
    guard = true;

    auto scene = VRScene::getCurrent();

    if (selected_type == "window" && window) {
        VRWindowPtr win = dynamic_pointer_cast<VRWindow>(window);

        if (win) {
            string mtouch;
#ifndef WITHOUT_MTOUCH
            mtouch = win->getMultitouch() ? win->getMultitouch()->getName() : "";
#endif
            uiSignal( "on_setup_select_window", {
                {"name", win->getName()},
                {"sizeW", toString(win->getSize()[0])},
                {"sizeH", toString(win->getSize()[1])},
                {"active", toString(win->isActive())},
                {"mouse", win->getMouse() ? win->getMouse()->getName() : "" },
                {"multitouch", mtouch },
                {"keyboard", win->getKeyboard() ? win->getKeyboard()->getName() : "" },
                {"msaa", win->getMSAA()},
                {"title", win->getTitle()},
                {"icon", win->getIcon()}
            } );
        }

        if (window->hasType("distributed")) { // multiwindow
            VRMultiWindowPtr mwin = dynamic_pointer_cast<VRMultiWindow>(window);
            if (mwin) {
                int nx, ny;
                nx = mwin->getNXTiles();
                ny = mwin->getNYTiles();

                vector<string> serverIDs;
                for (int y=0; y<ny; y++) {
                    for (int x=0; x<nx; x++) {
                        string s = mwin->getServer(x,y);
                        serverIDs.push_back(s);
                    }
                }

                uiSignal( "on_setup_select_multiwindow", {
                    {"name", mwin->getName()},
                    {"state", mwin->getStateString()},
                    {"connType", mwin->getConnectionType()},
                    {"nx", toString(nx)},
                    {"ny", toString(ny)},
                    {"serverIDs", toString(serverIDs)}
                } );
            }
        }
    }

    if (selected_type == "view" && view) {
        Vec4d p = view->getPosition();

        uiSignal( "on_setup_select_view", {
            {"name", view->getName()},
            {"position", toString(p)},
            {"size", toString(view->getSize())},
            {"stereo", toString(view->isStereo())},
            {"eyesInverted", toString(view->eyesInverted())},
            {"activeStereo", toString(view->activeStereo())},
            {"projection", toString(view->isProjection())},
            {"mirror", toString(view->getMirror())},
            {"eyeSeparation", toString(view->getEyeSeparation())},
            {"userBeacon", view->getUser() ? view->getUser()->getName() : ""},
            {"projUser", toString(view->getProjectionUser())},
            {"projCenter", toString(view->getProjectionCenter())},
            {"projNormal", toString(view->getProjectionNormal())},
            {"projUp", toString(view->getProjectionUp())},
            {"projSize", toString(view->getProjectionSize())},
            {"projShear", toString(view->getProjectionShear())},
            {"projWarp", toString(view->getProjectionWarp())},
            {"mirrorPos", toString(view->getMirrorPos())},
            {"mirrorNorm", toString(view->getMirrorNorm())}
        } );
    }

    auto setup = VRSetup::getCurrent();
    if (setup) {
#ifndef WITHOUT_VRPN
        if (selected_name == "VRPN" || selected_type == "vrpn_device" || selected_type == "vrpn_tracker") {
            uiSignal( "on_setup_select_vrpn", {
                {"port", toString(setup->getVRPN()->getVRPNPort())},
                {"active", toString(setup->getVRPN()->getVRPNActive())}
            } );
        }
#endif

#ifndef WITHOUT_ART
        if (selected_name == "ART") {
            uiSignal( "on_setup_select_art", {
                {"port", toString(setup->getART()->getARTPort())},
                {"active", toString(setup->getART()->getARTActive())},
                {"offset", toString(setup->getART()->getARTOffset())},
                {"axis", toString(setup->getART()->getARTAxis())}
            } );
        }
#endif

        if (selected_name == "Displays") {
            uiSignal( "on_setup_select_display", {
                {"offset", toString(setup->getDisplaysOffset())}//,
                //{"active", toString(setup->getVRPN()->getVRPNActive())}
            } );
        }
    }

#ifndef WITHOUT_VRPN
    if (selected_type == "vrpn_device" && vrpn_device) {
        isDevice = true;
        uiSignal( "on_setup_select_vrpn_device", {
            {"name", vrpn_device->getName()},
            {"address", vrpn_device->address}
        } );
    }

    if (selected_type == "vrpn_tracker" && vrpn_tracker) {
        uiSignal( "on_setup_select_vrpn_tracker", {
            {"name", vrpn_tracker->getName()},
            {"address", vrpn_tracker->address},
            {"tAxis", toString(vrpn_tracker->translate_axis)},
            {"rAxis", toString(vrpn_tracker->rotation_axis)}
        } );
    }
#endif

    if (selected_type == "art_device" && art_device) {
        isDevice = true;
        uiSignal( "on_setup_select_art_device", {
            {"name", art_device->getName()},
            {"ID", toString(art_device->ID)}
        } );
    }

#ifndef WITHOUT_VIRTUOSE
    if (selected_type == "haptic" && haptic_device) {
        isDevice = true;
        uiSignal( "on_setup_select_haptic_device", {
            {"name", haptic_device->getName()},
            {"IP", haptic_device->getIP()},
            {"type", haptic_device->getType()},
            {"deamonState", haptic_device->getDeamonState()},
            {"deviceState", haptic_device->getDeviceState()}
        } );
    }
#endif

#ifndef WITHOUT_MTOUCH
    if (selected_type == "multitouch" && mtouch_device) {
        isDevice = true;
        uiSignal( "on_setup_select_multitouch", {
            {"name", mtouch_device->getName()},
            {"device", mtouch_device->getDevice()}
        } );
    }
#endif

    if (selected_type == "leap" && leap_device) {
        isDevice = true;
        uiSignal( "on_setup_select_multitouch", {
            {"name", leap_device->getName()},
            {"address", leap_device->getAddress()},
            {"connection", leap_device->getConnectionStatus()},
            {"serial", leap_device->getSerial()},
            {"pose", toString(leap_device->getPose())}
        } );
    }

    if (selected_type == "mouse") { isDevice = true; }
    if (selected_type == "keyboard") { isDevice = true; }
    if (selected_type == "server") { isDevice = true; }
    if (selected_type == "flystick") { isDevice = true; }

    if (isDevice) {
        auto ins = device->getLastIntersection();

        string iObj;
        string iPnt;
        string iTxl;

        if (ins) {
            iObj = ins->hit ? ins->name : "NONE";
            iPnt = toString(ins->point);
            iTxl = toString(ins->texel);
        }

        bool crossVisible = false;
        if (device->getCross()) crossVisible = device->getCross()->isVisible();

        uiSignal( "on_setup_select_device", {
            {"name", device->getName()},
            {"type", device->getType()},
            {"iObj", iObj},
            {"iPnt", iPnt},
            {"iTxl", iTxl},
            {"cross", toString(crossVisible)}
        } );
    }

    if (selected_type == "node" && node) {
        uiSignal( "on_setup_select_node", {
            {"address", node->getAddress()},
            {"user", node->getUser()},
            {"slave", node->getSlavePath()},
            {"nodeStatus", node->getStatNode()},
            {"sshStatus", node->getStatSSH()},
            {"sshKeyStatus", node->getStatSSHkey()},
            {"pathStatus", node->getStatPath()}
        } );
    }

    if (selected_type == "slave" && slave) {
        uiSignal( "on_setup_select_slave", {
            {"connectionID", slave->getConnectionIdentifier()},
            {"multicast", slave->getStatMulticast()},
            {"status", slave->getStat()},
            {"fullscreen", toString(slave->getFullscreen())},
            {"activeStereo", toString(slave->getActiveStereo())},
            {"autostart", toString(slave->getAutostart())},
            {"display", slave->getDisplay()},
            {"connectionType", slave->getConnectionType()},
            {"port", toString(slave->getPort())},
            {"startupDelay", toString(slave->getStartupDelay())},
            {"geometry", toString(slave->getGeometry())}
        } );

        auto displayList = slave->getAvailableDisplays();
        uiSignal("updateDisplayList", {{"list", toString(displayList)}});
    }

    if (selected_type == "script") {
        /*setWidgetVisibility("expander29", true, true);
        VRScript* script = (VRScript*)selected_object;
        editor->setCore(script->getHead() + script->getCore(), script->getHeadSize());
        auto trigs = script->getTriggers();
        setToggleButton("radiotoolbutton1", true);
        if (trigs.size() > 0) {
            auto trig = *trigs.begin();
            if (trig) {
                if (trig->trigger == "on_timeout") setToggleButton("radiotoolbutton2", true);
                else setToggleButton("radiotoolbutton3", true);
            }
        }*/
    }

    guard = false;
}

string VRGuiSetup::setupDir() { return VRSceneManager::get()->getOriginalWorkdir()+"/setup/"; }

// --------------------------
// ---------Callbacks--------
// --------------------------

// toolbuttons

void VRGuiSetup::on_new_clicked() {
    guard = true;
    auto setup = VRSetupManager::get()->newSetup();
    if (!setup) return;

    on_save_clicked();
    // remember setup
    string name = setup->getName();
    ofstream f("setup/.local"); f.write(name.c_str(), name.size()); f.close();

    updateSetupList();
    updateSetup();
    guard = false;
}

void VRGuiSetup::on_foto_clicked(bool b) {
    if (auto s = VRSetup::getCurrent()) s->setFotoMode(b);
}

void VRGuiSetup::on_del_clicked() { //TODO, should delete setup
    string msg1 = "Delete setup ";
    //if (!askUser(msg1, "Are you sure you want to delete this script?")) return;
    return;

    /*VRGuiTreeView tree_view("treeview2");
    string name = tree_view.getSelectedStringValue(0);
    tree_view.removeSelected();
    if (auto s = current_setup.lock()) s->removeWindow(name);*/
}

void VRGuiSetup::on_save_clicked() {
    if (auto s = VRSetup::getCurrent()) {
        cout << "save setup " << s->getName() << endl;
        s->save(setupDir() + s->getName() + ".xml");
    }
}

void VRGuiSetup::on_diag_save_as_clicked() {
    guard = true;
    /*string path = VRGuiFile::getPath();
    if (path == "") return;

    if (auto s = current_setup.lock()) {
        s->save(path);
        string name = s->getName();
        ofstream f("setup/.local"); f.write(name.c_str(), name.size()); f.close();
        setWidgetSensitivity("toolbutton12", false);
    }*/

    updateSetupList();
    updateSetup();
    guard = false;
}

void VRGuiSetup::on_save_as_clicked() {
    /*VRGuiFile::setCallbacks( bind(&VRGuiSetup::on_diag_save_as_clicked, this) );
    VRGuiFile::gotoPath( setupDir() );
    VRGuiFile::setFile( "mySetup.pvr" );
    VRGuiFile::clearFilter();
    VRGuiFile::open( "Save As..", "save", "Save Setup As.." );*/
}

// setup list
void VRGuiSetup::on_treeview_select(string selected) {
    uiSignal( "on_setup_select_clear", {} );

    auto setup = VRSetup::getCurrent();
    if (!setup) return;

    window = 0;
    view = 0;
    node = 0;
    slave = 0;
    device = 0;
    art_device = 0;
    vrpn_device = 0;
    vrpn_tracker = 0;
    haptic_device = 0;
    leap_device = 0;
    mtouch_device = 0;

    selected_type = "";
    selected_name = "";

    auto parts = splitString(selected,'$');
    if (parts.size() == 1) selected_name = parts[0];
    if (parts.size() != 2) return;

    selected_type = parts[0];
    selected_name = parts[1];
    //cout << "on_treeview_select " << selected_type << ", " << selected_name << endl;

    if (selected_type == "window") window = setup->getWindow(selected_name);
    else if (selected_type == "view") view = setup->getView(selected_name);
    else if (selected_type == "node") node = setup->getNetwork()->get(selected_name);
    else if (selected_type == "slave") slave = setup->getNetwork()->getSlave(selected_name);
#ifndef WITHOUT_VRPN
    else if (selected_type == "vrpn_tracker") vrpn_tracker = setup->getVRPN()->getVRPNTracker(toInt(selected_name));
    else if (selected_type == "vrpn_device") vrpn_device = setup->getVRPN()->getVRPNTracker(toInt(selected_name));
#endif
#ifndef WITHOUT_ART
    else if (selected_type == "art_device") art_device = setup->getART()->getARTDevice(toInt(selected_name));
#endif
    else device = setup->getDevice(selected_name);

    if (selected_type == "leap") leap_device = dynamic_pointer_cast<VRLeap>( device );
#ifndef WITHOUT_VIRTUOSE
    else if (selected_type == "haptic") haptic_device = dynamic_pointer_cast<VRHaptic>( device );
#endif
#ifndef WITHOUT_MTOUCH
    else if(selected_type == "multitouch") mtouch_device = dynamic_pointer_cast<VRMultiTouch>( device );
#endif

    updateObjectData();
}

void VRGuiSetup::on_treeview_rename(string ID, string name) {
    // VRGuiSetup_ModelColumns cols;
    //  add(name); add(type); add(obj);
    /*VRGuiTreeView tree_view("treeview2");
    string name  = tree_view.getSelectedStringValue(0);
    string type  = tree_view.getSelectedStringValue(1);
    gpointer obj = tree_view.getSelectedValue(2);

    // update key in map
    if (auto s = current_setup.lock()) {
        if (type == "window") s->changeWindowName(name, new_name);
#ifndef WITHOUT_VRPN
        if (type == "vrpn_tracker") s->getVRPN()->changeVRPNDeviceName(((VRPN_device*)obj)->ptr(), new_name);
#endif
        if (type == "node") ((VRNetworkNode*)obj)->setName(new_name);
        if (type == "slave") ((VRNetworkSlave*)obj)->setName(new_name);
    }

    tree_view.setSelectedStringValue(0, name);*/
    updateSetup();
}

bool VRGuiSetup::on_treeview_rightclick() {
    /*if (event->type != GDK_BUTTON_RELEASE) return false;
    if (event->button-1 != 2) return false;

    //open contextmenu
    menu->popup("SetupMenu", event);*/
	return true;
}


void VRGuiSetup::on_menu_delete(string node) {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;

    string name = splitString(node, '$')[1];
    string type = splitString(node, '$')[0];

    if (type == "window") {} // TODO

    if (type == "view") {} // TODO

#ifndef WITHOUT_VRPN
    if (type == "vrpn_tracker") {} // TODO
#endif

    if (type == "art_device") {} // deprecated?

    if (type == "node") setup->getNetwork()->remNode( name );
    if (type == "slave") setup->getNetwork()->remSlave( name );

    updateSetup();
}

void VRGuiSetup::on_menu_add_window() {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    auto win = setup->addMultiWindow("Display");
    win->setActive(true);
    if ( VRScene::getCurrent() ) win->setContent(true);
    on_menu_add_viewport(win->getName());
}

void VRGuiSetup::on_menu_add_viewport(string winName) {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    auto win = setup->getWindow(winName);
    if (!win) return;

    int v = setup->addView(win->getBaseName());
    auto view = setup->getView(v);
    win->addView(view);

    if ( auto scene = VRScene::getCurrent() ) {
        setup->setViewRoot(scene->getRoot(), v);
        view->setCamera( scene->getActiveCamera() );
        view->setBackground( scene->getBackground() );
    }

    updateSetup();
}

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_menu_add_vrpn_tracker() {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    setup->getVRPN()->addVRPNTracker(0, "Tracker0@localhost", Vec3d(0,0,0), 1);
    //setup->addVRPNTracker(0, "LeapTracker@tcp://141.3.151.136", Vec3d(0,0,0), 1);

    updateSetup();
    //VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

template<class T>
void VRGuiSetup::on_menu_add_device() {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    setup->addDevice(T::create());
    updateSetup();
    //VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_menu_add_network_node() {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    setup->getNetwork()->add("Node");
    updateSetup();
    //VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_menu_add_network_slave(string node) {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    VRNetworkNodePtr n = setup->getNetwork()->getNode(node);
    if (n) n->add("Slave");
    updateSetup();
}

void VRGuiSetup::on_menu_add_script() {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    setup->addScript("script");
    updateSetup();
    //VRGuiWidget("toolbutton12").setSensitivity(true);
}

// window options

void VRGuiSetup::on_servern_edit(int Nx, int Ny) {
    if (guard || !window) return;
    auto mwin = dynamic_pointer_cast<VRMultiWindow>(window);
    if (!mwin) return;
    mwin->setNTiles(Nx, Ny);
    updateObjectData();
}

void VRGuiSetup::on_server_set_connection(string ct) {
    if (guard || !window) return;
    auto mwin = dynamic_pointer_cast<VRMultiWindow>(window);
    if (!mwin) return;
    mwin->setConnectionType(ct);
    mwin->reset();
    updateObjectData();
}

void VRGuiSetup::on_server_edit(int x, int y, string sID) {
    if (guard || !window) return;
    auto mwin = dynamic_pointer_cast<VRMultiWindow>(window);
    if (!mwin) return;
    mwin->setServer(x,y,sID);
    mwin->reset();
    updateObjectData();
}

void VRGuiSetup::on_connect_mw_clicked() {
    if (selected_type != "window") return;
    auto mwin = dynamic_pointer_cast<VRMultiWindow>(window);
    if (!mwin) return;
    mwin->reset();
}

// view options

void VRGuiSetup::on_toggle_view_stats(bool b) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->showStats(b);
}

void VRGuiSetup::on_toggle_display_stereo(bool b) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setStereo(b);
}

void VRGuiSetup::on_toggle_display_projection(bool b) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setProjection(b);
}

void VRGuiSetup::on_toggle_view_invert(bool b) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->swapEyes(b);
}

void VRGuiSetup::on_toggle_view_active_stereo(bool b) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setActiveStereo(b);
}

void VRGuiSetup::on_pos_edit(Vec4d pos) {
    if (guard) return;
    if (selected_type != "view") return;
    view->setPosition(pos);
}

void VRGuiSetup::on_eyesep_edit(float d) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setStereoEyeSeparation(d);
}

void VRGuiSetup::on_toggle_view_user(bool b) {
    if (guard) return;
    if (selected_type != "view") return;
    //if (!b) view->setUser(0);
    //else view->setUser(u);
}

void VRGuiSetup::on_toggle_view_mirror(bool b) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setMirror(b);
}

void VRGuiSetup::on_view_mirror_pos_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setMirrorPos(v);
}

void VRGuiSetup::on_view_mirror_norm_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setMirrorNorm(v);
}

void VRGuiSetup::on_change_view_user(string name) {
    if (guard) return;
    if (selected_type != "view") return;
    auto u = dynamic_pointer_cast<VRTransform>( VRScene::getCurrent()->get(name) );
    if (view && u) view->setUser(u);
}

void VRGuiSetup::on_view_size_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setSize(Vec2i(v));
}

void VRGuiSetup::on_proj_user_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setProjectionUser(v);
}

void VRGuiSetup::on_proj_center_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setProjectionCenter(v);
}

void VRGuiSetup::on_proj_normal_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setProjectionNormal(v);
}

void VRGuiSetup::on_proj_up_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setProjectionUp(v);
}

void VRGuiSetup::on_proj_size_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setProjectionSize(v);
}

void VRGuiSetup::on_proj_shear_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setProjectionShear(v);
}

void VRGuiSetup::on_proj_warp_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;
    if (view) view->setProjectionWarp(v);
}

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_vrpn_trans_axis_edit(Vec3d v) {
    if (guard) return;

    /*if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    t->setTranslationAxis(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_vrpn_rot_axis_edit(Vec3d v) {
    if (guard) return;

    /*if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    t->setRotationAxis(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

// tracker

#ifndef WITHOUT_ART
void VRGuiSetup::on_toggle_art() {
    if (guard) return;
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    /*bool b = getCheckButtonState("checkbutton24");
    setup->getART()->setARTActive(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_toggle_vrpn() {
    if (guard) return;
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    /*bool b = getCheckButtonState("checkbutton25");
    setup->getVRPN()->setVRPNActive(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

#ifndef WITHOUT_ART
void VRGuiSetup::on_art_edit_port() {
    if (guard) return;
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    /*int p = toInt(getTextEntry("entry39"));
    setup->getART()->setARTPort(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

void VRGuiSetup::on_displays_edit_offset() {
    if (guard) return;
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    /*float ox = toFloat(getTextEntry("entry29"));
    float oy = toFloat(getTextEntry("entry30"));
    float oz = toFloat(getTextEntry("entry31"));
    setup->setDisplaysOffset(Vec3d(ox,oy,oz));
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

#ifndef WITHOUT_ART
void VRGuiSetup::on_art_edit_offset(Vec3d v) {
    if (guard) return;
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    /*setup->getART()->setARTOffset(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_art_edit_axis(Vec3d v) {
    if (guard) return;
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    /*setup->getART()->setARTAxis(Vec3i(round(v[0]), round(v[1]), round(v[2])));
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_art_edit_id() {
    if (guard) return;
    /*int id = toInt(getTextEntry("entry40"));
    ART_device* dev = (ART_device*)selected_object;
    dev->ID = id;
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_vrpn_edit_port() {
    if (guard) return;
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    /*int p = toInt(getTextEntry("entry13"));
    setup->getVRPN()->setVRPNPort(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_edit_VRPN_tracker_address() {
    if (guard) return;
    if (selected_type != "vrpn_tracker") return;
    /*VRPN_device* t = (VRPN_device*)selected_object;

    string txt = getTextEntry("entry50");
    t->setAddress(txt);

    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

void VRGuiSetup::on_netnode_address_edited(string s) {
    if (guard || !node) return;
    node->setAddress(s);
    updateObjectData();
}

void VRGuiSetup::on_netnode_user_edited(string s) {
    if (guard || !node) return;
    node->setUser(s);
    updateObjectData();
}

void VRGuiSetup::on_netnode_path_edited(string s) {
    if (guard || !node) return;
    node->setSlavePath(s);
    updateObjectData();
}

void VRGuiSetup::on_netnode_key_clicked() {
    if (guard || !node) return;
    node->distributeKey();
    updateObjectData();
}

void VRGuiSetup::on_netnode_stopall_clicked() {
    if (guard || !node) return;
    node->stopSlaves();
    updateObjectData();
}

void VRGuiSetup::on_netslave_set_autostart(bool b) { if (slave && !guard) { slave->setAutostart(b); updateObjectData(); } }
void VRGuiSetup::on_netslave_set_fullscreen(bool b) { if (slave && !guard) { slave->setFullscreen(b); updateObjectData(); } }
void VRGuiSetup::on_netslave_set_activestereo(bool b) { if (slave && !guard) { slave->setActiveStereo(b); updateObjectData(); } }
void VRGuiSetup::on_netslave_set_port(int p) { if (slave && !guard) { slave->setPort(p); updateObjectData(); } }
void VRGuiSetup::on_netslave_set_delay(int d) { if (slave && !guard) { slave->setDelay(d); updateObjectData(); } }
void VRGuiSetup::on_netslave_set_screen(string s) { if (slave && !guard) { slave->setDisplay(s); updateObjectData(); } }
void VRGuiSetup::on_netslave_set_geometry(string g) { if (slave && !guard) { slave->setGeometry(g); updateObjectData(); } }
void VRGuiSetup::on_netslave_start_clicked() { if (slave && !guard) { slave->start(); updateObjectData(); } }
void VRGuiSetup::on_netslave_set_connection(string ct) { if (slave && !guard) { slave->setConnectionType(ct); updateObjectData(); } }

#ifndef WITHOUT_VIRTUOSE
void VRGuiSetup::on_haptic_ip_edited() {
    if (guard) return;
    /*RHaptic* dev = (VRHaptic*)selected_object;
    dev->setIP(getTextEntry("entry8"));
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

void VRGuiSetup::on_leap_host_edited() {
    if (guard) return;
    /*VRLeap* dev = (VRLeap*)selected_object;
    dev->setAddress(getTextEntry("entry28"));
    dev->reconnect();
    setLabel("label157", dev->getConnectionStatus());
    setLabel("label159", dev->getSerial());
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_leap_startcalib_clicked() {
    if (guard) return;
    /*VRLeap* dev = (VRLeap*)selected_object;
    dev->startCalibration();
    setWidgetSensitivity("button34", false);
    setWidgetSensitivity("button35", true);*/
}
void VRGuiSetup::on_leap_stopcalib_clicked() {
    if (guard) return;
    /*VRLeap* dev = (VRLeap*)selected_object;
    dev->stopCalibration();
    setWidgetSensitivity("button34", true);
    setWidgetSensitivity("button35", false);
    auto p = dev->getPose();
    leapPosEntry.set(p->pos());
    leapUpEntry.set(p->up());
    leapDirEntry.set(p->dir());
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_leap_pos_edit(Vec3d v) {
    if (guard) return;
    /*VRLeap* dev = (VRLeap*)selected_object;
    auto p = dev->getPose();
    p->setPos(v);
    dev->setPose(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_leap_up_edit(Vec3d v) {
    if (guard) return;
    /*VRLeap* dev = (VRLeap*)selected_object;
    auto p = dev->getPose();
    p->setUp(v);
    dev->setPose(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_leap_dir_edit(Vec3d v) {
    if (guard) return;
    /*VRLeap* dev = (VRLeap*)selected_object;
    auto p = dev->getPose();
    p->setDir(v);
    dev->setPose(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

#ifndef WITHOUT_VIRTUOSE
void VRGuiSetup::on_change_haptic_type() {
    if (guard) return;
    /*VRHaptic* dev = (VRHaptic*)selected_object;
    dev->setType(getComboboxText("combobox25"));
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

#ifndef WITHOUT_MTOUCH
void VRGuiSetup::on_mt_device_changed() {
    if (guard) return;
    /*VRMultiTouch* dev = (VRMultiTouch*)selected_object;
    dev->setDevice(getComboboxText("combobox12"));
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}
#endif

void VRGuiSetup::on_toggle_dev_cross() {
    if (guard) return;

    /*bool b = getCheckButtonState("checkbutton37");
    VRDevice* dev = (VRDevice*)selected_object;
    dev->showHitPoint(b);*/
}

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_toggle_vrpn_test_server() {
    if (guard) return;
    /*auto setup = VRSetup::getCurrent();
    if (!setup) return;
    bool b = getCheckButtonState("checkbutton39");
    if (b) setup->getVRPN()->startVRPNTestServer();
    else setup->getVRPN()->stopVRPNTestServer();*/
}

void VRGuiSetup::on_toggle_vrpn_verbose() {
    if (guard) return;
    /*auto setup = VRSetup::getCurrent();
    if (!setup) return;
    bool b = getCheckButtonState("checkbutton40");
    setup->getVRPN()->setVRPNVerbose(b);*/
}
#endif

VRScriptPtr VRGuiSetup::getSelectedScript() {
    /*auto script = (VRScript*)selected_object;
    return script->ptr();*/
    return 0;
}

void VRGuiSetup::on_script_save_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    /*string core = editor->getCore();
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core);

    setWidgetSensitivity("toolbutton27", false);
    VRGuiWidget("toolbutton12").setSensitivity(true);*/
}

void VRGuiSetup::on_script_exec_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;
    on_script_save_clicked();

    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->triggerScript(script->getName());
}

void VRGuiSetup::on_script_trigger_switched() {
    /*bool noTrigger = getRadioToolButtonState("radiotoolbutton1");
    bool onFrame = getRadioToolButtonState("radiotoolbutton2");
    bool onStart = getRadioToolButtonState("radiotoolbutton3");

    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;
    on_script_save_clicked();
    VRGuiWidget("toolbutton12").setSensitivity(true);

    for (auto t : script->getTriggers()) script->remTrigger(t->getName());
    if (noTrigger) return;
    auto trig = script->addTrigger();
    script->changeTrigParams(trig->getName(), "0");
    if (onStart) script->changeTrigger(trig->getName(), "on_scene_load");
    if (onFrame) script->changeTrigger(trig->getName(), "on_timeout");*/
}

shared_ptr<VRGuiEditor> VRGuiSetup::getEditor() { return editor; }

void VRGuiSetup::on_script_changed() {
    /*setWidgetSensitivity("toolbutton27", true);

    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    string core = getEditor()->getCore();
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core, false);*/
}

// --------------------------
// ---------Main-------------
// --------------------------

namespace PL = std::placeholders;

VRGuiSetup::VRGuiSetup() {
    /*selected_object = 0;
    mwindow = 0;
    guard = true;

    updatePtr = VRUpdateCb::create("Setup_gui", bind(&VRGuiSetup::updateStatus, this));
    VRSceneManager::get()->addUpdateFkt(updatePtr);

    menu = new VRGuiContextMenu("SetupMenu");
    menu->appendMenu("SetupMenu", "Add", "SM_AddMenu");
    menu->appendItem("SetupMenu", "Delete", bind( &VRGuiSetup::on_menu_delete, this));
    menu->appendItem("SM_AddMenu", "Window", bind( &VRGuiSetup::on_menu_add_window, this) );
    menu->appendItem("SM_AddMenu", "Viewport", bind( &VRGuiSetup::on_menu_add_viewport, this) );
    menu->appendMenu("SM_AddMenu", "Network", "SM_AddNetworkMenu");
    menu->appendMenu("SM_AddMenu", "Device", "SM_AddDevMenu");
    menu->appendMenu("SM_AddMenu", "VRPN", "SM_AddVRPNMenu");
    menu->appendItem("SM_AddDevMenu", "Mouse", bind( &VRGuiSetup::on_menu_add_device<VRMouse>, this) );
#ifndef WITHOUT_PRESENTER
    menu->appendItem("SM_AddDevMenu", "Presenter", bind( &VRGuiSetup::on_menu_add_device<VRPresenter>, this) );
#endif
#ifndef WITHOUT_MTOUCH
    menu->appendItem("SM_AddDevMenu", "MultiTouch", bind( &VRGuiSetup::on_menu_add_device<VRMultiTouch>, this) );
#endif
    menu->appendItem("SM_AddDevMenu", "Leap", bind( &VRGuiSetup::on_menu_add_device<VRLeap>, this) );
    menu->appendItem("SM_AddDevMenu", "Keyboard", bind( &VRGuiSetup::on_menu_add_device<VRKeyboard>, this) );
#ifndef WITHOUT_VIRTUOSE
    menu->appendItem("SM_AddDevMenu", "Haptic", bind( &VRGuiSetup::on_menu_add_device<VRHaptic>, this) );
#endif
    menu->appendItem("SM_AddDevMenu", "Server", bind( &VRGuiSetup::on_menu_add_device<VRServer>, this) );
#ifndef WITHOUT_VRPN
    menu->appendItem("SM_AddVRPNMenu", "VRPN tracker", bind( &VRGuiSetup::on_menu_add_vrpn_tracker, this) );
#endif
    menu->appendItem("SM_AddNetworkMenu", "Node", bind( &VRGuiSetup::on_menu_add_network_node, this) );
    menu->appendItem("SM_AddNetworkMenu", "Slave", bind( &VRGuiSetup::on_menu_add_network_slave, this) );
    menu->appendItem("SM_AddMenu", "Script", bind( &VRGuiSetup::on_menu_add_script, this) );

    setLabel("label13", "VR Setup: None");

    setButtonCallback("button25", bind( &VRGuiSetup::on_connect_mw_clicked, this) );

    setToolButtonCallback("toolbutton10", bind( &VRGuiSetup::on_new_clicked, this) );
    setToolButtonCallback("toolbutton11", bind( &VRGuiSetup::on_del_clicked, this) );
    setToolButtonCallback("toolbutton12", bind( &VRGuiSetup::on_save_clicked, this) );
    setToolButtonCallback("toolbutton13", bind( &VRGuiSetup::on_save_as_clicked, this) );
    //setToolButtonCallback("toolbutton27", bind( &VRGuiSetup::on_script_save_clicked, this) );
    //setToolButtonCallback("toolbutton26", bind( &VRGuiSetup::on_script_exec_clicked, this) );

    //setRadioToolButtonCallback("radiotoolbutton1", bind( &VRGuiSetup::on_script_trigger_switched, this) );
    //setRadioToolButtonCallback("radiotoolbutton2", bind( &VRGuiSetup::on_script_trigger_switched, this) );
    //setRadioToolButtonCallback("radiotoolbutton3", bind( &VRGuiSetup::on_script_trigger_switched, this) );

#ifndef WITHOUT_ART
    artAxis.init("art_axis", "Axis", bind( &VRGuiSetup::on_art_edit_axis, this, PL::_1));
    artOffset.init("art_offset", "Offset", bind( &VRGuiSetup::on_art_edit_offset, this, PL::_1));
#endif

    centerEntry.init("center_entry", "center", bind( &VRGuiSetup::on_proj_center_edit, this, PL::_1));
    userEntry.init("user_entry", "user", bind( &VRGuiSetup::on_proj_user_edit, this, PL::_1));
    normalEntry.init("normal_entry", "normal", bind( &VRGuiSetup::on_proj_normal_edit, this, PL::_1));
    upEntry.init("viewup_entry", "up", bind( &VRGuiSetup::on_proj_up_edit, this, PL::_1));
    mirrorPosEntry.init("mirror_pos_entry", "origin", bind( &VRGuiSetup::on_view_mirror_pos_edit, this, PL::_1));
    mirrorNormEntry.init("mirror_norm_entry", "normal", bind( &VRGuiSetup::on_view_mirror_norm_edit, this, PL::_1));
    sizeEntry.init2D("size_entry", "size", bind( &VRGuiSetup::on_proj_size_edit, this, PL::_1));
    shearEntry.init2D("shear_entry", "shear", bind( &VRGuiSetup::on_proj_shear_edit, this, PL::_1));
    warpEntry.init2D("warp_entry", "warp", bind( &VRGuiSetup::on_proj_warp_edit, this, PL::_1));
    vsizeEntry.init2D("vsize_entry", "size", bind( &VRGuiSetup::on_view_size_edit, this, PL::_1));

#ifndef WITHOUT_VRPN
    tVRPNAxisEntry.init("tvrpn_entry", "", bind( &VRGuiSetup::on_vrpn_trans_axis_edit, this, PL::_1));
    rVRPNAxisEntry.init("rvrpn_entry", "", bind( &VRGuiSetup::on_vrpn_rot_axis_edit, this, PL::_1));
#endif

    leapPosEntry.init("leap_pos_entry", "from", bind( &VRGuiSetup::on_leap_pos_edit, this, PL::_1));
    leapUpEntry.init("leap_up_entry", "up", bind( &VRGuiSetup::on_leap_up_edit, this, PL::_1));
    leapDirEntry.init("leap_dir_entry", "dir", bind( &VRGuiSetup::on_leap_dir_edit, this, PL::_1));

#ifndef WITHOUT_VRPN
	setEntryCallback("entry13", bind( &VRGuiSetup::on_vrpn_edit_port, this));
    setEntryCallback("entry50", bind( &VRGuiSetup::on_edit_VRPN_tracker_address, this) );
#endif
    setEntryCallback("entry52", bind( &VRGuiSetup::on_pos_edit, this) );
    setEntryCallback("entry53", bind( &VRGuiSetup::on_pos_edit, this) );
    setEntryCallback("entry56", bind( &VRGuiSetup::on_pos_edit, this) );
    setEntryCallback("entry57", bind( &VRGuiSetup::on_pos_edit, this) );
    setEntryCallback("entry12", bind( &VRGuiSetup::on_eyesep_edit, this) );
    setEntryCallback("entry33", bind( &VRGuiSetup::on_servern_edit, this) );
    setEntryCallback("entry34", bind( &VRGuiSetup::on_servern_edit, this) );
#ifndef WITHOUT_ART
    setEntryCallback("entry39", bind( &VRGuiSetup::on_art_edit_port, this) );
    setEntryCallback("entry40", bind( &VRGuiSetup::on_art_edit_id, this) );
#endif
    setEntryCallback("entry29", bind( &VRGuiSetup::on_displays_edit_offset, this) );
    setEntryCallback("entry30", bind( &VRGuiSetup::on_displays_edit_offset, this) );
    setEntryCallback("entry31", bind( &VRGuiSetup::on_displays_edit_offset, this) );
#ifndef WITHOUT_VIRTUOSE
    setEntryCallback("entry8", bind( &VRGuiSetup::on_haptic_ip_edited, this) );
#endif
    setEntryCallback("entry28", bind( &VRGuiSetup::on_leap_host_edited, this) );
    setEntryCallback("entry15", bind( &VRGuiSetup::on_netnode_edited, this) );
    setEntryCallback("entry20", bind( &VRGuiSetup::on_netnode_edited, this) );
    setEntryCallback("entry32", bind( &VRGuiSetup::on_netnode_edited, this) );
    setEntryCallback("entry19", bind( &VRGuiSetup::on_netslave_edited, this) );
    setEntryCallback("entry22", bind( &VRGuiSetup::on_netslave_edited, this) );
    setEntryCallback("entry37", bind( &VRGuiSetup::on_netslave_edited, this) );
    setEntryCallback("entry38", bind( &VRGuiSetup::on_netslave_edited, this) );

    setButtonCallback("button6", bind( &VRGuiSetup::on_netnode_key_clicked, this) );
    setButtonCallback("button1", bind( &VRGuiSetup::on_netslave_start_clicked, this) );
    setButtonCallback("button30", bind( &VRGuiSetup::on_netnode_stopall_clicked, this) );
    setButtonCallback("button34", bind( &VRGuiSetup::on_leap_startcalib_clicked, this) );
    setButtonCallback("button35", bind( &VRGuiSetup::on_leap_stopcalib_clicked, this) );

    setRadioButtonCallback("radiobutton6", bind( &VRGuiSetup::on_server_ct_toggled, this) );
    setRadioButtonCallback("radiobutton7", bind( &VRGuiSetup::on_server_ct_toggled, this) );
    setRadioButtonCallback("radiobutton9", bind( &VRGuiSetup::on_server_ct_toggled, this) );
    setRadioButtonCallback("radiobutton10", bind( &VRGuiSetup::on_netslave_edited, this));
    setRadioButtonCallback("radiobutton11", bind( &VRGuiSetup::on_netslave_edited, this));
    setRadioButtonCallback("radiobutton12", bind( &VRGuiSetup::on_netslave_edited, this));

#ifndef WITHOUT_MTOUCH
    setComboboxCallback("combobox12", bind( &VRGuiSetup::on_mt_device_changed, this) );
#endif
    setComboboxCallback("combobox13", bind( &VRGuiSetup::on_window_device_changed, this) );
    setComboboxCallback("combobox15", bind( &VRGuiSetup::on_window_msaa_changed, this) );
    setComboboxCallback("combobox18", bind( &VRGuiSetup::on_change_view_user, this) );
#ifndef WITHOUT_VIRTUOSE
    setComboboxCallback("combobox25", bind( &VRGuiSetup::on_change_haptic_type, this) );
#endif

#ifndef WITHOUT_VIRTUOSE
    fillStringListstore("liststore8", VRHaptic::getDevTypes() );
#endif
#ifndef WITHOUT_MTOUCH
    fillStringListstore("liststore11", VRMultiTouch::getDevices() );
#endif
    fillStringListstore("msaa_list", {"x0", "x2", "x4", "x8", "x16"});

    auto tree_view = VRGuiBuilder::get()->get_widget("treeview2");
    connect_signal<void>(tree_view, bind( &VRGuiSetup::on_treeview_select, this), "cursor_changed");
    connect_signal<bool, GdkEventButton*>(tree_view, bind( &VRGuiSetup::on_treeview_rightclick, this, PL::_1), "button_release_event");


    auto crt = VRGuiBuilder::get()->get_widget("cellrenderertext3");
    connect_signal<void, const char*, const char*>(crt, bind( &VRGuiSetup::on_name_edited, this, PL::_1, PL::_2), "edited");
    crt = VRGuiBuilder::get()->get_widget("cellrenderertext21");
    connect_signal<void, const char*, const char*>(crt, bind( &VRGuiSetup::on_server_edit, this, PL::_1, PL::_2), "edited");

    setToggleButtonCallback("checkbutton9", bind( &VRGuiSetup::on_toggle_view_invert, this));
    setToggleButtonCallback("checkbutton10", bind( &VRGuiSetup::on_toggle_view_active_stereo, this));
    setToggleButtonCallback("checkbutton7", bind( &VRGuiSetup::on_toggle_display_active, this));
    setToggleButtonCallback("checkbutton8", bind( &VRGuiSetup::on_toggle_display_stereo, this));
    setToggleButtonCallback("checkbutton11", bind( &VRGuiSetup::on_toggle_display_projection, this));
#ifndef WITHOUT_ART
    setToggleButtonCallback("checkbutton24", bind( &VRGuiSetup::on_toggle_art, this));
#endif
    setToggleButtonCallback("checkbutton26", bind( &VRGuiSetup::on_toggle_view_user, this));
    setToggleButtonCallback("checkbutton30", bind( &VRGuiSetup::on_toggle_view_mirror, this));
    setToggleButtonCallback("checkbutton4", bind( &VRGuiSetup::on_toggle_view_stats, this));
    setToggleButtonCallback("checkbutton37", bind( &VRGuiSetup::on_toggle_dev_cross, this));
#ifndef WITHOUT_VRPN
	setToggleButtonCallback("checkbutton25", bind( &VRGuiSetup::on_toggle_vrpn, this));
    setToggleButtonCallback("checkbutton39", bind( &VRGuiSetup::on_toggle_vrpn_test_server, this));
    setToggleButtonCallback("checkbutton40", bind( &VRGuiSetup::on_toggle_vrpn_verbose, this));
#endif
    setToggleButtonCallback("checkbutton29", bind( &VRGuiSetup::on_netslave_edited, this));
    setToggleButtonCallback("checkbutton41", bind( &VRGuiSetup::on_netslave_edited, this));
    setToggleButtonCallback("checkbutton42", bind( &VRGuiSetup::on_netslave_edited, this));

    //editor = shared_ptr<VRGuiEditor>( new VRGuiEditor("scrolledwindow12") );
    //connect_signal<void>(editor->getSourceBuffer(), bind( &VRGuiSetup::on_script_changed, this), "changed");

    // primitive list
    fillStringListstore("prim_list", VRPrimitive::getTypes());

    setWidgetSensitivity("table7", false);
    setWidgetSensitivity("table8", false);*/

    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("ui_change_main_tab", [&](OSG::VRGuiSignals::Options o) { if (o["tab"] == "Setup") updateSetup(); return true; }, true );
    mgr->addCallback("treeview_select", [&](OSG::VRGuiSignals::Options o) { if (o["treeview"] == "setup") on_treeview_select( o["node"] ); return true; }, true );
    mgr->addCallback("treeview_rename", [&](OSG::VRGuiSignals::Options o) { if (o["treeview"] == "setup") on_treeview_rename( o["node"], o["name"] ); return true; }, true );

    mgr->addCallback("ui_toggle_fotomode", [&](OSG::VRGuiSignals::Options o) { on_foto_clicked(toBool(o["active"])); return true; }, true );
    mgr->addCallback("ui_set_framesleep", [&](OSG::VRGuiSignals::Options o) { VRSceneManager::get()->setTargetFPS(toInt(o["fps"])); return true; }, true );
    mgr->addCallback("ui_set_framesleep", [&](OSG::VRGuiSignals::Options o) { VRSceneManager::get()->setTargetFPS(toInt(o["fps"])); return true; }, true );

    mgr->addCallback("setup_switch_setup", [&](OSG::VRGuiSignals::Options o) { on_setup_change_request(o["setup"]); return true; }, true );
    mgr->addCallback("change_setup", [&](OSG::VRGuiSignals::Options o) { on_setup_changed(); return true; }, true );
    mgr->addCallback("setup_new", [&](OSG::VRGuiSignals::Options o) { on_new_clicked(); return true; }, true );
    mgr->addCallback("setup_delete", [&](OSG::VRGuiSignals::Options o) { on_del_clicked(); return true; }, true );
    mgr->addCallback("setup_save", [&](OSG::VRGuiSignals::Options o) { on_save_clicked(); return true; }, true );
    mgr->addCallback("setup_saveas", [&](OSG::VRGuiSignals::Options o) { on_save_as_clicked(); return true; }, true );

    mgr->addCallback("setup_set_view_position", [&](OSG::VRGuiSignals::Options o) { on_pos_edit(Vec4d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]), toFloat(o["w"]))); return true; }, true );
    mgr->addCallback("setup_set_view_size", [&](OSG::VRGuiSignals::Options o) { on_view_size_edit(Vec2d(toFloat(o["x"]), toFloat(o["y"]))); return true; }, true );
    mgr->addCallback("setup_set_view_stereo", [&](OSG::VRGuiSignals::Options o) { on_toggle_display_stereo(toBool(o["active"])); return true; }, true );
    mgr->addCallback("setup_set_view_eye_separation", [&](OSG::VRGuiSignals::Options o) { on_eyesep_edit(toBool(o["active"])); return true; }, true );
    mgr->addCallback("setup_set_view_invert_eyes", [&](OSG::VRGuiSignals::Options o) { on_toggle_view_invert(toBool(o["active"])); return true; }, true );
    mgr->addCallback("setup_set_view_active_stereo", [&](OSG::VRGuiSignals::Options o) { on_toggle_view_active_stereo(toBool(o["active"])); return true; }, true );
    mgr->addCallback("setup_set_view_projection", [&](OSG::VRGuiSignals::Options o) { on_toggle_display_projection(toBool(o["active"])); return true; }, true );
    mgr->addCallback("setup_switch_view_user", [&](OSG::VRGuiSignals::Options o) { on_change_view_user(o["tracker"]); return true; }, true );
    mgr->addCallback("setup_set_view_proj_center", [&](OSG::VRGuiSignals::Options o) { on_proj_center_edit(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("setup_set_view_proj_user", [&](OSG::VRGuiSignals::Options o) { on_proj_user_edit(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("setup_set_view_proj_normal", [&](OSG::VRGuiSignals::Options o) { on_proj_normal_edit(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("setup_set_view_proj_up", [&](OSG::VRGuiSignals::Options o) { on_proj_up_edit(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("setup_set_view_proj_size", [&](OSG::VRGuiSignals::Options o) { on_proj_size_edit(Vec2d(toFloat(o["x"]), toFloat(o["y"]))); return true; }, true );
    mgr->addCallback("setup_set_view_proj_shear", [&](OSG::VRGuiSignals::Options o) { on_proj_shear_edit(Vec2d(toFloat(o["x"]), toFloat(o["y"]))); return true; }, true );
    mgr->addCallback("setup_set_view_proj_warp", [&](OSG::VRGuiSignals::Options o) { on_proj_warp_edit(Vec2d(toFloat(o["x"]), toFloat(o["y"]))); return true; }, true );
    mgr->addCallback("setup_set_view_mirror", [&](OSG::VRGuiSignals::Options o) { on_toggle_view_mirror(toBool(o["active"])); return true; }, true );
    mgr->addCallback("setup_set_view_mirror_position", [&](OSG::VRGuiSignals::Options o) { on_view_mirror_pos_edit(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("setup_set_view_mirror_normal", [&](OSG::VRGuiSignals::Options o) { on_view_mirror_norm_edit(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );

    mgr->addCallback("setup_set_win_active", [&](OSG::VRGuiSignals::Options o) { on_window_set_active(toBool(o["active"])); return true; }, true );
    mgr->addCallback("win_set_res", [&](OSG::VRGuiSignals::Options o) { on_window_size_changed(toInt(o["x"]), toInt(o["y"])); return true; }, true );
    mgr->addCallback("setup_switch_win_msaa", [&](OSG::VRGuiSignals::Options o) { on_window_msaa_changed(o["selection"]); return true; }, true );
    mgr->addCallback("setup_switch_win_mouse", [&](OSG::VRGuiSignals::Options o) { on_window_mouse_changed(o["selection"]); return true; }, true );
    mgr->addCallback("setup_switch_win_mtouch", [&](OSG::VRGuiSignals::Options o) { on_window_touch_changed(o["selection"]); return true; }, true );
    mgr->addCallback("setup_switch_win_keyb", [&](OSG::VRGuiSignals::Options o) { on_window_kboard_changed(o["selection"]); return true; }, true );
    mgr->addCallback("win_set_title", [&](OSG::VRGuiSignals::Options o) { on_window_title_changed(o["title"]); return true; }, true );
    mgr->addCallback("win_set_icon", [&](OSG::VRGuiSignals::Options o) { on_window_icon_changed(o["icon"]); return true; }, true );

    mgr->addCallback("win_set_NxNy", [&](OSG::VRGuiSignals::Options o) { on_servern_edit(toInt(o["x"]), toInt(o["y"])); return true; }, true );
    mgr->addCallback("win_set_serverID", [&](OSG::VRGuiSignals::Options o) { on_server_edit(toInt(o["x"]), toInt(o["y"]), o["sID"]); return true; }, true );
    mgr->addCallback("win_click_connect", [&](OSG::VRGuiSignals::Options o) { on_connect_mw_clicked(); return true; }, true );
    mgr->addCallback("setup_switch_win_conn_type", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_connection(o["selection"]); return true; }, true );

    mgr->addCallback("node_set_address", [&](OSG::VRGuiSignals::Options o) { on_netnode_address_edited(o["address"]); return true; }, true );
    mgr->addCallback("node_set_user", [&](OSG::VRGuiSignals::Options o) { on_netnode_user_edited(o["user"]); return true; }, true );
    mgr->addCallback("node_set_path", [&](OSG::VRGuiSignals::Options o) { on_netnode_path_edited(o["path"]); return true; }, true );
    mgr->addCallback("node_clicked_distribkey", [&](OSG::VRGuiSignals::Options o) { on_netnode_key_clicked(); return true; }, true );
    mgr->addCallback("node_clicked_stopslaves", [&](OSG::VRGuiSignals::Options o) { on_netnode_stopall_clicked(); return true; }, true );

    mgr->addCallback("slave_clicked_start", [&](OSG::VRGuiSignals::Options o) { on_netslave_start_clicked(); return true; }, true );
    mgr->addCallback("slave_toggle_autostart", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_autostart(toBool(o["state"])); return true; }, true );
    mgr->addCallback("slave_toggle_fullscreen", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_fullscreen(toBool(o["state"])); return true; }, true );
    mgr->addCallback("slave_toggle_activestereo", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_activestereo(toBool(o["state"])); return true; }, true );
    mgr->addCallback("slave_set_port", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_port(toInt(o["port"])); return true; }, true );
    mgr->addCallback("slave_set_screen", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_screen(o["screen"]); return true; }, true );
    mgr->addCallback("slave_set_delay", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_delay(toInt(o["delay"])); return true; }, true );
    mgr->addCallback("slave_set_geometry", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_geometry(o["geometry"]); return true; }, true );
    mgr->addCallback("setup_switch_slave_conn_type", [&](OSG::VRGuiSignals::Options o) { on_netslave_set_connection(o["selection"]); return true; }, true );

    mgr->addCallback("onSetupMenuAddNode", [&](OSG::VRGuiSignals::Options o) { on_menu_add_network_node(); return true; }, true );
    mgr->addCallback("onSetupMenuAddSlave", [&](OSG::VRGuiSignals::Options o) { on_menu_add_network_slave(o["node"]); return true; }, true );
    mgr->addCallback("onSetupMenuAddWindow", [&](OSG::VRGuiSignals::Options o) { on_menu_add_window(); return true; }, true );
    mgr->addCallback("onSetupMenuAddSlave", [&](OSG::VRGuiSignals::Options o) { on_menu_add_viewport(o["node"]); return true; }, true );
    mgr->addCallback("onSetupMenuDelete", [&](OSG::VRGuiSignals::Options o) { on_menu_delete(o["ID"]); return true; }, true );

    updateSetupCb = VRDeviceCb::create("update gui setup", bind(&VRGuiSetup::updateSetup, this) );

    guard = false;

    updateSetupList();
    updateSetup();
}

void VRGuiSetup::on_setup_change_request(string name) {
    if (guard) return;
    cout << "on_setup_changed\n";
    if (name == "") return;
    setupRequest = name;

    uiSignal("askUser", {{"msg1","Switch to setup '" + name + "' - this will quit PolyVR"},
                         {"msg2","Are you sure you want to switch to the " + name + " setup?"},
                         {"sig","change_setup"}});
}

void VRGuiSetup::on_setup_changed() {
    string name = setupRequest;
    string setupDirPath = setupDir();
    ofstream f(setupDirPath+".local"); f.write(name.c_str(), name.size()); f.close(); // remember setup
    string d = setupDirPath + name + ".xml";
    auto mgr = VRSetupManager::get();
    auto setup = mgr->load(name, d);
    updateSetup();

#ifndef WITHOUT_ART
    setup->getART()->getSignal_on_new_art_device()->add(updateSetupCb); // TODO: where to put this? NOT in updateSetup() !!!
#endif

    auto fkt = VRUpdateCb::create("setup_induced_shutdown", bind(&PolyVR::shutdown));
    VRSceneManager::get()->queueJob(fkt, 0, 100); // TODO: this blocks everything..
}

void VRGuiSetup::on_window_set_active(bool b) {
    if (guard || !window) return;
    window->setActive(b);
}

void VRGuiSetup::on_window_mouse_changed(string s) {
    if (guard || !window) return;
    auto dev = VRSetup::getCurrent()->getDevice(s);
    window->setMouse( dynamic_pointer_cast<VRMouse>(dev) );
}

void VRGuiSetup::on_window_touch_changed(string s) {
    if (guard || !window) return;
#ifndef WITHOUT_MTOUCH
    auto dev = VRSetup::getCurrent()->getDevice(s);
    window->setMultitouch( dynamic_pointer_cast<VRMultiTouch>(dev) );
#endif
}

void VRGuiSetup::on_window_kboard_changed(string s) {
    if (guard || !window) return;
    auto dev = VRSetup::getCurrent()->getDevice(s);
    window->setKeyboard( dynamic_pointer_cast<VRKeyboard>(dev) );
}

void VRGuiSetup::on_window_msaa_changed(string s) {
    if (guard || !window) return;
    window->setMSAA( s );
}

void VRGuiSetup::on_window_size_changed(int w, int h) {
    if (guard || !window) return;
    window->resize( w, h );
}

void VRGuiSetup::on_window_title_changed(string s) {
    if (guard || !window) return;
    window->setTitle( s );
    updateObjectData();
}

void VRGuiSetup::on_window_icon_changed(string s) {
    if (guard || !window) return;
    window->setIcon( s );
    updateObjectData();
}


void VRGuiSetup::updateStatus() {
    //if (mwindow) setLabel("win_state", mwindow->getStateString());
}

bool VRGuiSetup::updateSetup() {
    cout << " - - - - updateSetup" << endl;
    uiSignal("on_setup_tree_clear");

    uiSignal("on_setup_tree_append", {{ "ID","SecNetwork" }, { "label","Network" }, { "type","section" }, { "parent","" }});
    uiSignal("on_setup_tree_append", {{ "ID","SecDisplays" }, { "label","Displays" }, { "type","section" }, { "parent","" }});
    uiSignal("on_setup_tree_append", {{ "ID","SecDevices" }, { "label","Devices" }, { "type","section" }, { "parent","" }});
    uiSignal("on_setup_tree_append", {{ "ID","SecART" }, { "label","ART" }, { "type","section" }, { "parent","" }});
    uiSignal("on_setup_tree_append", {{ "ID","SecVRPN" }, { "label","VRPN" }, { "type","section" }, { "parent","" }});
    uiSignal("on_setup_tree_append", {{ "ID","SecScripts" }, { "label","Scripts" }, { "type","section" }, { "parent","" }});

    /*GtkTreeIter row;
    auto user_list = (GtkListStore*)VRGuiBuilder::get()->get_object("user_list");
    gtk_list_store_clear(user_list);
    gtk_list_store_append(user_list, &row);
    gtk_list_store_set(user_list, &row, 0, "None", -1);
    gtk_list_store_set(user_list, &row, 1, 0, -1);*/

    /*auto mouse_list = (GtkListStore*)VRGuiBuilder::get()->get_object("mouse_list");
    gtk_list_store_clear(mouse_list);
    gtk_list_store_append(mouse_list, &row);
    gtk_list_store_set (mouse_list, &row, 0, "None", -1);*/

    auto setup = VRSetup::getCurrent();
    //setLabel("label13", "VR Setup: NONE");
    if (!setup) return true;
    //setLabel("label13", "VR Setup: " + setup->getName());

    vector<string> mouseList = {"none"};
    vector<string> mtouchList = {"none"};
    vector<string> keyboardList = {"none"};

    for (auto ditr : setup->getDevices()) {
        VRDevicePtr dev = ditr.second;
        string devID = dev->getType() + "$" + ditr.first;
        uiSignal("on_setup_tree_append", {{ "ID",devID }, { "label",ditr.first }, { "type",dev->getType() }, { "parent","SecDevices" }});

        if (dev->getType() == "mouse") mouseList.push_back(ditr.first);
        if (dev->getType() == "multitouch") mtouchList.push_back(ditr.first);
        if (dev->getType() == "keyboard") keyboardList.push_back(ditr.first);
    }

    vector<string> devTypesList = setup->getDeviceTypes();

    uiSignal("updateMouseList", {{"list", toString(mouseList)}});
    uiSignal("updateMTouchList", {{"list", toString(mtouchList)}});
    uiSignal("updateKeyboardList", {{"list", toString(keyboardList)}});
    uiSignal("updateDevTypesList", {{"list", toString(devTypesList)}});

    for (auto node : setup->getNetwork()->getData() ) {
        string nodeID = "node$"+node->getName();
        uiSignal("on_setup_tree_append", {{ "ID",nodeID }, { "label",node->getName() }, { "type","node" }, { "parent","SecNetwork" }});
        for (auto slave : node->getData() ) {
            string slaveID = "slave$" + slave->getName();
            uiSignal("on_setup_tree_append", {{ "ID",slaveID }, { "label",slave->getName() }, { "type","slave" }, { "parent",nodeID }});
        }
    }

    for (auto win : setup->getWindows()) {
        VRWindow* w = win.second.get();
        string name = win.first;
        string winID = "window$"+name;
        uiSignal("on_setup_tree_append", {{ "ID",winID }, { "label",name }, { "type","window" }, { "parent","SecDisplays" }});

        // add viewports
        vector<VRViewPtr> views = w->getViews();
        for (uint i=0; i<views.size(); i++) {
            VRViewPtr v = views[i];
            string vname = name;
            string viewID = "view$"+vname;
            uiSignal("on_setup_tree_append", {{ "ID",viewID }, { "label",vname }, { "type","view" }, { "parent",winID }});
        }
    }

#ifndef WITHOUT_VRPN
    // VRPN
    for (int ID : setup->getVRPN()->getVRPNTrackerIDs() ) {
        VRPN_device* t = setup->getVRPN()->getVRPNTracker(ID).get();
        string vrpnID = "vrpn_tracker$"+toString(ID);
        uiSignal("on_setup_tree_append", {{ "ID",vrpnID }, { "label",t->getName() }, { "type","vrpn_tracker" }, { "parent","SecVRPN" }});
    }
#endif

#ifndef WITHOUT_ART
    // ART
    vector<string> viewTrackers;
    for (int ID : setup->getART()->getARTDevices() ) {
        ART_devicePtr dev = setup->getART()->getARTDevice(ID);

        string name = dev->getName();
        if (dev->dev) name = dev->dev->getName();
        else if (dev->ent) name = dev->ent->getName();
        string artID = "art_device$"+toString(ID);
        uiSignal("on_setup_tree_append", {{ "ID",artID }, { "label",name }, { "type","art_device" }, { "parent","SecART" }});

        if (dev->ent) viewTrackers.push_back( dev->ent->getName() );
    }
    uiSignal("updateViewTrackersList", {{"trackers", toString(viewTrackers)}});
#endif

    /*for (auto s : setup->getScripts()) {
        auto script = s.second.get();
        GtkTreeIter itr;
        gtk_tree_store_append(tree_store, &itr, &scripts_itr);
        setTreeRow(tree_store, &itr, script->getName().c_str(), "script", (gpointer)script);
    }*/

    //on_treeview_select();
    uiSignal("on_setup_tree_expand");
    return true;
}

bool getSetupEntries(string dir, string& local, string& def) {
    ifstream f1(dir+".local");
    ifstream f2(dir+".default");
    if (!f1.good() && !f2.good()) return 0;

    if (f1.good()) getline(f1, local);
    else           getline(f2, local);
    if (f2.good()) getline(f2, def);

    if (f1.good()) f1.close();
    if (f2.good()) f2.close();
    return 1;
}

void VRGuiSetup::updateSetupList() {
    string dir = setupDir();
    if (!exists(dir)) { cerr << "Error: no local directory setup\n"; return; }

    string local, defaul;
    if (!getSetupEntries(dir, local, defaul)) { cerr << "Error: no setup file found\n"; return; }

    auto splitFileName = [&](string& name, string& ending) {
        int N = name.size();
        if (N < 6) return false;

        ending = name.substr(N-4, N-1);
        name = name.substr(0,N-4);
        if (ending != ".xml") return false;
        return true;
    };

    string ending;
    vector<string> setups;
    for (string name : openFolder(dir)) { // update list
        if (!splitFileName(name, ending)) continue;
        setups.push_back(name);
    }
    uiSignal("updateSetupsList", {{"setups", toString(setups)}});

    int active = -1;
    auto setActive = [&](string n) {
        int i = 0;
        for(string name : openFolder(dir)) {
            if (n == name) { active = i; break; }
            if (!splitFileName(name, ending)) continue;
            if (n == name) { active = i; break; }
            i++;
        }
    };

    setActive(local);
    if (active < 0) {
        cout << "Setup " << local << " not found. Load default: " << defaul << endl;
        setActive(defaul);
    }
    uiSignal("setCurrentSetup", {{"setup", toString(active)}});
}

OSG_END_NAMESPACE;
