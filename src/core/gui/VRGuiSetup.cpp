#include "VRGuiSetup.h"
#include "VRGuiUtils.h"
#include "VRGuiFile.h"
#include "VRGuiSignals.h"
#include "VRGuiContextMenu.h"
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
#include "core/setup/devices/VRHaptic.h"
#include "core/setup/devices/VRServer.h"
#include "core/setup/devices/VRMouse.h"
#include "core/setup/devices/VRMultiTouch.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/utils/VRManager.cpp"
#include <gtkmm/builder.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treestore.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/notebook.h>
#include <gtkmm/combobox.h>
#include <gtkmm/cellrenderercombo.h>

#include "core/objects/VRCamera.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSetup_ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSetup_ModelColumns() { add(name); add(type); add(obj); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<gpointer> obj;
};

class VRGuiSetup_ServerColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSetup_ServerColumns() { add(y); add(x); add(server); }

        Gtk::TreeModelColumn<gint> y;
        Gtk::TreeModelColumn<gint> x;
        Gtk::TreeModelColumn<Glib::ustring> server;
};

class VRGuiSetup_UserColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSetup_UserColumns() { add(name); add(user); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<gpointer> user;
};

void VRGuiSetup::closeAllExpander() {
    setExpanderSensitivity("expander3", false);
    setExpanderSensitivity("expander4", false);
    setExpanderSensitivity("expander5", false);
    setExpanderSensitivity("expander6", false);
    setExpanderSensitivity("expander7", false);
    setExpanderSensitivity("expander8", false);
    setExpanderSensitivity("expander20", false);
    setExpanderSensitivity("expander21", false);
    setExpanderSensitivity("expander22", false);
    setExpanderSensitivity("expander23", false);
    setExpanderSensitivity("expander24", false);
    setExpanderSensitivity("expander25", false);
    setExpanderSensitivity("expander26", false);
    setExpanderSensitivity("expander28", false);
    setExpanderSensitivity("expander29", false);
    setExpanderSensitivity("expander30", false);
}

void VRGuiSetup::updateObjectData() {
    bool device = false;
    guard = true;

    current_scene = VRScene::getCurrent();

    if (selected_type == "window") {
        setExpanderSensitivity("expander3", true);

        VRWindow* win = (VRWindow*)selected_object;
        setCheckButton("checkbutton7", win->isActive());

        if (win->hasType(0)) { // multiwindow
            setExpanderSensitivity("expander24", true);
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
            if (ct == "Multicast") setRadioButton("radiobutton6", 1);
            if (ct == "SockPipeline") setRadioButton("radiobutton7", 1);
            if (ct == "StreamSock") setRadioButton("radiobutton9", 1);

            //TODO: clear server array && add entry for each nx * ny
            Glib::RefPtr<Gtk::ListStore> servers = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("serverlist"));
            servers->clear();
            for (int y=0; y<ny; y++) {
                for (int x=0; x<nx; x++) {
                    Gtk::ListStore::Row row = *servers->append();
                    gtk_list_store_set (servers->gobj(), row.gobj(), 0, y, -1);
                    gtk_list_store_set (servers->gobj(), row.gobj(), 1, x, -1);
                    gtk_list_store_set (servers->gobj(), row.gobj(), 2, mwin->getServer(x,y).c_str(), -1);
                }
            }
        }

        if (win->hasType(1)) setExpanderSensitivity("expander23", true);
        if (win->hasType(2)) setExpanderSensitivity("expander22", true); // GTK

        // mouse
        string name = "None";
        if (win->getMouse()) name = win->getMouse()->getName();
        if (win->getMultitouch()) name = win->getMultitouch()->getName();
        setCombobox("combobox13", getListStorePos("mouse_list", name));
    }

    if (selected_type == "view") {
        setExpanderSensitivity("expander8", true);

        VRView* view = (VRView*)selected_object;

        Vec4d p = view->getPosition();
        setTextEntry("entry52", toString(p[0]).c_str());
        setTextEntry("entry53", toString(p[2]).c_str());
        setTextEntry("entry56", toString(p[1]).c_str());
        setTextEntry("entry57", toString(p[3]).c_str());

        setCheckButton("checkbutton8", view->isStereo());
        setCheckButton("checkbutton9", view->eyesInverted());
        setCheckButton("checkbutton10", view->activeStereo());
        setCheckButton("checkbutton11", view->isProjection());
        setCheckButton("checkbutton30", view->getMirror());

        setTextEntry("entry12", toString(view->getEyeSeparation()).c_str());
        int uID = getListStorePos("user_list", view->getUser()->getName());
        setCombobox("combobox18", uID);
        setCheckButton("checkbutton26", uID != -1);

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
        setExpanderSensitivity("expander4", true);
        setExpanderSensitivity("expander7", true);
        device = true;
        VRPN_device* t = (VRPN_device*)selected_object;
        setTextEntry("entry50", t->address);
    }

    if (selected_type == "vrpn_tracker") {
        setExpanderSensitivity("expander4", true);
        setExpanderSensitivity("expander7", true);
        VRPN_device* t = (VRPN_device*)selected_object;
        setTextEntry("entry50", t->address);
        tVRPNAxisEntry.set(t->translate_axis);
        rVRPNAxisEntry.set(t->rotation_axis);
    }

    if (selected_type == "art_device") {
        setExpanderSensitivity("expander5", true);
        ART_device* t = (ART_device*)selected_object;
        setTextEntry("entry40", toString(t->ID));
    }

    if (selected_type == "haptic") {
        setExpanderSensitivity("expander20", true);
        device = true;
        VRHaptic* t = (VRHaptic*)selected_object;
        setTextEntry("entry8", t->getIP());
        setCombobox("combobox25", getListStorePos("liststore8", t->getType()) );
        setLabel("label64", t->getDeamonState());
        setLabel("label66", t->getDeviceState());
    }

    if (selected_type == "multitouch") {
        setExpanderSensitivity("expander30", true);
        VRMultiTouch* t = (VRMultiTouch*)selected_object;
        setCombobox("combobox12", getListStorePos("liststore11", t->getDevice()) );
    }

    if (selected_type == "mouse") { device = true; }
    if (selected_type == "multitouch") { device = true; }
    if (selected_type == "keyboard") { device = true; }
    if (selected_type == "server") { device = true; }
    if (selected_type == "flystick") { device = true; }

    auto setup = current_setup.lock();
    if (selected_type == "vrpn_device" || selected_type == "vrpn_tracker") {
        if (setup) {
            setTextEntry("entry13", toString(setup->getVRPNPort()));
            setCheckButton("checkbutton25", setup->getVRPNActive());
        }
    }

    if (selected_type == "section" && setup) {
        if (selected_name == "ART") {
            setExpanderSensitivity("expander6", true);
            setTextEntry("entry39", toString(setup->getARTPort()));
            setCheckButton("checkbutton24", setup->getARTActive());

            artOffset.set(setup->getARTOffset());
            artAxis.set(Vec3d(setup->getARTAxis()));
        }

        if (selected_name == "VRPN") {
            setExpanderSensitivity("expander7", true);
            setTextEntry("entry13", toString(setup->getVRPNPort()));
            setCheckButton("checkbutton25", setup->getVRPNActive());
        }

        if (selected_name == "Displays") {
            setExpanderSensitivity("expander28", true);
            Vec3d o = setup->getDisplaysOffset();
            setTextEntry("entry29", toString(o[0]));
            setTextEntry("entry30", toString(o[1]));
            setTextEntry("entry31", toString(o[2]));
        }
    }

    if (device) {
        VRDevice* dev = (VRDevice*)selected_object;
        VRIntersection ins = dev->getLastIntersection();

        setExpanderSensitivity("expander21", true);
        setLabel("label93", dev->getName());
        if (setup) fillStringListstore("dev_types_list", setup->getDeviceTypes());
        setCombobox("combobox26", getListStorePos("dev_types_list", dev->getType()) );
        string hobj = ins.hit ? ins.name : "NONE";
        setLabel("label110", hobj);
        setLabel("label111", toString(ins.point));
        setLabel("label112", toString(ins.texel));
        setCheckButton("checkbutton37", dev->getCross()->isVisible());
    }

    if (selected_type == "node") {
        setExpanderSensitivity("expander25", true);
        VRNetworkNode* n = (VRNetworkNode*)selected_object;
        setTextEntry("entry15", n->getAddress());
        setTextEntry("entry20", n->getUser());
        setLabel("label130", n->getStatNode());
        setLabel("label129", n->getStatSSH());
        setLabel("label126", n->getStatSSHkey());
    }

    if (selected_type == "slave") {
        setExpanderSensitivity("expander26", true);
        VRNetworkSlave* n = (VRNetworkSlave*)selected_object;
        setLabel("label138", n->getConnectionIdentifier());
        setLabel("label132", n->getStatMulticast());
        setLabel("label136", n->getStat());
        setCheckButton("checkbutton29", n->getFullscreen());
        setCheckButton("checkbutton41", n->getActiveStereo());
        setCheckButton("checkbutton42", n->getAutostart());
        setTextEntry("entry19", n->getDisplay());
        setTextEntry("entry22", toString(n->getPort()));

        string ct = n->getConnectionType();
        if (ct == "Multicast") setRadioButton("radiobutton10", 1);
        if (ct == "SockPipeline") setRadioButton("radiobutton11", 1);
        if (ct == "StreamSock") setRadioButton("radiobutton12", 1);
    }

    if (selected_type == "script") {
        setExpanderSensitivity("expander29", true);
        VRScript* script = (VRScript*)selected_object;
        editor->setCore(script->getHead() + script->getCore());
        auto trigs = script->getTriggers();
        setRadioToolButton("radiotoolbutton1", true);
        if (trigs.size() > 0) {
            auto trig = *trigs.begin();
            if (trig) {
                if (trig->trigger == "on_timeout") setRadioToolButton("radiotoolbutton2", true);
                else setRadioToolButton("radiotoolbutton3", true);
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
    current_setup = VRSetupManager::get()->create();

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
        bool b = getToggleButtonState("toolbutton19");
        s->setFotoMode(b);
    }
}

void VRGuiSetup::on_del_clicked() { //TODO, should delete setup
    string msg1 = "Delete setup ";
    if (!askUser(msg1, "Are you sure you want to delete this script?")) return;
    return;

    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview2"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    VRGuiSetup_ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);

    Glib::RefPtr<Gtk::ListStore> list_store  = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("displays"));
    list_store->erase(iter);

    if (auto s = current_setup.lock()) s->removeWindow(name);

    /*Gtk::ToolButton* b;
    VRGuiBuilder()->get_widget("toolbutton9", b);
    b->set_sensitive(false);
    VRGuiBuilder()->get_widget("toolbutton8", b);
    b->set_sensitive(false);*/
}


void VRGuiSetup::on_save_clicked() {
    if (auto s = current_setup.lock()) {
        s->save(setupDir() + s->getName() + ".xml");
        setToolButtonSensitivity("toolbutton12", false);
    }
}

// setup list
void VRGuiSetup::on_treeview_select() {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview2"));
    Glib::RefPtr<Gtk::TreeStore> tree_store  = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("setupTree"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();

    closeAllExpander();
    if(!iter) {
        selected_type = "";
        updateObjectData();
        return;
    }

    // get selected window
    VRGuiSetup_ModelColumns cols;
    selected_row = *iter;

    selected_object = 0;
    selected_name = selected_row.get_value(cols.name);
    selected_type = selected_row.get_value(cols.type);
    selected_object = selected_row.get_value(cols.obj);

    //cout << "SELECT " << selected_name << " of type " << selected_type << endl;

    window = mwindow = 0;
    if (selected_type == "window") {
        window = (VRWindow*)selected_object;
        mwindow = (VRMultiWindow*)selected_object;
    }

    Gtk::TreePath path(iter);
    path.up();
    if (path.size()>0) {
        iter = tree_store->get_iter(path);
        parent_row = *iter;
        selected_object_parent = parent_row.get_value(cols.obj);
    }
    updateObjectData();
}

void VRGuiSetup::on_name_edited(const Glib::ustring& path, const Glib::ustring& new_name) {
    VRGuiSetup_ModelColumns cols;
    string type = selected_row.get_value(cols.type);
    string name = selected_row.get_value(cols.name);
    gpointer obj = selected_row.get_value(cols.obj);

    // update key in map
    if (auto s = current_setup.lock()) {
        if (type == "window") s->changeWindowName(name, new_name);
        if (type == "vrpn_tracker") s->changeVRPNDeviceName(((VRPN_device*)obj)->ptr(), new_name);
        if (type == "node") ((VRNetworkNode*)obj)->setName(new_name);
        if (type == "slave") ((VRNetworkSlave*)obj)->setName(new_name);
    }

    selected_row[cols.name] = name;
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

    if (selected_type == "vrpn_tracker") {
        VRPN_device* t = (VRPN_device*)selected_object;
        setup->delVRPNTracker(t->ptr());
    }

    if (selected_type == "art_device") {
        ;
    }

    if (selected_type == "node") {
        auto node = (VRNetworkNode*)selected_object;
        setup->getNetwork()->rem( node->getName() );
    }

    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
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
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_vrpn_tracker() {
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->addVRPNTracker(0, "Tracker0@localhost", Vec3d(0,0,0), 1);
    //setup->addVRPNTracker(0, "LeapTracker@tcp://141.3.151.136", Vec3d(0,0,0), 1);

    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

template<class T>
void VRGuiSetup::on_menu_add_device() {
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->addDevice(T::create());
    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_network_node() {
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->getNetwork()->add("Node");
    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_network_slave() {
    if (selected_type != "node") return;
    VRNetworkNode* n = (VRNetworkNode*)selected_object;
    n->add("Slave");
    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_script() {
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->addScript("script");
    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

// window options

void VRGuiSetup::on_toggle_display_active() {
    bool b = getCheckButtonState("checkbutton7");
    setTableSensitivity("table2", b);
    if (guard) return;

    if (selected_type != "window") return;
    VRWindow* win = (VRWindow*)selected_object;

    //cout << "\nToggleActive " << name << " " << b << endl;
    win->setActive(b);

    string bg = "#FFFFFF";
    if (!b) bg = "#FFDDDD";
    Glib::RefPtr<Gtk::TreeStore> tree_store = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("setupTree"));
    setTreeRow(tree_store, selected_row, win->getName().c_str(), "window", (gpointer)win, "#000000", bg);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_servern_edit() {
    if (guard) return;
    if (selected_type != "window") return;

    string nx = getTextEntry("entry33");
    string ny = getTextEntry("entry34");

    VRMultiWindow* mwin = (VRMultiWindow*)selected_object;
    mwin->setNTiles(toInt(nx.c_str()), toInt(ny.c_str()));
    updateObjectData();
    setToolButtonSensitivity("toolbutton12", true);
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
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_server_edit(const Glib::ustring& path, const Glib::ustring& server) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview1"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // get selected window
    Gtk::TreeModel::Row row = *iter;

    VRGuiSetup_ServerColumns cols;
    int x = row.get_value(cols.x);
    int y = row.get_value(cols.y);
    row[cols.server] = server;

    VRMultiWindow* mwin = (VRMultiWindow*)selected_object;
    mwin->setServer(x,y,server);
    mwin->reset();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_connect_mw_clicked() {
    VRMultiWindow* mwin = (VRMultiWindow*)selected_object;
    if (mwin == 0) return;
    mwin->reset();
    setToolButtonSensitivity("toolbutton12", true);
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
    setTableSensitivity("table7", b);
    if (guard) return;

    if (selected_type != "view") return;
    VRView* view = (VRView*)selected_object;

    view->setStereo(b);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_toggle_display_projection() {
    bool b = getCheckButtonState("checkbutton11");
    setTableSensitivity("table8", b);
    if (guard) return;

    if (selected_type != "view") return;
    VRView* view = (VRView*)selected_object;

    view->setProjection(b);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_toggle_view_invert() {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;

    bool b = getCheckButtonState("checkbutton9");
    view->swapEyes(b);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_toggle_view_active_stereo() {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;

    bool b = getCheckButtonState("checkbutton10");
    view->setActiveStereo(b);
    setToolButtonSensitivity("toolbutton12", true);
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
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_eyesep_edit() {
    if (guard) return;
    if (selected_type != "view") return;

    string es = getTextEntry("entry12");

    VRView* view = (VRView*)selected_object;
    view->setStereoEyeSeparation(toFloat(es));
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_toggle_view_user() {
    if (guard) return;
    if (selected_type != "view") return;

    bool b = getCheckButtonState("checkbutton26");
    VRView* view = (VRView*)selected_object;
    if (!b) view->setUser(0);
    else {
        VRGuiSetup_UserColumns cols;
        Gtk::TreeModel::Row row = *getComboboxIter("combobox18");
        if (auto U = row.get_value(cols.user)) {
            VRTransformPtr u = ( (VRTransform*)U )->ptr();
            view->setUser(u);
        }
    }
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_toggle_view_mirror() {
    if (guard) return;
    if (selected_type != "view") return;

    bool b = getCheckButtonState("checkbutton30");
    VRView* view = (VRView*)selected_object;
    view->setMirror(b);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_view_mirror_pos_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setMirrorPos(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_view_mirror_norm_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setMirrorNorm(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_change_view_user() {
    if (guard) return;
    if (selected_type != "view") return;

    VRGuiSetup_UserColumns cols;
    Gtk::TreeModel::Row row = *getComboboxIter("combobox18");
    //VRTransformPtr u = static_pointer_cast<VRTransform>(row.get_value(cols.user));
    auto U = row.get_value(cols.user);
    if (!U) return;
    VRTransformPtr u = ( (VRTransform*)U )->ptr();

    VRView* view = (VRView*)selected_object;
    view->setUser(u);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_view_size_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setSize(Vec2i(v));
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_user_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionUser(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_center_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionCenter(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_normal_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionNormal(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_up_edit(Vec3d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionUp(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_size_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionSize(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_shear_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionShear(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_warp_edit(Vec2d v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionWarp(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_vrpn_trans_axis_edit(Vec3d v) {
    if (guard) return;

    if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    t->setTranslationAxis(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_vrpn_rot_axis_edit(Vec3d v) {
    if (guard) return;

    if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    t->setRotationAxis(v);
    setToolButtonSensitivity("toolbutton12", true);
}

// tracker

void VRGuiSetup::on_toggle_art() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    bool b = getCheckButtonState("checkbutton24");
    setup->setARTActive(b);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_toggle_vrpn() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    bool b = getCheckButtonState("checkbutton25");
    setup->setVRPNActive(b);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_art_edit_port() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    int p = toInt(getTextEntry("entry39"));
    setup->setARTPort(p);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_displays_edit_offset() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    float ox = toFloat(getTextEntry("entry29"));
    float oy = toFloat(getTextEntry("entry30"));
    float oz = toFloat(getTextEntry("entry31"));
    setup->setDisplaysOffset(Vec3d(ox,oy,oz));
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_art_edit_offset(Vec3d v) {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->setARTOffset(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_art_edit_axis(Vec3d v) {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    setup->setARTAxis(Vec3i(round(v[0]), round(v[1]), round(v[2])));
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_art_edit_id() {
    if (guard) return;
    int id = toInt(getTextEntry("entry40"));
    ART_device* dev = (ART_device*)selected_object;
    dev->ID = id;
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_vrpn_edit_port() {
    if (guard) return;
    auto setup = current_setup.lock();
    if (!setup) return;
    int p = toInt(getTextEntry("entry13"));
    setup->setVRPNPort(p);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_edit_VRPN_tracker_address() {
    if (guard) return;
    if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    string txt = getTextEntry("entry50");
    t->setAddress(txt);

    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_netnode_edited() {
    if (guard) return;
    VRNetworkNode* n = (VRNetworkNode*)selected_object;
    n->set(getTextEntry("entry15"), getTextEntry("entry20"));
    setToolButtonSensitivity("toolbutton12", true);
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
    n->set(ct, getCheckButtonState("checkbutton29"), getCheckButtonState("checkbutton41"),
           getCheckButtonState("checkbutton42"), getTextEntry("entry19"), toInt( getTextEntry("entry22") ));
    setToolButtonSensitivity("toolbutton12", true);
    updateObjectData();
}

void VRGuiSetup::on_netslave_start_clicked() {
    if (guard) return;
    VRNetworkSlave* n = (VRNetworkSlave*)selected_object;
    n->start();
    updateObjectData();
}

void VRGuiSetup::on_haptic_ip_edited() {
    if (guard) return;
    VRHaptic* dev = (VRHaptic*)selected_object;
    dev->setIP(getTextEntry("entry8"));
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_change_haptic_type() {
    if (guard) return;
    VRHaptic* dev = (VRHaptic*)selected_object;
    dev->setType(getComboboxText("combobox25"));
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_mt_device_changed() {
    if (guard) return;
    VRMultiTouch* dev = (VRMultiTouch*)selected_object;
    dev->setDevice(getComboboxText("combobox12"));
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_toggle_dev_cross() {
    if (guard) return;

    bool b = getCheckButtonState("checkbutton37");
    VRDevice* dev = (VRDevice*)selected_object;
    dev->showHitPoint(b);
}

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

    setToolButtonSensitivity("toolbutton27", false);
    setToolButtonSensitivity("toolbutton12", true);
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
    setToolButtonSensitivity("toolbutton12", true);

    for (auto t : script->getTriggers()) script->remTrigger(t->getName());
    if (noTrigger) return;
    auto trig = script->addTrigger();
    script->changeTrigParams(trig->getName(), "0");
    if (onStart) script->changeTrigger(trig->getName(), "on_scene_load");
    if (onFrame) script->changeTrigger(trig->getName(), "on_timeout");
}

shared_ptr<VRGuiEditor> VRGuiSetup::getEditor() { return editor; }

void VRGuiSetup_on_script_changed(GtkTextBuffer* tb, gpointer user_data) {
    setToolButtonSensitivity("toolbutton27", true);

    auto gs = (VRGuiSetup*)user_data;
    VRScriptPtr script = gs->getSelectedScript();
    if (script == 0) return;

    string core = gs->getEditor()->getCore(script->getHeadSize());
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    scene->updateScript(script->getName(), core, false);
}

// --------------------------
// ---------Main-------------
// --------------------------

VRGuiSetup::VRGuiSetup() {
    selected_object = 0;
    mwindow = 0;
    guard = true;

    updatePtr = VRUpdateCb::create("Setup_gui", boost::bind(&VRGuiSetup::updateStatus, this));
    VRSceneManager::get()->addUpdateFkt(updatePtr);

    menu = new VRGuiContextMenu("SetupMenu");
    menu->appendMenu("SetupMenu", "Add", "SM_AddMenu");
    menu->appendItem("SetupMenu", "Delete", sigc::mem_fun(*this, &VRGuiSetup::on_menu_delete));
    menu->appendItem("SM_AddMenu", "Window", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_window) );
    menu->appendItem("SM_AddMenu", "Viewport", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_viewport) );
    menu->appendMenu("SM_AddMenu", "Network", "SM_AddNetworkMenu");
    menu->appendMenu("SM_AddMenu", "Device", "SM_AddDevMenu");
    menu->appendMenu("SM_AddMenu", "VRPN", "SM_AddVRPNMenu");
    menu->appendItem("SM_AddDevMenu", "Mouse", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_device<VRMouse>) );
    menu->appendItem("SM_AddDevMenu", "MultiTouch", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_device<VRMultiTouch>) );
    menu->appendItem("SM_AddDevMenu", "Keyboard", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_device<VRKeyboard>) );
    menu->appendItem("SM_AddDevMenu", "Haptic", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_device<VRHaptic>) );
    menu->appendItem("SM_AddDevMenu", "Server", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_device<VRServer>) );
    menu->appendItem("SM_AddVRPNMenu", "VRPN tracker", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_vrpn_tracker) );
    menu->appendItem("SM_AddNetworkMenu", "Node", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_network_node) );
    menu->appendItem("SM_AddNetworkMenu", "Slave", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_network_slave) );
    menu->appendItem("SM_AddMenu", "Script", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_script) );

    Glib::RefPtr<Gtk::ToolButton> tbutton;
    Glib::RefPtr<Gtk::CheckButton> cbutton;
    Glib::RefPtr<Gtk::CellRendererText> crt;
    Glib::RefPtr<Gtk::TreeView> tree_view;
    Glib::RefPtr<Gtk::ComboBox> cbox;
    Glib::RefPtr<Gtk::Entry> entry;
    Glib::RefPtr<Gtk::Button> button;

    setLabel("label13", "VR Setup: None");

    setButtonCallback("button25", sigc::mem_fun(*this, &VRGuiSetup::on_connect_mw_clicked) );

    setToolButtonCallback("toolbutton10", sigc::mem_fun(*this, &VRGuiSetup::on_new_clicked) );
    setToolButtonCallback("toolbutton11", sigc::mem_fun(*this, &VRGuiSetup::on_del_clicked) );
    setToolButtonCallback("toolbutton12", sigc::mem_fun(*this, &VRGuiSetup::on_save_clicked) );
    setToolButtonCallback("toolbutton19", sigc::mem_fun(*this, &VRGuiSetup::on_foto_clicked) );
    setToolButtonCallback("toolbutton27", sigc::mem_fun(*this, &VRGuiSetup::on_script_save_clicked) );
    setToolButtonCallback("toolbutton26", sigc::mem_fun(*this, &VRGuiSetup::on_script_exec_clicked) );

    setRadioToolButtonCallback("radiotoolbutton1", sigc::mem_fun(*this, &VRGuiSetup::on_script_trigger_switched) );
    setRadioToolButtonCallback("radiotoolbutton2", sigc::mem_fun(*this, &VRGuiSetup::on_script_trigger_switched) );
    setRadioToolButtonCallback("radiotoolbutton3", sigc::mem_fun(*this, &VRGuiSetup::on_script_trigger_switched) );

    artAxis.init("art_axis", "Axis", sigc::mem_fun(*this, &VRGuiSetup::on_art_edit_axis));
    artOffset.init("art_offset", "Offset", sigc::mem_fun(*this, &VRGuiSetup::on_art_edit_offset));

    centerEntry.init("center_entry", "center", sigc::mem_fun(*this, &VRGuiSetup::on_proj_center_edit));
    userEntry.init("user_entry", "user", sigc::mem_fun(*this, &VRGuiSetup::on_proj_user_edit));
    normalEntry.init("normal_entry", "normal", sigc::mem_fun(*this, &VRGuiSetup::on_proj_normal_edit));
    upEntry.init("viewup_entry", "up", sigc::mem_fun(*this, &VRGuiSetup::on_proj_up_edit));
    mirrorPosEntry.init("mirror_pos_entry", "origin", sigc::mem_fun(*this, &VRGuiSetup::on_view_mirror_pos_edit));
    mirrorNormEntry.init("mirror_norm_entry", "normal", sigc::mem_fun(*this, &VRGuiSetup::on_view_mirror_norm_edit));
    sizeEntry.init2D("size_entry", "size", sigc::mem_fun(*this, &VRGuiSetup::on_proj_size_edit));
    shearEntry.init2D("shear_entry", "shear", sigc::mem_fun(*this, &VRGuiSetup::on_proj_shear_edit));
    warpEntry.init2D("warp_entry", "warp", sigc::mem_fun(*this, &VRGuiSetup::on_proj_warp_edit));
    vsizeEntry.init2D("vsize_entry", "size", sigc::mem_fun(*this, &VRGuiSetup::on_view_size_edit));

    tVRPNAxisEntry.init("tvrpn_entry", "", sigc::mem_fun(*this, &VRGuiSetup::on_vrpn_trans_axis_edit));
    rVRPNAxisEntry.init("rvrpn_entry", "", sigc::mem_fun(*this, &VRGuiSetup::on_vrpn_rot_axis_edit));

    setEntryCallback("entry50", sigc::mem_fun(*this, &VRGuiSetup::on_edit_VRPN_tracker_address) );
    setEntryCallback("entry52", sigc::mem_fun(*this, &VRGuiSetup::on_pos_edit) );
    setEntryCallback("entry53", sigc::mem_fun(*this, &VRGuiSetup::on_pos_edit) );
    setEntryCallback("entry56", sigc::mem_fun(*this, &VRGuiSetup::on_pos_edit) );
    setEntryCallback("entry57", sigc::mem_fun(*this, &VRGuiSetup::on_pos_edit) );
    setEntryCallback("entry12", sigc::mem_fun(*this, &VRGuiSetup::on_eyesep_edit) );
    setEntryCallback("entry13", sigc::mem_fun(*this, &VRGuiSetup::on_vrpn_edit_port) );
    setEntryCallback("entry33", sigc::mem_fun(*this, &VRGuiSetup::on_servern_edit) );
    setEntryCallback("entry34", sigc::mem_fun(*this, &VRGuiSetup::on_servern_edit) );
    setEntryCallback("entry39", sigc::mem_fun(*this, &VRGuiSetup::on_art_edit_port) );
    setEntryCallback("entry40", sigc::mem_fun(*this, &VRGuiSetup::on_art_edit_id) );
    setEntryCallback("entry29", sigc::mem_fun(*this, &VRGuiSetup::on_displays_edit_offset) );
    setEntryCallback("entry30", sigc::mem_fun(*this, &VRGuiSetup::on_displays_edit_offset) );
    setEntryCallback("entry31", sigc::mem_fun(*this, &VRGuiSetup::on_displays_edit_offset) );
    setEntryCallback("entry8", sigc::mem_fun(*this, &VRGuiSetup::on_haptic_ip_edited) );
    setEntryCallback("entry15", sigc::mem_fun(*this, &VRGuiSetup::on_netnode_edited) );
    setEntryCallback("entry20", sigc::mem_fun(*this, &VRGuiSetup::on_netnode_edited) );
    setEntryCallback("entry19", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_edited) );
    setEntryCallback("entry22", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_edited) );

    setButtonCallback("button6", sigc::mem_fun(*this, &VRGuiSetup::on_netnode_key_clicked) );
    setButtonCallback("button1", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_start_clicked) );
    setButtonCallback("button30", sigc::mem_fun(*this, &VRGuiSetup::on_netnode_stopall_clicked) );

    setRadioButtonCallback("radiobutton6", sigc::mem_fun(*this, &VRGuiSetup::on_server_ct_toggled) );
    setRadioButtonCallback("radiobutton7", sigc::mem_fun(*this, &VRGuiSetup::on_server_ct_toggled) );
    setRadioButtonCallback("radiobutton9", sigc::mem_fun(*this, &VRGuiSetup::on_server_ct_toggled) );
    setRadioButtonCallback("radiobutton10", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_edited));
    setRadioButtonCallback("radiobutton11", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_edited));
    setRadioButtonCallback("radiobutton12", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_edited));

    setComboboxCallback("combobox6", sigc::mem_fun(*this, &VRGuiSetup::on_setup_changed) );
    setComboboxCallback("combobox12", sigc::mem_fun(*this, &VRGuiSetup::on_mt_device_changed) );
    setComboboxCallback("combobox13", sigc::mem_fun(*this, &VRGuiSetup::on_window_device_changed) );
    setComboboxCallback("combobox18", sigc::mem_fun(*this, &VRGuiSetup::on_change_view_user) );
    setComboboxCallback("combobox25", sigc::mem_fun(*this, &VRGuiSetup::on_change_haptic_type) );

    fillStringListstore("liststore8", VRHaptic::getDevTypes() );
    fillStringListstore("liststore11", VRMultiTouch::getDevices() );

    tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview2"));
    tree_view->signal_cursor_changed().connect( sigc::mem_fun(*this, &VRGuiSetup::on_treeview_select) );
    tree_view->signal_button_release_event().connect( sigc::mem_fun(*this, &VRGuiSetup::on_treeview_rightclick) );

    crt = Glib::RefPtr<Gtk::CellRendererText>::cast_static(VRGuiBuilder()->get_object("cellrenderertext3"));
    crt->signal_edited().connect( sigc::mem_fun(*this, &VRGuiSetup::on_name_edited) );
    crt = Glib::RefPtr<Gtk::CellRendererText>::cast_static(VRGuiBuilder()->get_object("cellrenderertext21"));
    crt->signal_edited().connect( sigc::mem_fun(*this, &VRGuiSetup::on_server_edit) );

    setCheckButtonCallback("checkbutton9", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_view_invert));
    setCheckButtonCallback("checkbutton10", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_view_active_stereo));
    setCheckButtonCallback("checkbutton7", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_display_active));
    setCheckButtonCallback("checkbutton8", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_display_stereo));
    setCheckButtonCallback("checkbutton11", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_display_projection));
    setCheckButtonCallback("checkbutton24", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_art));
    setCheckButtonCallback("checkbutton25", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_vrpn));
    setCheckButtonCallback("checkbutton26", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_view_user));
    setCheckButtonCallback("checkbutton30", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_view_mirror));
    setCheckButtonCallback("checkbutton4", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_view_stats));
    setCheckButtonCallback("checkbutton37", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_dev_cross));
    setCheckButtonCallback("checkbutton39", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_vrpn_test_server));
    setCheckButtonCallback("checkbutton40", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_vrpn_verbose));
    setCheckButtonCallback("checkbutton29", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_edited));
    setCheckButtonCallback("checkbutton41", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_edited));
    setCheckButtonCallback("checkbutton42", sigc::mem_fun(*this, &VRGuiSetup::on_netslave_edited));


    editor = shared_ptr<VRGuiEditor>( new VRGuiEditor("scrolledwindow12") );
    g_signal_connect(editor->getSourceBuffer(), "changed", G_CALLBACK(VRGuiSetup_on_script_changed), this);

    // primitive list
    fillStringListstore("prim_list", VRPrimitive::getTypes());

    setTableSensitivity("table2", false);
    setTableSensitivity("table7", false);
    setTableSensitivity("table8", false);

    updateSetupCb = VRFunction<VRDeviceWeakPtr>::create("update gui setup", boost::bind(&VRGuiSetup::updateSetup, this) );

    guard = false;
    updateSetupList();
    updateSetup();
}

void VRGuiSetup::on_setup_changed() {
    if (guard) return;
    cout << "on_setup_changed\n";
    string name = getComboboxText("combobox6");
    ofstream f(setupDir()+".local"); f.write(name.c_str(), name.size()); f.close(); // remember setup
    string d = setupDir() + name + ".xml";
    auto mgr = VRSetupManager::get();
    current_setup = mgr->load(name, d);
    updateSetup();

    current_setup.lock()->getSignal_on_new_art_device()->add(updateSetupCb); // TODO: where to put this? NOT in updateSetup() !!!
}

void VRGuiSetup::on_window_device_changed() {
    if (guard || !window) return;
    string name = getComboboxText("combobox13");
    auto dev = VRSetup::getCurrent()->getDevice(name);
    window->setMouse( dynamic_pointer_cast<VRMouse>(dev) );
    window->setMultitouch( dynamic_pointer_cast<VRMultiTouch>(dev) );
}

void VRGuiSetup::setTreeRow(Glib::RefPtr<Gtk::TreeStore> tree_store, Gtk::TreeStore::Row row, string name, string type, gpointer ptr, string fg, string bg) {
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 0, name.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 1, type.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 2, ptr, -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 3, fg.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 4, bg.c_str(), -1);
}

void VRGuiSetup::updateStatus() {
    if (mwindow) setLabel("win_state", mwindow->getStateString());
}

void VRGuiSetup::updateSetup() {
    Glib::RefPtr<Gtk::TreeStore> tree_store = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("setupTree"));
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview2"));
    tree_store->clear();

    auto network_itr = tree_store->append();
    auto windows_itr = tree_store->append();
    auto devices_itr = tree_store->append();
    auto art_itr = tree_store->append();
    auto vrpn_itr = tree_store->append();
    auto scripts_itr = tree_store->append();

    setTreeRow(tree_store, *network_itr, "Network", "section", 0);
    setTreeRow(tree_store, *windows_itr, "Displays", "section", 0);
    setTreeRow(tree_store, *devices_itr, "Devices", "section", 0);
    setTreeRow(tree_store, *art_itr, "ART", "section", 0);
    setTreeRow(tree_store, *vrpn_itr, "VRPN", "section", 0);
    setTreeRow(tree_store, *scripts_itr, "Scripts", "section", 0);

    Gtk::TreeStore::Row row;
    Glib::RefPtr<Gtk::ListStore> user_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("user_list"));
    user_list->clear();
    row = *user_list->append();
    gtk_list_store_set (user_list->gobj(), row.gobj(), 0, "None", -1);
    gtk_list_store_set (user_list->gobj(), row.gobj(), 1, 0, -1);

    // Devices
    Glib::RefPtr<Gtk::ListStore> mouse_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("mouse_list"));
    mouse_list->clear();
    row = *mouse_list->append();
    gtk_list_store_set (mouse_list->gobj(), row.gobj(), 0, "None", -1);

    auto setup = current_setup.lock();
    setLabel("label13", "VR Setup: NONE");
    if (!setup) return;
    setLabel("label13", "VR Setup: " + setup->getName());

    for (auto ditr : setup->getDevices()) {
        VRDevicePtr dev = ditr.second;
        auto itr = tree_store->append(devices_itr->children());
        setTreeRow(tree_store, *itr, ditr.first.c_str(), dev->getType().c_str(), (gpointer)dev.get());

        if (dev->getType() == "mouse") {
            row = *mouse_list->append();
            gtk_list_store_set (mouse_list->gobj(), row.gobj(), 0, dev->getName().c_str(), -1);
        }

        if (dev->getType() == "multitouch") {
            row = *mouse_list->append();
            gtk_list_store_set (mouse_list->gobj(), row.gobj(), 0, dev->getName().c_str(), -1);
        }
    }

    for (auto node : setup->getNetwork()->getData() ) {
        auto itr = tree_store->append(network_itr->children());
        setTreeRow(tree_store, *itr, node->getName().c_str(), "node", (gpointer)node.get(), "#000000", "#FFFFFF");
        for (auto slave : node->getData() ) {
            auto itr2 = tree_store->append(itr->children());
            setTreeRow(tree_store, *itr2, slave->getName().c_str(), "slave", (gpointer)slave.get(), "#000000", "#FFFFFF");
        }
    }

    for (auto win : setup->getWindows()) {
        VRWindow* w = win.second.get();
        string name = win.first;
        auto itr = tree_store->append(windows_itr->children());
        string bg = "#FFFFFF";
        if (w->isActive() == false) bg = "#FFDDDD";
        setTreeRow(tree_store, *itr, name.c_str(), "window", (gpointer)w, "#000000", bg);

        // add viewports
        vector<VRViewPtr> views = w->getViews();
        for (uint i=0; i<views.size(); i++) {
            VRViewPtr v = views[i];
            stringstream ss;
            ss << name << i;
            auto itr2 = tree_store->append(itr->children());
            setTreeRow(tree_store, *itr2, ss.str().c_str(), "view", (gpointer)v.get());
        }
    }

    // VRPN
    vector<int> vrpnIDs = setup->getVRPNTrackerIDs();
    for (uint i=0; i<vrpnIDs.size(); i++) {
        VRPN_device* t = setup->getVRPNTracker(vrpnIDs[i]).get();
        auto itr = tree_store->append(vrpn_itr->children());
        cout << "vrpn liststore: " << t->getName() << endl;
        setTreeRow(tree_store, *itr, t->getName().c_str(), "vrpn_tracker", (gpointer)t);
    }

    // ART
    for (int ID : setup->getARTDevices() ) {
        ART_devicePtr dev = setup->getARTDevice(ID);

        auto itr = tree_store->append(art_itr->children());
        string name = dev->getName();
        if (dev->dev) name = dev->dev->getName();
        else if (dev->ent) name = dev->ent->getName();
        setTreeRow(tree_store, *itr, name.c_str(), "art_device", (gpointer)dev.get());

        if (dev->ent) {
            row = *user_list->append();
            gtk_list_store_set (user_list->gobj(), row.gobj(), 0, dev->ent->getName().c_str(), -1);
            gtk_list_store_set (user_list->gobj(), row.gobj(), 1, dev->ent.get(), -1);
        }
    }

    for (auto s : setup->getScripts()) {
        auto script = s.second.get();
        auto itr = tree_store->append(scripts_itr->children());
        setTreeRow(tree_store, *itr, script->getName().c_str(), "script", (gpointer)script);
    }

    on_treeview_select();
    tree_view->expand_all();
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
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("setups"));
    store->clear();

    string dir = setupDir();
    if (!VRGuiFile::exists(dir)) { cerr << "Error: no local directory setup\n"; return; }

    string local, def;
    if (!getSetupEntries(dir, local, def)) { cerr << "Error: no setup file found\n"; return; }

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
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, name.c_str(), -1);
    }

    int active = -1;
    auto setActive = [&](string n) {
        int i = 0;
        for(string name : VRGuiFile::listDir(dir)) {
            if (!splitFileName(name, ending)) continue;
            if (n == name) { active = i; break; }
            i++;
        }
    };

    setActive(local);
    if (active < 0) {
        cout << "Setup " << local << " not found. Load default: " << def << endl;
        setActive(def);
    }
    setCombobox("combobox6", active);
}

OSG_END_NAMESPACE;
