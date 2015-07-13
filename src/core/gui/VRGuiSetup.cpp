#include "VRGuiSetup.h"
#include "VRGuiUtils.h"
#include "VRGuiFile.h"
#include "core/scripting/VRScript.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/windows/VRWindow.h"
#include "core/setup/windows/VRMultiWindow.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/setup/devices/VRFlystick.h"
#include "core/setup/devices/VRHaptic.h"
#include "core/setup/devices/VRMobile.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include <gtkmm/liststore.h>
#include <gtkmm/treestore.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/notebook.h>
#include <gtkmm/combobox.h>
#include <gtkmm/cellrenderercombo.h>

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

void VRGuiSetup::updateObjectData() {
    guard = true;
    bool device = false;
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

    current_scene = VRSceneManager::getCurrent();

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
        setCombobox("combobox13", getListStorePos("mouse_list", name));
    }

    if (selected_type == "view") {
        setExpanderSensitivity("expander8", true);

        VRView* view = (VRView*)selected_object;

        Vec4f p = view->getPosition();
        setTextEntry("entry52", toString(p[0]).c_str());
        setTextEntry("entry53", toString(p[2]).c_str());
        setTextEntry("entry56", toString(p[1]).c_str());
        setTextEntry("entry57", toString(p[3]).c_str());

        setCheckButton("checkbutton9", view->eyesInverted());
        setCheckButton("checkbutton8", view->isStereo());
        setCheckButton("checkbutton11", view->isProjection());

        setTextEntry("entry12", toString(view->getEyeSeparation()).c_str());
        setCombobox("combobox18", getListStorePos("user_list", view->getUser()->getName()));

        Vec3f p3;
        if (view->getUser()) p3 = view->getUser()->getFrom();
        userEntry.set(p3);
        centerEntry.set(view->getProjectionCenter());
        normalEntry.set(view->getProjectionNormal());
        upEntry.set(view->getProjectionUp());
        sizeEntry.set(view->getProjectionSize());
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
        setExpanderSensitivity("expander6", true);
        ART_device* t = (ART_device*)selected_object;
        setTextEntry("entry40", toString(t->ID));
    }

    if (selected_type == "haptic") {
        setExpanderSensitivity("expander20", true);
        device = true;
        VRHaptic* t = (VRHaptic*)selected_object;
        setTextEntry("entry8", t->getIP());
        setCombobox("combobox25", getListStorePos("liststore8", t->getType()) );
    }

    if (selected_type == "mouse") { device = true; }
    if (selected_type == "keyboard") { device = true; }
    if (selected_type == "mobile") { device = true; }
    if (selected_type == "flystick") { device = true; }

    if (selected_type == "section") {
        if (selected_name == "ART") {
            setExpanderSensitivity("expander6", true);
            setTextEntry("entry39", toString(current_setup->getARTPort()));
            setCheckButton("checkbutton24", current_setup->getARTActive());

            Vec3f o = current_setup->getARTOffset();
            setTextEntry("entry48", toString(o[0]));
            setTextEntry("entry62", toString(o[1]));
            setTextEntry("entry63", toString(o[2]));
        }

        if (selected_name == "VRPN") {
            setExpanderSensitivity("expander7", true);
            setTextEntry("entry13", toString(current_setup->getVRPNPort()));
            setCheckButton("checkbutton25", current_setup->getVRPNActive());
        }
    }

    if (device) {
        VRDevice* dev = (VRDevice*)selected_object;
        VRIntersection ins = dev->getLastIntersection();

        setExpanderSensitivity("expander21", true);
        setLabel("label93", dev->getName());
        fillStringListstore("dev_types_list", current_setup->getDeviceTypes());
        setCombobox("combobox26", getListStorePos("dev_types_list", dev->getType()) );
        string hobj = ins.object && ins.hit ? ins.object->getName() : "NONE";
        setLabel("label110", hobj);
        setLabel("label111", toString(ins.point));
        setLabel("label112", toString(ins.texel));
        setCheckButton("checkbutton37", dev->getCross()->isVisible());
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
    current_setup = VRSetupManager::get()->create();
    updateSetup();
    // remember setup
    string name = current_setup->getName();
    ofstream f("setup/.local"); f.write(name.c_str(), name.size()); f.close();
}

void VRGuiSetup::on_foto_clicked() { //TODO, should create new setup
    if (current_setup == 0) return;
    bool b = getToggleButtonState("toolbutton19");
    current_setup->setFotoMode(b);
}

void VRGuiSetup::on_calibration_foreground_clicked() { //TODO, should create new setup
    if (current_setup == 0) return;
    bool b = getToggleButtonState("toolbutton20");
    current_setup->setCallibrationMode(b);
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

    if (current_setup == 0) return;
    current_setup->removeWindow(name);

    /*Gtk::ToolButton* b;
    VRGuiBuilder()->get_widget("toolbutton9", b);
    b->set_sensitive(false);
    VRGuiBuilder()->get_widget("toolbutton8", b);
    b->set_sensitive(false);*/
}


void VRGuiSetup::on_save_clicked() {
    if (current_setup == 0) return;
    current_setup->save(setupDir() + current_setup->getName() + ".xml");
    setToolButtonSensitivity("toolbutton12", false);
}

// setup list

void VRGuiSetup::on_setup_changed() {
    string name = getComboboxText("combobox6");
    ofstream f(setupDir()+".local"); f.write(name.c_str(), name.size()); f.close(); // remember setup
    current_setup = VRSetupManager::get()->load(name, setupDir() + name + ".xml");
    updateSetup();
}

void VRGuiSetup::on_treeview_select() {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview2"));
    Glib::RefPtr<Gtk::TreeStore> tree_store  = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("setupTree"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) {
        selected_type = "";
        updateObjectData();
        return;
    }

    // get selected window
    VRGuiSetup_ModelColumns cols;
    selected_row = *iter;

    selected_name = selected_row.get_value(cols.name);
    selected_type = selected_row.get_value(cols.type);
    selected_object = selected_row.get_value(cols.obj);

    if (selected_type == "window") mwindow = (VRMultiWindow*) selected_object;
    else mwindow = 0;

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
    if (type == "window") current_setup->changeWindowName(name, new_name);
    if (type == "vrpn_tracker") current_setup->changeVRPNDeviceName((VRPN_device*)obj, new_name);

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
    if (selected_type == "window") {
        VRWindow* win = (VRWindow*)selected_object;
        current_setup->removeWindow(win->getName());
    }

    if (selected_type == "view") {
        VRView* view = (VRView*)selected_object;
        current_setup->removeView(view->getID());
        VRWindow* win = (VRWindow*)selected_object_parent;
        win->remView(view);
    }

    if (selected_type == "vrpn_tracker") {
        VRPN_device* t = (VRPN_device*)selected_object;
        current_setup->delVRPNTracker(t);
    }

    if (selected_type == "art_device") {
        ;
    }

    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_window() {
    VRWindow* win = current_setup->addMultiWindow("Display");
    win->setActive(true);
    if ( VRSceneManager::getCurrent() ) win->setContent(true);

    updateSetup();
    selected_object = win;
    selected_type = "window";
    on_menu_add_viewport();
}

void VRGuiSetup::on_menu_add_viewport() {
    if (selected_type != "window") return;

    VRWindow* win = (VRWindow*)selected_object;
    int v = current_setup->addView(win->getBaseName());
    win->addView(current_setup->getView(v));

    if (current_scene) {
        current_setup->setViewCamera(current_scene->getActiveCamera(), v);
        current_setup->setViewRoot(current_scene->getRoot(), v);
        current_setup->setViewBackground(current_scene->getBackground(), v);
    }

    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_vrpn_tracker() {
    current_setup->addVRPNTracker(0, "Tracker0@localhost", Vec3f(0,0,0), 1);
    //current_setup->addVRPNTracker(0, "LeapTracker@tcp://141.3.151.136", Vec3f(0,0,0), 1);

    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_mouse() {
    VRMouse* m = new VRMouse();
    current_setup->addDevice(m);
    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_keyboard() {
    VRKeyboard* m = new VRKeyboard();
    current_setup->addDevice(m);
    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_haptic() {
    VRHaptic* h = new VRHaptic();
    current_setup->addDevice(h);
    updateSetup();
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_menu_add_mobile() {
    VRMobile* m = new VRMobile(5500);
    current_setup->addDevice(m);
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

void VRGuiSetup::on_pos_edit() {
    if (guard) return;
    if (selected_type != "view") return;

    string x0 = getTextEntry("entry52");
    string x1 = getTextEntry("entry53");
    string y0 = getTextEntry("entry56");
    string y1 = getTextEntry("entry57");
    Vec4f pos = toVec4f(x0 + " " + y0 + " " + x1 + " " + y1);

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

    VRView* view = (VRView*)selected_object;
    view->setUser(0);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_change_view_user() {
    if (guard) return;
    if (selected_type != "view") return;

    VRGuiSetup_UserColumns cols;
    Gtk::TreeModel::Row row = *getComboboxIter("combobox18");
    VRTransform* u = (VRTransform*)row.get_value(cols.user);

    VRView* view = (VRView*)selected_object;
    view->setUser(u);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_user_edit(Vec3f v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    if (view->getUser()) view->getUser()->setFrom(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_center_edit(Vec3f v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionCenter(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_normal_edit(Vec3f v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionNormal(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_up_edit(Vec3f v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionUp(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_proj_size_edit(Vec2f v) {
    if (guard) return;
    if (selected_type != "view") return;

    VRView* view = (VRView*)selected_object;
    view->setProjectionSize(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_vrpn_trans_axis_edit(Vec3f v) {
    if (guard) return;

    if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    t->setTranslationAxis(v);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_vrpn_rot_axis_edit(Vec3f v) {
    if (guard) return;

    if (selected_type != "vrpn_tracker") return;
    VRPN_device* t = (VRPN_device*)selected_object;

    t->setRotationAxis(v);
    setToolButtonSensitivity("toolbutton12", true);
}

// tracker

void VRGuiSetup::on_toggle_art() {
    if (guard) return;
    bool b = getCheckButtonState("checkbutton24");
    current_setup->setARTActive(b);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_toggle_vrpn() {
    if (guard) return;
    bool b = getCheckButtonState("checkbutton25");
    current_setup->setVRPNActive(b);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_art_edit_port() {
    if (guard) return;
    int p = toInt(getTextEntry("entry39"));
    current_setup->setARTPort(p);
    setToolButtonSensitivity("toolbutton12", true);
}

void VRGuiSetup::on_art_edit_offset() {
    if (guard) return;
    float ox = toFloat(getTextEntry("entry48"));
    float oy = toFloat(getTextEntry("entry62"));
    float oz = toFloat(getTextEntry("entry63"));
    current_setup->setARTOffset(Vec3f(ox,oy,oz));
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
    int p = toInt(getTextEntry("entry13"));
    current_setup->setVRPNPort(p);
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

void VRGuiSetup::on_toggle_dev_cross() {
    if (guard) return;

    bool b = getCheckButtonState("checkbutton37");
    VRDevice* dev = (VRDevice*)selected_object;
    dev->showHitPoint(b);
}

void VRGuiSetup::on_toggle_vrpn_test_server() {
    if (guard) return;
    bool b = getCheckButtonState("checkbutton39");
    if (b) current_setup->startVRPNTestServer();
    else current_setup->stopVRPNTestServer();
}

void VRGuiSetup::on_toggle_vrpn_verbose() {
    if (guard) return;
    bool b = getCheckButtonState("checkbutton40");
    current_setup->setVRPNVerbose(b);
}

// --------------------------
// ---------Main-------------
// --------------------------

VRGuiSetup::VRGuiSetup() {
    selected_object = 0;
    current_setup = 0;
    current_scene = 0;
    mwindow = 0;
    guard = false;

    VRSceneManager::get()->addUpdateFkt(new VRFunction<int>("Setup_gui", boost::bind(&VRGuiSetup::updateStatus, this)));

    menu = new VRGuiContextMenu("SetupMenu");
    menu->appendMenu("SetupMenu", "Add", "SM_AddMenu");
    menu->appendItem("SetupMenu", "Delete", sigc::mem_fun(*this, &VRGuiSetup::on_menu_delete));
    menu->appendItem("SM_AddMenu", "Window", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_window) );
    menu->appendItem("SM_AddMenu", "Viewport", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_viewport) );
    menu->appendMenu("SM_AddMenu", "Device", "SM_AddDevMenu");
    menu->appendMenu("SM_AddMenu", "VRPN", "SM_AddVRPNMenu");
    menu->appendItem("SM_AddDevMenu", "Mouse", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_mouse) );
    menu->appendItem("SM_AddDevMenu", "Keyboard", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_keyboard) );
    menu->appendItem("SM_AddDevMenu", "Haptic", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_haptic) );
    menu->appendItem("SM_AddDevMenu", "Mobile", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_mobile) );
    menu->appendItem("SM_AddVRPNMenu", "VRPN tracker", sigc::mem_fun(*this, &VRGuiSetup::on_menu_add_vrpn_tracker) );

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
    setToolButtonCallback("toolbutton20", sigc::mem_fun(*this, &VRGuiSetup::on_calibration_foreground_clicked) );

    centerEntry.init("center_entry", "center", sigc::mem_fun(*this, &VRGuiSetup::on_proj_center_edit));
    userEntry.init("user_entry", "user", sigc::mem_fun(*this, &VRGuiSetup::on_proj_user_edit));
    normalEntry.init("normal_entry", "normal", sigc::mem_fun(*this, &VRGuiSetup::on_proj_normal_edit));
    upEntry.init("viewup_entry", "up", sigc::mem_fun(*this, &VRGuiSetup::on_proj_up_edit));
    sizeEntry.init2D("size_entry", "size", sigc::mem_fun(*this, &VRGuiSetup::on_proj_size_edit));

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
    setEntryCallback("entry48", sigc::mem_fun(*this, &VRGuiSetup::on_art_edit_offset) );
    setEntryCallback("entry62", sigc::mem_fun(*this, &VRGuiSetup::on_art_edit_offset) );
    setEntryCallback("entry63", sigc::mem_fun(*this, &VRGuiSetup::on_art_edit_offset) );
    setEntryCallback("entry8", sigc::mem_fun(*this, &VRGuiSetup::on_haptic_ip_edited) );

    setComboboxCallback("combobox6", sigc::mem_fun(*this, &VRGuiSetup::on_setup_changed) );
    setComboboxCallback("combobox18", sigc::mem_fun(*this, &VRGuiSetup::on_change_view_user) );
    setComboboxCallback("combobox25", sigc::mem_fun(*this, &VRGuiSetup::on_change_haptic_type) );

    const char *haptic_types[] = {"Virtuose 6D Desktop", "Virtuose 6D35-45", "Virtuose 6D40-40", "INCA 6D"};
    fillStringListstore("liststore8", vector<string>(haptic_types, end(haptic_types)) );

    tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview2"));
    tree_view->signal_cursor_changed().connect( sigc::mem_fun(*this, &VRGuiSetup::on_treeview_select) );
    tree_view->signal_button_release_event().connect( sigc::mem_fun(*this, &VRGuiSetup::on_treeview_rightclick) );

    crt = Glib::RefPtr<Gtk::CellRendererText>::cast_static(VRGuiBuilder()->get_object("cellrenderertext3"));
    crt->signal_edited().connect( sigc::mem_fun(*this, &VRGuiSetup::on_name_edited) );
    crt = Glib::RefPtr<Gtk::CellRendererText>::cast_static(VRGuiBuilder()->get_object("cellrenderertext21"));
    crt->signal_edited().connect( sigc::mem_fun(*this, &VRGuiSetup::on_server_edit) );

    setCheckButtonCallback("checkbutton9", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_view_invert));
    setCheckButtonCallback("checkbutton7", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_display_active));
    setCheckButtonCallback("checkbutton8", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_display_stereo));
    setCheckButtonCallback("checkbutton11", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_display_projection));
    setCheckButtonCallback("checkbutton24", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_art));
    setCheckButtonCallback("checkbutton25", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_vrpn));
    setCheckButtonCallback("checkbutton26", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_view_user));
    setCheckButtonCallback("checkbutton4", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_view_stats));
    setCheckButtonCallback("checkbutton37", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_dev_cross));
    setCheckButtonCallback("checkbutton39", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_vrpn_test_server));
    setCheckButtonCallback("checkbutton40", sigc::mem_fun(*this, &VRGuiSetup::on_toggle_vrpn_verbose));

    // primitive list
    fillStringListstore("prim_list", VRPrimitive::getTypes());

    setTableSensitivity("table2", false);
    setTableSensitivity("table7", false);
    setTableSensitivity("table8", false);

    updateSetupList();
    updateSetup();

    VRDevCb* fkt = new VRDevCb("update gui setup", boost::bind(&VRGuiSetup::updateSetup, this) );
    VRSetupManager::getCurrent()->getSignal_on_new_art_device()->add(fkt);
}

void VRGuiSetup::setTreeRow(Glib::RefPtr<Gtk::TreeStore> tree_store, Gtk::TreeStore::Row row, string name, string type, gpointer ptr, string fg, string bg) {
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 0, name.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 1, type.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 2, ptr, -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 3, fg.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 4, bg.c_str(), -1);
}

void VRGuiSetup::updateStatus() {
    if (mwindow != 0) setLabel("win_state", mwindow->getStateString());
}

void VRGuiSetup::updateSetup() {
    Glib::RefPtr<Gtk::TreeStore> tree_store = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("setupTree"));
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview2"));
    tree_store->clear();

    Gtk::TreeModel::iterator itr;
    Gtk::TreeModel::iterator itr2;
    Gtk::TreeStore::Row row;

    Gtk::TreeModel::iterator windows_itr;
    Gtk::TreeModel::iterator devices_itr;
    Gtk::TreeModel::iterator art_itr;
    Gtk::TreeModel::iterator vrpn_itr;

    windows_itr = tree_store->append();
    devices_itr = tree_store->append();
    art_itr = tree_store->append();
    vrpn_itr = tree_store->append();

    setTreeRow(tree_store, *windows_itr, "Displays", "section", 0);
    setTreeRow(tree_store, *devices_itr, "Devices", "section", 0);
    setTreeRow(tree_store, *art_itr, "ART", "section", 0);
    setTreeRow(tree_store, *vrpn_itr, "VRPN", "section", 0);

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

    for (auto ditr : current_setup->getDevices()) {
        VRDevice* dev = ditr.second;
        itr = tree_store->append(devices_itr->children());
        setTreeRow(tree_store, *itr, ditr.first.c_str(), dev->getType().c_str(), (gpointer)dev);

        if (dev->getType() == "mouse") {
            row = *mouse_list->append();
            gtk_list_store_set (mouse_list->gobj(), row.gobj(), 0, dev->getName().c_str(), -1);
        }
    }

    for (auto win : current_setup->getWindows()) {
        VRWindow* w = win.second;
        string name = win.first;
        itr = tree_store->append(windows_itr->children());
        string bg = "#FFFFFF";
        if (w->isActive() == false) bg = "#FFDDDD";
        setTreeRow(tree_store, *itr, name.c_str(), "window", (gpointer)w, "#000000", bg);

        // add viewports
        vector<VRView*> views = w->getViews();
        for (uint i=0; i<views.size(); i++) {
            VRView* v = views[i];
            stringstream ss;
            ss << name << i;
            itr2 = tree_store->append(itr->children());
            setTreeRow(tree_store, *itr2, ss.str().c_str(), "view", (gpointer)v);
        }
    }

    // VRPN
    vector<int> vrpnIDs = current_setup->getVRPNTrackerIDs();
    for (uint i=0; i<vrpnIDs.size(); i++) {
        VRPN_device* t = current_setup->getVRPNTracker(vrpnIDs[i]);
        itr = tree_store->append(vrpn_itr->children());
        cout << "vrpn liststore: " << t->getName() << endl;
        setTreeRow(tree_store, *itr, t->getName().c_str(), "vrpn_tracker", (gpointer)t);
    }

    // ART
    for (int ID : current_setup->getARTDevices() ) {
        ART_device* dev = current_setup->getARTDevice(ID);

        itr = tree_store->append(art_itr->children());
        string name = dev->getName();
        if (dev->dev) name = dev->dev->getName();
        else if (dev->ent) name = dev->ent->getName();
        setTreeRow(tree_store, *itr, name.c_str(), "art_device", (gpointer)dev);

        if (dev->ent) {
            row = *user_list->append();
            gtk_list_store_set (user_list->gobj(), row.gobj(), 0, dev->ent->getName().c_str(), -1);
            gtk_list_store_set (user_list->gobj(), row.gobj(), 1, dev->ent, -1);
        }
    }

    on_treeview_select();
    tree_view->expand_all();

    setLabel("label13", "VR Setup: " + VRSetupManager::getCurrent()->getName());
}

void VRGuiSetup::updateSetupList() {
    // update script list
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("setups"));
    store->clear();

    string dir = setupDir();
    if (!VRGuiFile::exists(dir)) { cerr << "Error: no local directory setup\n"; return; }

    string last;
    ifstream f(dir+".local");
    if (!f.good()) f.open(dir+".default");
    if (!f.good()) { cerr << "Error: no setup file found\n"; return; }
    getline(f, last);
    f.close();

    int active = 0;
    int i = 0;
    for(string name : VRGuiFile::listDir(dir)) {
        int N = name.size();
        if (N < 6) continue;

        string ending = name.substr(N-4, N-1);
        name = name.substr(0,N-4);

        if (ending != ".xml") continue;

        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, name.c_str(), -1);

        if (last == name) active = i;
        i++;
    }

    setCombobox("combobox6", active);
}

OSG_END_NAMESPACE;
