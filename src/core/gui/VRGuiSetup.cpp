#include <gtk/gtk.h>
#include "VRGuiSetup.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
#include "VRGuiFile.h"
#include "VRGuiSignals.h"
#include "VRGuiContextMenu.h"
#include "PolyVR.h"

#include "core/scripting/VRScript.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRNetwork.h"
#include "core/setup/windows/VRWindow.h"
#include "core/setup/windows/VRMultiWindow.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/setup/devices/VRFlystick.h"
#ifndef WITHOUT_VIRTUOSE
#include "core/setup/devices/VRHaptic.h"
#endif
#include "core/setup/devices/VRServer.h"
#include "core/setup/devices/VRMouse.h"
#ifndef WITHOUT_MTOUCH
#include "core/setup/devices/VRMultiTouch.h"
#endif
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/utils/VRManager.cpp"
#include "addons/LeapMotion/VRLeap.h"


#include "core/objects/VRCamera.h"

#include "wrapper/VRGuiTreeView.h"
#include "wrapper/VRGuiCombobox.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

// VRGuiSetup_ModelColumns
//  add(name); add(type); add(obj);

// VRGuiSetup_ServerColumns
//  add(y); add(x); add(server);

// VRGuiSetup_UserColumns
//  add(name); add(user);

void VRGuiSetup::closeAllExpander() {
    setWidgetVisibility("expander3", false, true);
    setWidgetVisibility("expander4", false, true);
    setWidgetVisibility("expander5", false, true);
    setWidgetVisibility("expander6", false, true);
    setWidgetVisibility("expander7", false, true);
    setWidgetVisibility("expander8", false, true);
    setWidgetVisibility("expander20", false, true);
    setWidgetVisibility("expander21", false, true);
    setWidgetVisibility("expander22", false, true);
    setWidgetVisibility("expander23", false, true);
    setWidgetVisibility("expander24", false, true);
    setWidgetVisibility("expander25", false, true);
    setWidgetVisibility("expander26", false, true);
    setWidgetVisibility("expander28", false, true);
    setWidgetVisibility("expander29", false, true);
    setWidgetVisibility("expander30", false, true);
    setWidgetVisibility("expander31", false, true);
}

void VRGuiSetup::updateObjectData() {
    bool device = false;
    guard = true;

    current_scene = VRScene::getCurrent();

    if (selected_type == "window") {
        setWidgetVisibility("expander3", true, true);

        VRWindow* win = (VRWindow*)selected_object;
        setToggleButton("checkbutton7", win->isActive());

        if (win->hasType(0)) { // multiwindow
            setWidgetVisibility("expander24", true, true);
            VRMultiWindow* mwin = (VRMultiWindow*)win;
            int nx, ny;
            nx = mwin->getNXTiles();
            ny = mwin->getNYTiles();
            stringstream ssx, ssy;
            ssx << nx;
            ssy << ny;
            setTextEntry("entry33", ssx.str());
            setTextEntry("entry34", ssy.str());
            setLabel("win_state", mwin->getStateString());
            string ct = mwin->getConnectionType();
            if (ct == "Multicast") setToggleButton("radiobutton6", 1);
            if (ct == "SockPipeline") setToggleButton("radiobutton7", 1);
            if (ct == "StreamSock") setToggleButton("radiobutton9", 1);

            //TODO: clear server array && add entry for each nx * ny
            auto servers = (GtkListStore*)VRGuiBuilder::get()->get_object("serverlist");
            gtk_list_store_clear(servers);
            for (int y=0; y<ny; y++) {
                for (int x=0; x<nx; x++) {
                    GtkTreeIter row;
                    gtk_list_store_append(servers, &row);
                    gtk_list_store_set(servers, &row, 0, y, -1);
                    gtk_list_store_set(servers, &row, 1, x, -1);
                    gtk_list_store_set(servers, &row, 2, mwin->getServer(x,y).c_str(), -1);
                }
            }
        }

        if (win->hasType(1)) setWidgetVisibility("expander23", true, true);
        if (win->hasType(2)) setWidgetVisibility("expander22", true, true); // GTK

        // mouse
        string name = "None";
        if (win->getMouse()) name = win->getMouse()->getName();
#ifndef WITHOUT_MTOUCH
        if (win->getMultitouch()) name = win->getMultitouch()->getName();
#endif
        setCombobox("combobox13", getListStorePos("mouse_list", name));
        setCombobox("combobox15", getListStorePos("msaa_list", win->getMSAA()));
    }

    if (selected_type == "view") {
        setWidgetVisibility("expander8", true, true);

        VRView* view = (VRView*)selected_object;

        Vec4d p = view->getPosition();
        setTextEntry("entry52", toString(p[0]).c_str());
        setTextEntry("entry53", toString(p[2]).c_str());
        setTextEntry("entry56", toString(p[1]).c_str());
        setTextEntry("entry57", toString(p[3]).c_str());

        setToggleButton("checkbutton8", view->isStereo());
        setToggleButton("checkbutton9", view->eyesInverted());
        setToggleButton("checkbutton10", view->activeStereo());
        setToggleButton("checkbutton11", view->isProjection());
        setToggleButton("checkbutton30", view->getMirror());

        setTextEntry("entry12", toString(view->getEyeSeparation()).c_str());
        int uID = getListStorePos("user_list", view->getUser()->getName());
        setCombobox("combobox18", uID);
        setToggleButton("checkbutton26", uID != -1);

        userEntry.set(view->getProjectionUser());
        centerEntry.set(view->getProjectionCenter());
        normalEntry.set(view->getProjectionNormal());
        upEntry.set(view->getProjectionUp());
        sizeEntry.set(view->getProjectionSize());
        shearEntry.set(view->getProjectionShear());
        warpEntry.set(view->getProjectionWarp());
        vsizeEntry.set(Vec2d(view->getSize()));
        mirrorPosEntry.set(view->getMirrorPos());
        mirrorNormEntry.set(view->getMirrorNorm());
    }

    if (selected_type == "vrpn_device") {
        setWidgetVisibility("expander4", true, true);
        setWidgetVisibility("expander7", true, true);
        device = true;
        VRPN_device* t = (VRPN_device*)selected_object;
        setTextEntry("entry50", t->address);
    }

    if (selected_type == "vrpn_tracker") {
        setWidgetVisibility("expander4", true, true);
        setWidgetVisibility("expander7", true, true);
        VRPN_device* t = (VRPN_device*)selected_object;
        setTextEntry("entry50", t->address);
        tVRPNAxisEntry.set(t->translate_axis);
        rVRPNAxisEntry.set(t->rotation_axis);
    }

    if (selected_type == "art_device") {
        setWidgetVisibility("expander5", true, true);
        ART_device* t = (ART_device*)selected_object;
        setTextEntry("entry40", toString(t->ID));
    }

#ifndef WITHOUT_VIRTUOSE
    if (selected_type == "haptic") {
        setWidgetVisibility("expander20", true, true);
        device = true;
        VRHaptic* t = (VRHaptic*)selected_object;
        setTextEntry("entry8", t->getIP());
        setCombobox("combobox25", getListStorePos("liststore8", t->getType()) );
        setLabel("label64", t->getDeamonState());
        setLabel("label66", t->getDeviceState());
    }
#endif

#ifndef WITHOUT_MTOUCH
    if (selected_type == "multitouch") {
        setWidgetVisibility("expander30", true, true);
        VRMultiTouch* t = (VRMultiTouch*)selected_object;
        setCombobox("combobox12", getListStorePos("liststore11", t->getDevice()) );
    }
#endif

    if (selected_type == "leap") {
        setWidgetVisibility("expander31", true, true);
        VRLeap* t = (VRLeap*)selected_object;
        setTextEntry("entry28", t->getAddress());
        setLabel("label157", t->getConnectionStatus());
        setLabel("label159", t->getSerial());
        auto p = t->getPose();
        leapPosEntry.set(p->pos());
        leapUpEntry.set(p->up());
        leapDirEntry.set(p->dir());
    }

    if (selected_type == "mouse") { device = true; }
    if (selected_type == "multitouch") { device = true; }
    if (selected_type == "keyboard") { device = true; }
    if (selected_type == "server") { device = true; }
    if (selected_type == "flystick") { device = true; }
    if (selected_type == "leap") { device = true; }

    auto setup = current_setup.lock();
#ifndef WITHOUT_VRPN
    if (selected_type == "vrpn_device" || selected_type == "vrpn_tracker") {
        if (setup) {
            setTextEntry("entry13", toString(setup->getVRPNPort()));
            setToggleButton("checkbutton25", setup->getVRPNActive());
        }
    }
#endif

    if (selected_type == "section" && setup) {
#ifndef WITHOUT_ART
        if (selected_name == "ART") {
            setWidgetVisibility("expander6", true, true);
            setTextEntry("entry39", toString(setup->getARTPort()));
            setToggleButton("checkbutton24", setup->getARTActive());

            artOffset.set(setup->getARTOffset());
            artAxis.set(Vec3d(setup->getARTAxis()));
        }
#endif

#ifndef WITHOUT_VRPN
        if (selected_name == "VRPN") {
            setWidgetVisibility("expander7", true, true);
            setTextEntry("entry13", toString(setup->getVRPNPort()));
            setToggleButton("checkbutton25", setup->getVRPNActive());
        }
#endif

        if (selected_name == "Displays") {
            setWidgetVisibility("expander28", true, true);
            Vec3d o = setup->getDisplaysOffset();
            setTextEntry("entry29", toString(o[0]));
            setTextEntry("entry30", toString(o[1]));
            setTextEntry("entry31", toString(o[2]));
        }
    }

    if (device) {
        VRDevice* dev = (VRDevice*)selected_object;
        VRIntersection ins = dev->getLastIntersection();

        setWidgetVisibility("expander21", true, true);
        setLabel("label93", dev->getName());
        if (setup) fillStringListstore("dev_types_list", setup->getDeviceTypes());
        setCombobox("combobox26", getListStorePos("dev_types_list", dev->getType()) );
        string hobj = ins.hit ? ins.name : "NONE";
        setLabel("label110", hobj);
        setLabel("label111", toString(ins.point));
        setLabel("label112", toString(ins.texel));
        setToggleButton("checkbutton37", dev->getCross()->isVisible());
    }

    if (selected_type == "node") {
        setWidgetVisibility("expander25", true, true);
        VRNetworkNode* n = (VRNetworkNode*)selected_object;
        setTextEntry("entry15", n->getAddress());
        setTextEntry("entry20", n->getUser());
        setTextEntry("entry32", n->getSlavePath());
        setLabel("label130", n->getStatNode());
        setLabel("label129", n->getStatSSH());
        setLabel("label126", n->getStatSSHkey());
        setLabel("label161", n->getStatPath());
    }

    if (selected_type == "slave") {
        setWidgetVisibility("expander26", true, true);
        VRNetworkSlave* n = (VRNetworkSlave*)selected_object;
        setLabel("label138", n->getConnectionIdentifier());
        setLabel("label132", n->getStatMulticast());
        setLabel("label136", n->getStat());
        setToggleButton("checkbutton29", n->getFullscreen());
        setToggleButton("checkbutton41", n->getActiveStereo());
        setToggleButton("checkbutton42", n->getAutostart());
        setTextEntry("entry19", n->getDisplay());
        setTextEntry("entry22", toString(n->getPort()));
        setTextEntry("entry37", toString(n->getStartupDelay()));
        setTextEntry("entry38", toString(n->getGeometry()));

        string ct = n->getConnectionType();
        if (ct == "Multicast") setToggleButton("radiobutton10", 1);
        if (ct == "SockPipeline") setToggleButton("radiobutton11", 1);
        if (ct == "StreamSock") setToggleButton("radiobutton12", 1);
    }

    if (selected_type == "script") {
        setWidgetVisibility("expander29", true, true);
        VRScript* script = (VRScript*)selected_object;
        editor->setCore(script->getHead() + script->getCore());
        auto trigs = script->getTriggers();
        setToggleButton("radiotoolbutton1", true);
        if (trigs.size() > 0) {
            auto trig = *trigs.begin();
            if (trig) {
                if (trig->trigger == "on_timeout") setToggleButton("radiotoolbutton2", true);
                else setToggleButton("radiotoolbutton3", true);
            }
        }
    }

    guard = false;
}

//TODO:
// - win->init in a thread

string VRGuiSetup::setupDir() { return VRSceneManager::get()->getOriginalWorkdir()+"/setup/"; }

// --------------------------
// ---------Callbacks--------
// --------------------------

// toolbuttons

void VRGuiSetup::on_new_clicked() {
    guard = true;
    current_setup = VRSetupManager::get()->newSetup();

    on_save_clicked();
    // remember setup
    if (auto s = current_setup.lock()) {
        string name = s->getName();
        ofstream f("setup/.local"); f.write(name.c_str(), name.size()); f.close();
    }

    updateSetupList();
    updateSetup();
    guard = false;
}

void VRGuiSetup::on_foto_clicked() {
    if (auto s = current_setup.lock()) {
        bool b = getToggleToolButtonState("toolbutton19");
        s->setFotoMode(b);
    }
}

void VRGuiSetup::on_del_clicked() { //TODO, should delete setup
    string msg1 = "Delete setup ";
    if (!askUser(msg1, "Are you sure you want to delete this script?")) return;
    return;

    VRGuiTreeView tree_view("treeview2");
    string name = tree_view.getSelectedStringValue(0);
    tree_view.removeSelected();
    if (auto s = current_setup.lock()) s->removeWindow(name);

    /*Gtk::ToolButton* b;
    VRGuiBuilder::get()->get_widget("toolbutton9", b);
    b->set_sensitive(false);
    VRGuiBuilder::get()->get_widget("toolbutton8", b);
    b->set_sensitive(false);*/
}

void VRGuiSetup::on_save_clicked() {
    if (auto s = current_setup.lock()) {
        s->save(setupDir() + s->getName() + ".xml");
        setWidgetSensitivity("toolbutton12", false);
    }
}

void VRGuiSetup::on_diag_save_as_clicked() {
    guard = true;
    string path = VRGuiFile::getPath();
    if (path == "") return;

    if (auto s = current_setup.lock()) {
        s->save(path);
        string name = s->getName();
        ofstream f("setup/.local"); f.write(name.c_str(), name.size()); f.close();
        setWidgetSensitivity("toolbutton12", false);
    }

    updateSetupList();
    updateSetup();
    guard = false;
}

void VRGuiSetup::on_save_as_clicked() {
    VRGuiFile::setCallbacks( bind(&VRGuiSetup::on_diag_save_as_clicked, this) );
    VRGuiFile::gotoPath( setupDir() );
    VRGuiFile::setFile( "mySetup.pvr" );
    VRGuiFile::clearFilter();
    VRGuiFile::open( "Save As..", GTK_FILE_CHOOSER_ACTION_SAVE, "Save Setup As.." );
}

// setup list
void VRGuiSetup::on_treeview_select() {
    closeAllExpander();

    VRGuiTreeView tree_view("treeview2");

    if (!tree_view.hasSelection()) {
        selected_type = "";
        updateObjectData();
        return;
    }

    selected_object = 0;
    selected_name = tree_view.getSelectedStringValue(0);
    selected_type = tree_view.getSelectedStringValue(1);
    selected_object = tree_view.getSelectedValue(2);

    GtkTreeIter parent;
    if (tree_view.getSelectedParent(parent)) selected_object_parent = tree_view.getValue(&parent, 2);

    window = mwindow = 0;
    if (selected_type == "window") {
        window = (VRWindow*)selected_object;
        mwindow = (VRMultiWindow*)selected_object;
    }

    updateObjectData();
}

void VRGuiSetup::on_name_edited(const char* path, const char* new_name) {
    // VRGuiSetup_ModelColumns cols;
    //  add(name); add(type); add(obj);
    VRGuiTreeView tree_view("treeview2");
    string name  = tree_view.getSelectedStringValue(0);
    string type  = tree_view.getSelectedStringValue(1);
    gpointer obj = tree_view.getSelectedValue(2);

    // update key in map
    if (auto s = current_setup.lock()) {
        if (type == "window") s->changeWindowName(name, new_name);
#ifndef WITHOUT_VRPN
        if (type == "vrpn_tracker") s->changeVRPNDeviceName(((VRPN_device*)obj)->ptr(), new_name);
#endif
        if (type == "node") ((VRNetworkNode*)obj)->setName(new_name);
        if (type == "slave") ((VRNetworkSlave*)obj)->setName(new_name);
    }

    tree_view.setSelectedStringValue(0, name);
    updateSetup();
}

bool VRGuiSetup::on_treeview_rightclick(GdkEventButton* event) {
    if (event->type != GDK_BUTTON_RELEASE) return false;
    if (event->button-1 != 2) return false;

    //open contextmenu
    menu->popup("SetupMenu", event);
	return true;
}


void VRGuiSetup::on_menu_delete() {
    auto setup = current_setup.lock();
    if (!setup) return;

    if (selected_type == "window") {
        VRWindow* win = (VRWindow*)selected_object;
        setup->removeWindow(win->getName());
    }

    if (selected_type == "view") {
        VRView* view = (VRView*)selected_object;
        setup->removeView(view->getID());
        VRWindow* win = (VRWindow*)selected_object_parent;
        win->remView(view->ptr());
    }

#ifndef WITHOUT_VRPN
    if (selected_type == "vrpn_tracker") {
        VRPN_device* t = (VRPN_device*)selected_object;
        setup->delVRPNTracker(t->ptr());
    }
#endif

    if (selected_type == "art_device") {
        ;
    }

    if (selected_type == "node") {
        auto node = (VRNetworkNode*)selected_object;
        setup->getNetwork()->rem( node->getName() );
    }

    updateSetup();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_menu_add_window() {
    auto setup = current_setup.lock();
    if (!setup) return;
    VRWindowPtr win = setup->addMultiWindow("Display");
    win->setActive(true);
    if ( VRScene::getCurrent() ) win->setContent(true);

    updateSetup();
    selected_object = win.get();
    selected_type = "window";
    on_menu_add_viewport();
}

void VRGuiSetup::on_menu_add_viewport() {
    auto setup = current_setup.lock();
    if (!setup) return;
    if (selected_type != "window") return;

    VRWindow* win = (VRWindow*)selected_object;
    int v = setup->addView(win->getBaseName());
    auto view = setup->getView(v);
    win->addView(view);

    if (auto scene = current_scene.lock()) {
        setup->setViewRoot(scene->getRoot(), v);
        view->setCamera( scene->getActiveCamera() );
        view->setBackground( scene->getBackground() );
    }

    updateSetup();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_menu_add_vrpn_tracker() {
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->addVRPNTracker(0, "Tracker0@localhost", Vec3d(0,0,0), 1);
    //setup->addVRPNTracker(0, "LeapTracker@tcp://141.3.151.136", Vec3d(0,0,0), 1);

    updateSetup();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

template<class T>
void VRGuiSetup::on_menu_add_device() {
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->addDevice(T::create());
    updateSetup();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_menu_add_network_node() {
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->getNetwork()->add("Node");
    updateSetup();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_menu_add_network_slave() {
    if (selected_type != "node") {
        notifyUser("Please select a network node to add a slave.", "(Right click the node to add the slave to)");
        return;
    }
    VRNetworkNode* n = (VRNetworkNode*)selected_object;
    n->add("Slave");
    updateSetup();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_menu_add_script() {
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->addScript("script");
    updateSetup();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

// window options

void VRGuiSetup::on_toggle_display_active() {
    bool b = getCheckButtonState("checkbutton7");
    VRGuiTreeView tree_view("treeview2");
    tree_view.setSensitivity(b);
    if (guard) return;

    if (selected_type != "window") return;
    VRWindow* win = (VRWindow*)selected_object;

    //cout << "\nToggleActive " << name << " " << b << endl;
    win->setActive(b);

    // TODO
    //string bg = "#FFFFFF";
    //if (!b) bg = "#FFDDDD";
    //VRGuiTreeView tree_view("treeview2");
    //auto tree_store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("setupTree");
    //setTreeRow(tree_store, selected_row, win->getName().c_str(), "window", (gpointer)win, "#000000", bg);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_servern_edit() {
    if (guard) return;
    if (selected_type != "window") return;

    string nx = getTextEntry("entry33");
    string ny = getTextEntry("entry34");

    VRMultiWindow* mwin = (VRMultiWindow*)selected_object;
    mwin->setNTiles(toInt(nx.c_str()), toInt(ny.c_str()));
    updateObjectData();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_server_ct_toggled() {
    if (guard) return;
    if (selected_type != "window") return;

    string ct = "StreamSock";
    if ( getRadioButtonState("radiobutton6") ) ct = "Multicast";
    if ( getRadioButtonState("radiobutton7") ) ct = "SockPipeline";

    VRMultiWindow* mwin = (VRMultiWindow*)selected_object;
    mwin->setConnectionType(ct);
    updateObjectData();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_server_edit(const char* path, const char* server) {
    VRGuiTreeView tree_view("treeview1");
    if (!tree_view.hasSelection()) return;

    // get selected window

    // VRGuiSetup_ServerColumns cols;
    //  add(y); add(x); add(server);
    int x = tree_view.getSelectedIntValue(0);
    int y = tree_view.getSelectedIntValue(1);
    tree_view.setSelectedStringValue(2, server);

    VRMultiWindow* mwin = (VRMultiWindow*)selected_object;
    mwin->setServer(x,y,server);
    mwin->reset();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_connect_mw_clicked() {
    VRMultiWindow* mwin = (VRMultiWindow*)selected_object;
    if (mwin == 0) return;
    mwin->reset();
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

// view options

void VRGuiSetup::on_toggle_view_stats() {
    if (guard) return;
    if (selected_type != "view") return;

    bool b = getCheckButtonState("checkbutton4");
    VRView* view = (VRView*)selected_object;
    view->showStats(b);
}

void VRGuiSetup::on_toggle_display_stereo() {
    bool b = getCheckButtonState("checkbutton8");
    VRGuiWidget("table7").setSensitivity(b);
    if (guard) return;

    if (selected_type != "view") return;
    VRView* view = (VRView*)selected_object;

    view->setStereo(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_toggle_display_projection() {
    bool b = getCheckButtonState("checkbutton11");
    VRGuiWidget("table8").setSensitivity(b);
    if (guard) return;

    if (selected_type != "view") return;
    VRView* view = (VRView*)selected_object;

    view->setProjection(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_toggle_view_invert() {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;

    bool b = getCheckButtonState("checkbutton9");
    view->swapEyes(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_toggle_view_active_stereo() {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;

    bool b = getCheckButtonState("checkbutton10");
    view->setActiveStereo(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_pos_edit() {
    if (guard) return;
    if (selected_type != "view") return;

    string x0 = getTextEntry("entry52");
    string x1 = getTextEntry("entry53");
    string y0 = getTextEntry("entry56");
    string y1 = getTextEntry("entry57");
    Vec4d pos = toValue<Vec4d>(x0 + " " + y0 + " " + x1 + " " + y1);

    VRView* view = (VRView*)selected_object;
    view->setPosition(pos);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_eyesep_edit() {
    if (guard) return;
    if (selected_type != "view") return;

    string es = getTextEntry("entry12");

    VRView* view = (VRView*)selected_object;
    view->setStereoEyeSeparation(toFloat(es));
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_toggle_view_user() {
    if (guard) return;
    if (selected_type != "view") return;

    bool b = getCheckButtonState("checkbutton26");
    VRView* view = (VRView*)selected_object;
    if (!b) view->setUser(0);
    else {
        // VRGuiSetup_UserColumns cols; //  add(name); add(user);
        VRGuiCombobox combobox("combobox18");
        auto U = combobox.getSelectedValue(1);
        if (U) {
            VRTransformPtr u = ( (VRTransform*)U )->ptr();
            view->setUser(u);
        }
    }
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_toggle_view_mirror() {
    if (guard) return;
    if (selected_type != "view") return;

    bool b = getCheckButtonState("checkbutton30");
    VRView* view = (VRView*)selected_object;
    view->setMirror(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_view_mirror_pos_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setMirrorPos(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_view_mirror_norm_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setMirrorNorm(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_change_view_user() {
    if (guard) return;
    if (selected_type != "view") return;

    // VRGuiSetup_UserColumns cols; //  add(name); add(user);
    VRGuiCombobox combobox("combobox18");
    auto U = combobox.getSelectedValue(1);
    if (!U) return;
    VRTransformPtr u = ( (VRTransform*)U )->ptr();

    VRView* view = (VRView*)selected_object;
    view->setUser(u);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_view_size_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setSize(Vec2i(v));
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_proj_user_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionUser(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_proj_center_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionCenter(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_proj_normal_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionNormal(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_proj_up_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionUp(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_proj_size_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionSize(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_proj_shear_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionShear(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_proj_warp_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionWarp(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_vrpn_trans_axis_edit(Vec3d v) {
    if (guard) return;

    if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    t->setTranslationAxis(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_vrpn_rot_axis_edit(Vec3d v) {
    if (guard) return;

    if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    t->setRotationAxis(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

// tracker

#ifndef WITHOUT_ART
void VRGuiSetup::on_toggle_art() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    bool b = getCheckButtonState("checkbutton24");
    setup->setARTActive(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_toggle_vrpn() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    bool b = getCheckButtonState("checkbutton25");
    setup->setVRPNActive(b);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

#ifndef WITHOUT_ART
void VRGuiSetup::on_art_edit_port() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    int p = toInt(getTextEntry("entry39"));
    setup->setARTPort(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

void VRGuiSetup::on_displays_edit_offset() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    float ox = toFloat(getTextEntry("entry29"));
    float oy = toFloat(getTextEntry("entry30"));
    float oz = toFloat(getTextEntry("entry31"));
    setup->setDisplaysOffset(Vec3d(ox,oy,oz));
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

#ifndef WITHOUT_ART
void VRGuiSetup::on_art_edit_offset(Vec3d v) {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->setARTOffset(v);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_art_edit_axis(Vec3d v) {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->setARTAxis(Vec3i(round(v[0]), round(v[1]), round(v[2])));
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_art_edit_id() {
    if (guard) return;
    int id = toInt(getTextEntry("entry40"));
    ART_device* dev = (ART_device*)selected_object;
    dev->ID = id;
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_vrpn_edit_port() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    int p = toInt(getTextEntry("entry13"));
    setup->setVRPNPort(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_edit_VRPN_tracker_address() {
    if (guard) return;
    if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    string txt = getTextEntry("entry50");
    t->setAddress(txt);

    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

void VRGuiSetup::on_netnode_edited() {
    if (guard) return;
    VRNetworkNode* n = (VRNetworkNode*)selected_object;
    n->set(getTextEntry("entry15"), getTextEntry("entry20"), getTextEntry("entry32"));
    VRGuiWidget("toolbutton12").setSensitivity(true);
    updateObjectData();
}

void VRGuiSetup::on_netnode_key_clicked() {
    if (guard) return;
    VRNetworkNode* n = (VRNetworkNode*)selected_object;
    n->distributeKey();
    updateObjectData();
}

void VRGuiSetup::on_netnode_stopall_clicked() {
    if (guard) return;
    VRNetworkNode* n = (VRNetworkNode*)selected_object;
    n->stopSlaves();
    updateObjectData();
}

void VRGuiSetup::on_netslave_edited() {
    if (guard) return;
    VRNetworkSlave* n = (VRNetworkSlave*)selected_object;
    string ct = "StreamSock";
    if ( getRadioButtonState("radiobutton10") ) ct = "Multicast";
    if ( getRadioButtonState("radiobutton11") ) ct = "SockPipeline";

    bool fullscreen = getCheckButtonState("checkbutton29");
    bool astereo = getCheckButtonState("checkbutton41");
    bool astart = getCheckButtonState("checkbutton42");
    string display = getTextEntry("entry19");
    int port = toInt( getTextEntry("entry22") );
    int delay = toInt( getTextEntry("entry37") );
    string geometry = getTextEntry("entry38");
    n->set(ct, fullscreen, astereo, astart, display, port, delay, geometry);
    VRGuiWidget("toolbutton12").setSensitivity(true);
    updateObjectData();
}

void VRGuiSetup::on_netslave_start_clicked() {
    if (guard) return;
    VRNetworkSlave* n = (VRNetworkSlave*)selected_object;
    n->start();
    updateObjectData();
}

#ifndef WITHOUT_VIRTUOSE
void VRGuiSetup::on_haptic_ip_edited() {
    if (guard) return;
    VRHaptic* dev = (VRHaptic*)selected_object;
    dev->setIP(getTextEntry("entry8"));
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

void VRGuiSetup::on_leap_host_edited() {
    if (guard) return;
    VRLeap* dev = (VRLeap*)selected_object;
    dev->setAddress(getTextEntry("entry28"));
    dev->reconnect();
    setLabel("label157", dev->getConnectionStatus());
    setLabel("label159", dev->getSerial());
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_leap_startcalib_clicked() {
    if (guard) return;
    VRLeap* dev = (VRLeap*)selected_object;
    dev->startCalibration();
    setWidgetSensitivity("button34", false);
    setWidgetSensitivity("button35", true);
}
void VRGuiSetup::on_leap_stopcalib_clicked() {
    if (guard) return;
    VRLeap* dev = (VRLeap*)selected_object;
    dev->stopCalibration();
    setWidgetSensitivity("button34", true);
    setWidgetSensitivity("button35", false);
    auto p = dev->getPose();
    leapPosEntry.set(p->pos());
    leapUpEntry.set(p->up());
    leapDirEntry.set(p->dir());
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_leap_pos_edit(Vec3d v) {
    if (guard) return;
    VRLeap* dev = (VRLeap*)selected_object;
    auto p = dev->getPose();
    p->setPos(v);
    dev->setPose(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_leap_up_edit(Vec3d v) {
    if (guard) return;
    VRLeap* dev = (VRLeap*)selected_object;
    auto p = dev->getPose();
    p->setUp(v);
    dev->setPose(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

void VRGuiSetup::on_leap_dir_edit(Vec3d v) {
    if (guard) return;
    VRLeap* dev = (VRLeap*)selected_object;
    auto p = dev->getPose();
    p->setDir(v);
    dev->setPose(p);
    VRGuiWidget("toolbutton12").setSensitivity(true);
}

#ifndef WITHOUT_VIRTUOSE
void VRGuiSetup::on_change_haptic_type() {
    if (guard) return;
    VRHaptic* dev = (VRHaptic*)selected_object;
    dev->setType(getComboboxText("combobox25"));
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

#ifndef WITHOUT_MTOUCH
void VRGuiSetup::on_mt_device_changed() {
    if (guard) return;
    VRMultiTouch* dev = (VRMultiTouch*)selected_object;
    dev->setDevice(getComboboxText("combobox12"));
    VRGuiWidget("toolbutton12").setSensitivity(true);
}
#endif

void VRGuiSetup::on_toggle_dev_cross() {
    if (guard) return;

    bool b = getCheckButtonState("checkbutton37");
    VRDevice* dev = (VRDevice*)selected_object;
    dev->showHitPoint(b);
}

#ifndef WITHOUT_VRPN
void VRGuiSetup::on_toggle_vrpn_test_server() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    bool b = getCheckButtonState("checkbutton39");
    if (b) setup->startVRPNTestServer();
    else setup->stopVRPNTestServer();
}

void VRGuiSetup::on_toggle_vrpn_verbose() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    bool b = getCheckButtonState("checkbutton40");
    setup->setVRPNVerbose(b);
}
#endif

VRScriptPtr VRGuiSetup::getSelectedScript() {
    auto script = (VRScript*)selected_object;
    return script->ptr();
}

void VRGuiSetup::on_script_save_clicked() {
    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    string core = editor->getCore(script->getHeadSize());
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core);

    setWidgetSensitivity("toolbutton27", false);
    VRGuiWidget("toolbutton12").setSensitivity(true);
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
    bool noTrigger = getRadioToolButtonState("radiotoolbutton1");
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
    if (onFrame) script->changeTrigger(trig->getName(), "on_timeout");
}

shared_ptr<VRGuiEditor> VRGuiSetup::getEditor() { return editor; }

void VRGuiSetup::on_script_changed() {
    setWidgetSensitivity("toolbutton27", true);

    VRScriptPtr script = getSelectedScript();
    if (script == 0) return;

    string core = getEditor()->getCore(script->getHeadSize());
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core, false);
}

// --------------------------
// ---------Main-------------
// --------------------------

namespace PL = std::placeholders;

VRGuiSetup::VRGuiSetup() {
    selected_object = 0;
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
    setToolButtonCallback("toolbutton19", bind( &VRGuiSetup::on_foto_clicked, this) );
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

    setComboboxCallback("combobox6", bind( &VRGuiSetup::on_setup_changed, this) );
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
    setWidgetSensitivity("table8", false);

    updateSetupCb = VRFunction<VRDeviceWeakPtr>::create("update gui setup", bind(&VRGuiSetup::updateSetup, this) );

    guard = false;

    updateSetupList();
    updateSetup();
}

void VRGuiSetup::on_setup_changed() {
    if (guard) return;
    cout << "on_setup_changed\n";
    string name = getComboboxText("combobox6");
    if (name == "") return;

    static bool init = true;
    if (!init) if (!askUser("Switch to setup '" + name + "' - this will quit PolyVR", "Are you sure you want to switch to the " + name + " setup?")) return;

    string setupDirPath = setupDir();
    ofstream f(setupDirPath+".local"); f.write(name.c_str(), name.size()); f.close(); // remember setup
    string d = setupDirPath + name + ".xml";
    auto mgr = VRSetupManager::get();
    current_setup = mgr->load(name, d);
    updateSetup();

#ifndef WITHOUT_ART
    current_setup.lock()->getSignal_on_new_art_device()->add(updateSetupCb); // TODO: where to put this? NOT in updateSetup() !!!
#endif

    if (!init) {
        auto fkt = VRUpdateCb::create("setup_induced_shutdown", bind(&PolyVR::shutdown));
        VRSceneManager::get()->queueJob(fkt, 0, 100); // TODO: this blocks everything..
    }
    init = false;
}

void VRGuiSetup::on_window_device_changed() {
    if (guard || !window) return;
    string name = getComboboxText("combobox13");
    auto dev = VRSetup::getCurrent()->getDevice(name);
    window->setMouse( dynamic_pointer_cast<VRMouse>(dev) );
#ifndef WITHOUT_MTOUCH
    window->setMultitouch( dynamic_pointer_cast<VRMultiTouch>(dev) );
#endif
}

void VRGuiSetup::on_window_msaa_changed() {
    if (guard || !window) return;
    string name = getComboboxText("combobox15");
    window->setMSAA( name );
    setLabel("msaa_info", "to take effect please restart PolyVR!");
}

void VRGuiSetup::setTreeRow(GtkTreeStore* tree_store, GtkTreeIter* row, string name, string type, gpointer ptr, string fg, string bg) {
    gtk_tree_store_set(tree_store, row, 0, name.c_str(), -1);
    gtk_tree_store_set(tree_store, row, 1, type.c_str(), -1);
    gtk_tree_store_set(tree_store, row, 2, ptr, -1);
    gtk_tree_store_set(tree_store, row, 3, fg.c_str(), -1);
    gtk_tree_store_set(tree_store, row, 4, bg.c_str(), -1);
}

void VRGuiSetup::updateStatus() {
    if (mwindow) setLabel("win_state", mwindow->getStateString());
}

void VRGuiSetup::updateSetup() {
    auto tree_store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("setupTree");
    gtk_tree_store_clear(tree_store);

    GtkTreeIter network_itr, windows_itr, devices_itr, art_itr, vrpn_itr, scripts_itr;
    gtk_tree_store_append(tree_store, &network_itr, NULL);
    gtk_tree_store_append(tree_store, &windows_itr, NULL);
    gtk_tree_store_append(tree_store, &devices_itr, NULL);
    gtk_tree_store_append(tree_store, &art_itr, NULL);
    gtk_tree_store_append(tree_store, &vrpn_itr, NULL);
    gtk_tree_store_append(tree_store, &scripts_itr, NULL);

    setTreeRow(tree_store, &network_itr, "Network", "section", 0);
    setTreeRow(tree_store, &windows_itr, "Displays", "section", 0);
    setTreeRow(tree_store, &devices_itr, "Devices", "section", 0);
    setTreeRow(tree_store, &art_itr, "ART", "section", 0);
    setTreeRow(tree_store, &vrpn_itr, "VRPN", "section", 0);
    setTreeRow(tree_store, &scripts_itr, "Scripts", "section", 0);

    GtkTreeIter row;
    auto user_list = (GtkListStore*)VRGuiBuilder::get()->get_object("user_list");
    gtk_list_store_clear(user_list);
    gtk_list_store_append(user_list, &row);
    gtk_list_store_set(user_list, &row, 0, "None", -1);
    gtk_list_store_set(user_list, &row, 1, 0, -1);

    // Devices
    auto mouse_list = (GtkListStore*)VRGuiBuilder::get()->get_object("mouse_list");
    gtk_list_store_clear(mouse_list);
    gtk_list_store_append(mouse_list, &row);
    gtk_list_store_set (mouse_list, &row, 0, "None", -1);

    auto setup = current_setup.lock();
    setLabel("label13", "VR Setup: NONE");
    if (!setup) return;
    setLabel("label13", "VR Setup: " + setup->getName());

    for (auto ditr : setup->getDevices()) {
        VRDevicePtr dev = ditr.second;
        GtkTreeIter itr;
        gtk_tree_store_append(tree_store, &itr, &devices_itr);
        setTreeRow(tree_store, &itr, ditr.first.c_str(), dev->getType().c_str(), (gpointer)dev.get());

        if (dev->getType() == "mouse") {
            gtk_list_store_append(mouse_list, &row);
            gtk_list_store_set(mouse_list, &row, 0, dev->getName().c_str(), -1);
        }

        if (dev->getType() == "multitouch") {
            gtk_list_store_append(mouse_list, &row);
            gtk_list_store_set(mouse_list, &row, 0, dev->getName().c_str(), -1);
        }
    }

    for (auto node : setup->getNetwork()->getData() ) {
        GtkTreeIter itr, itr2;
        gtk_tree_store_append(tree_store, &itr, &network_itr);
        setTreeRow(tree_store, &itr, node->getName().c_str(), "node", (gpointer)node.get(), "#000000", "#FFFFFF");
        for (auto slave : node->getData() ) {
            gtk_tree_store_append(tree_store, &itr2, &itr);
            setTreeRow(tree_store, &itr2, slave->getName().c_str(), "slave", (gpointer)slave.get(), "#000000", "#FFFFFF");
        }
    }

    for (auto win : setup->getWindows()) {
        VRWindow* w = win.second.get();
        string name = win.first;
        GtkTreeIter itr, itr2;
        gtk_tree_store_append(tree_store, &itr, &windows_itr);
        string bg = "#FFFFFF";
        if (w->isActive() == false) bg = "#FFDDDD";
        setTreeRow(tree_store, &itr, name.c_str(), "window", (gpointer)w, "#000000", bg);

        // add viewports
        vector<VRViewPtr> views = w->getViews();
        for (uint i=0; i<views.size(); i++) {
            VRViewPtr v = views[i];
            stringstream ss;
            ss << name << i;
            gtk_tree_store_append(tree_store, &itr2, &itr);
            setTreeRow(tree_store, &itr2, ss.str().c_str(), "view", (gpointer)v.get());
        }
    }

#ifndef WITHOUT_VRPN
    // VRPN
    vector<int> vrpnIDs = setup->getVRPNTrackerIDs();
    for (uint i=0; i<vrpnIDs.size(); i++) {
        VRPN_device* t = setup->getVRPNTracker(vrpnIDs[i]).get();
        GtkTreeIter itr;
        gtk_tree_store_append(tree_store, &itr, &vrpn_itr);
        cout << "vrpn liststore: " << t->getName() << endl;
        setTreeRow(tree_store, &itr, t->getName().c_str(), "vrpn_tracker", (gpointer)t);
    }
#endif

#ifndef WITHOUT_ART
    // ART
    for (int ID : setup->getARTDevices() ) {
        ART_devicePtr dev = setup->getARTDevice(ID);

        GtkTreeIter itr, row;
        gtk_tree_store_append(tree_store, &itr, &art_itr);
        string name = dev->getName();
        if (dev->dev) name = dev->dev->getName();
        else if (dev->ent) name = dev->ent->getName();
        setTreeRow(tree_store, &itr, name.c_str(), "art_device", (gpointer)dev.get());

        if (dev->ent) {
            gtk_list_store_append(user_list, &row);
            gtk_list_store_set (user_list, &row, 0, dev->ent->getName().c_str(), -1);
            gtk_list_store_set (user_list, &row, 1, dev->ent.get(), -1);
        }
    }
#endif

    for (auto s : setup->getScripts()) {
        auto script = s.second.get();
        GtkTreeIter itr;
        gtk_tree_store_append(tree_store, &itr, &scripts_itr);
        setTreeRow(tree_store, &itr, script->getName().c_str(), "script", (gpointer)script);
    }

    on_treeview_select();
    auto tree_view = VRGuiBuilder::get()->get_widget("treeview2");
    gtk_tree_view_expand_all((GtkTreeView*)tree_view);
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
    // update script list
    auto store = (GtkListStore*)VRGuiBuilder::get()->get_object("setups");
    gtk_list_store_clear(store);

    string dir = setupDir();
    if (!VRGuiFile::exists(dir)) { cerr << "Error: no local directory setup\n"; return; }

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
    for(string name : VRGuiFile::listDir(dir)) { // update list
        if (!splitFileName(name, ending)) continue;
        GtkTreeIter row;
        gtk_list_store_append(store, &row);
        gtk_list_store_set (store, &row, 0, name.c_str(), -1);
    }

    int active = -1;
    auto setActive = [&](string n) {
        int i = 0;
        for(string name : VRGuiFile::listDir(dir)) {
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
    setCombobox("combobox6", active);
}

OSG_END_NAMESPACE;
