#include "VRGuiScene.h"

#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/table.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/notebook.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/drawingarea.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRSceneLoader.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRCamera.h"
#include "core/objects/VRGroup.h"
#include "core/objects/VRLod.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "addons/construction/building/VRElectricDevice.h"
//#include "addons/Engineering/CSG/CSGGeometry.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

Glib::RefPtr<Gtk::TreeStore> tree_store;
Glib::RefPtr<Gtk::TreeView> tree_view;

Gtk::TreeModel::iterator selected_itr;
string selected;
VRGeometry* selected_geometry = 0;
VRObject* VRGuiScene_copied = 0;
bool liveUpdate = false;
bool trigger_cbs = false;

VRGuiVectorEntry posEntry;
VRGuiVectorEntry atEntry;
VRGuiVectorEntry dirEntry;
VRGuiVectorEntry upEntry;
VRGuiVectorEntry scaleEntry;
VRGuiVectorEntry ctEntry;
VRGuiVectorEntry lodCEntry;

// --------------------------
// ---------ObjectForms------
// --------------------------

void parseSGTree(VRObject* o);
VRObject* getSelected() {
    if (selected == "") return 0;
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return 0;

    VRObject* root = scene->getRoot();
    VRObject* res = root->getAtPath(selected);

    if (res == 0) {
        cout << "did not find " << selected << endl;
        tree_store->clear();
        parseSGTree( root );
        tree_view->expand_all();
        setTableSensitivity("table11", false);
    }

    return res;
}

void setObject(VRObject* o) {
    setExpanderSensitivity("expander2", true);

    // set object properties
    setCheckButton("checkbutton6", o->isVisible());
    setCheckButton("checkbutton15", o->isPickable());

    VRObject* parent = o->getParent();
    if (parent) setTextEntry("entry17", parent->getName());
    else setTextEntry("entry17", "");
}

void setTransform(VRTransform* e) {
    setExpanderSensitivity("expander1", true);

    Vec3f f = e->getFrom();
    Vec3f a = e->getAt();
    Vec3f u = e->getUp();
    Vec3f d = e->getDir();
    Vec3f s = e->getScale();

    Vec3f tc = e->getTConstraint();
    Vec3i rc = e->getRConstraint();

    posEntry.set(f);
    atEntry.set(a);
    dirEntry.set(d);
    upEntry.set(u);
    scaleEntry.set(s);
    ctEntry.set(tc);

    atEntry.setFontColor(Vec3f(0, 0, 0));
    dirEntry.setFontColor(Vec3f(0, 0, 0));
    if (e->get_orientation_mode())  atEntry.setFontColor(Vec3f(0.6, 0.6, 0.6));
    else                            dirEntry.setFontColor(Vec3f(0.6, 0.6, 0.6));

    bool doTc = e->hasTConstraint();
    bool doRc = e->hasRConstraint();

    setCheckButton("checkbutton18", rc[0]);
    setCheckButton("checkbutton19", rc[1]);
    setCheckButton("checkbutton20", rc[2]);

    setCheckButton("checkbutton21", doTc);
    setCheckButton("checkbutton22", doRc);

    setRadioButton("radiobutton1", !e->getTConstraintMode());
    setRadioButton("radiobutton2", e->getTConstraintMode());

    setCheckButton("checkbutton13", e->getPhysics()->isPhysicalized());
    setCheckButton("checkbutton33", e->getPhysics()->isDynamic());
    setTextEntry("entry59", toString(e->getPhysics()->getMass()));
    setCombobox("combobox8", getListStorePos("phys_shapes", e->getPhysics()->getShape()));
    setComboboxSensitivity("combobox8", e->getPhysics()->isPhysicalized());
}

void setGeometry(VRGeometry* g) {
    //setExpanderSensitivity("expander11", true);
    setExpanderSensitivity("expander14", true);
    setExpanderSensitivity("expander16", true);
    VRMaterial* mat = g->getMaterial();

    bool lit = true;
    Color3f _cd, _cs, _ca;
    if (mat) {
        _cd = mat->getDiffuse();
        _cs = mat->getSpecular();
        _ca = mat->getAmbient();
        lit = mat->isLit();
    } else {
        _cd = _cs = _ca = Color3f();
    }

    setColorChooserColor("mat_diffuse", _cd);
    setColorChooserColor("mat_specular", _cs);
    setColorChooserColor("mat_ambient", _ca);

    setCheckButton("checkbutton3", lit);
    setCheckButton("checkbutton28", false);
    setCombobox("combobox21", -1);

    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("primitive_opts"));
    store->clear();
    string file, obj;
    stringstream params;
    params << g->getReference().parameter;

    setTextEntry("entry36", "");
    setTextEntry("entry37", "");

    switch (g->getReference().type) {
        case VRGeometry::FILE:
            params >> file;
            params >> obj;
            setTextEntry("entry36", file);
            setTextEntry("entry37", obj);
            break;
        case VRGeometry::CODE:
            break;
        case VRGeometry::PRIMITIVE:
            setCheckButton("checkbutton28", true);
            string ptype; params >> ptype;
            setCombobox("combobox21", getListStorePos("prim_list", ptype));
            vector<string> param_names = VRPrimitive::getTypeParameter(ptype);
            for (uint i=0; i<param_names.size(); i++) {
                string val; params >> val;
                Gtk::ListStore::Row row = *store->append();
                gtk_list_store_set (store->gobj(), row.gobj(), 0, param_names[i].c_str(), -1);
                gtk_list_store_set (store->gobj(), row.gobj(), 1, val.c_str(), -1);
            }
            break;
    }
}

void setLight(VRLight* l) {
    setExpanderSensitivity("expander13", true);

    setCheckButton("checkbutton31", l->isOn());
    setCheckButton("checkbutton32", l->getShadows());
    setCombobox("combobox2", getListStorePos("light_types", l->getLightType()));
    setCombobox("combobox22", getListStorePos("shadow_types", l->getShadowType()));

    setColorChooserColor("shadow_col", toColor3f(l->getShadowColor()));
    setColorChooserColor("light_diff", toColor3f(l->getLightDiffColor()));
    setColorChooserColor("light_amb", toColor3f(l->getLightAmbColor()));
    setColorChooserColor("light_spec", toColor3f(l->getLightSpecColor()));

    Vec3f a = l->getAttenuation();
    setTextEntry("entry44", toString(a[0]));
    setTextEntry("entry45", toString(a[1]));
    setTextEntry("entry46", toString(a[2]));

    string bname = "NONE";
    if (l->getBeacon()) bname = l->getBeacon()->getName();
    setButtonText("button27", bname);

    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("light_params"));
    vector<string> param_names = VRLight::getTypeParameter(l->getLightType());
    for (uint i=0; i<param_names.size(); i++) {
        string val; // TODO
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, param_names[i].c_str(), -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 1, val.c_str(), -1);
    }
}

void setCamera(VRCamera* c) {
    setExpanderSensitivity("expander12", true);
    setCheckButton("checkbutton17", c->getAcceptRoot());
    setTextEntry("entry60", toString(c->getAspect()));
    setTextEntry("entry61", toString(c->getFov()));
    setTextEntry("entry6", toString(c->getNear()));
    setTextEntry("entry7", toString(c->getFar()));
}

void setGroup(VRGroup* g) {
    setExpanderSensitivity("expander2", true);
    setExpanderSensitivity("expander9", true);
    setCheckButton("checkbutton23", g->getActive() );

    fillStringListstore("liststore3", g->getGroups());

    setCombobox("combobox14", getListStorePos("liststore3",g->getGroup()));
}

void setLod(VRLod* lod) {
    setExpanderSensitivity("expander2", true);
    setExpanderSensitivity("expander10", true);

    setTextEntry("entry9", toString(lod->getDecimateNumber()));
    setCheckButton("checkbutton35", lod->getDecimate());
    lodCEntry.set(lod->getCenter());

    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("liststore5"));
    store->clear();

    vector<float> dists = lod->getDistances();
    for(uint i=0; i< dists.size(); i++) {
        cout << "   " << i << dists[i] << endl;
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, i, -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 1, dists[i], -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 2, true, -1);
    }
}

/*void setCSG(CSGGeometry* g) {
    setExpanderSensitivity("expander15", true);

    bool b = g->getEditMode();
    string op = g->getOperation();
    setCheckButton("checkbutton27", b);
    setCombobox("combobox19", getListStorePos("csg_operations",op));
}*/

void on_toggle_liveupdate(GtkToggleButton* tb, gpointer user_data) { liveUpdate = !liveUpdate; }

void updateObjectForms(bool disable = false) {
    setExpanderSensitivity("expander1", false);
    setExpanderSensitivity("expander2", false);
    setExpanderSensitivity("expander9", false);
    setExpanderSensitivity("expander10", false);
    setExpanderSensitivity("expander11", false);
    setExpanderSensitivity("expander12", false);
    setExpanderSensitivity("expander13", false);
    setExpanderSensitivity("expander14", false);
    setExpanderSensitivity("expander15", false);
    setExpanderSensitivity("expander16", false);
    if (disable) return;

    VRObject* obj = getSelected();
    if (obj == 0) return;

    // set object label && path
    setLabel( "current_object_lab", obj->getName() + "\npath " + obj->getPath() );

    string type = obj->getType();
    trigger_cbs = false;

    setObject(obj);
    if (obj->hasAttachment("transform")) setTransform((VRTransform*)obj);
    if (obj->hasAttachment("geometry")) setGeometry((VRGeometry*)obj);

    if (type == "Light") setLight((VRLight*)obj);

    if (type == "Camera") setCamera((VRCamera*)obj);

    if (type == "Group") setGroup((VRGroup*)obj);

    if (type == "Lod") setLod((VRLod*)obj);

    //if (type == "CSGGeometry") setCSG((CSGGeometry*)obj);

    trigger_cbs = true;
}

// --------------------------
// ---------TreeView---------
// --------------------------

class ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        ModelColumns() { add(name); add(type); add(obj); add(fg); add(bg); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<Glib::ustring> obj;
        Gtk::TreeModelColumn<Glib::ustring> fg;
        Gtk::TreeModelColumn<Glib::ustring> bg;
};

class PrimModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        PrimModelColumns() { add(name); add(val); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> val;
};

void getTypeColors(VRObject* o, string& fg, string& bg) {
    fg = "#000000";
    bg = "#FFFFFF";

    if (o->getType() == "Object") fg = "#444444";
    if (o->getType() == "Transform") fg = "#22BB22";
    if (o->getType() == "Geometry") fg = "#DD2222";
    if (o->getType() == "Light") fg = "#DDBB00";
    if (o->getType() == "LightBeacon") fg = "#AA8800";
    if (o->getType() == "Camera") fg = "#2222DD";
    if (o->getType() == "Group") fg = "#00AAAA";
    if (o->getType() == "Lod") fg = "#9900EE";

    if (!o->isVisible()) bg = "#EEEEEE";
}

void setSGRow(Gtk::TreeModel::iterator itr, VRObject* o) {
    if (o == 0) return;
    if (!itr) return;

    string fg, bg;
    getTypeColors(o, fg, bg);

    Gtk::TreeStore::Row row = *itr;
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 0, o->getName().c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 1, o->getType().c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 2, o->getPath().c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 3, fg.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 4, bg.c_str(), -1);
}

void parseSGTree(VRObject* o, Gtk::TreeModel::iterator itr) {
    if (o == 0) return;
    if (!itr) { parseSGTree(o); return; }
    itr = tree_store->append(itr->children());
    setSGRow( itr, o );
    for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), itr );
}

void parseSGTree(VRObject* o) {
    if (o == 0) return;
    Gtk::TreeModel::iterator itr = tree_store->append();
    setSGRow( itr, o );
    for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), itr );
}

void removeTreeStoreBranch(Gtk::TreeModel::iterator iter, bool self = true) {
    int N = tree_store->iter_depth(iter);

    if (!self) {
        if (iter->children().size() == 0) return;
        iter = iter->children()[0];
    } else iter = tree_store->erase(iter); //returns next row

    while(tree_store->iter_depth(iter) > N) {
        iter = tree_store->erase(iter); //returns next row
    }
}

void syncSGTree(VRObject* o, Gtk::TreeModel::iterator itr) {
    if (o == 0) return;

    removeTreeStoreBranch(itr, false);

    for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), itr );
}

// --------------------------
// ---------ObjectForms Callbacks
// --------------------------

// VRObjects
void on_toggle_visible(GtkToggleButton* tb, gpointer data) {
    if(trigger_cbs) getSelected()->toggleVisible();
    setSGRow(selected_itr, getSelected());
}
void on_toggle_pickable(GtkToggleButton* tb, gpointer data) { if(trigger_cbs) getSelected()->setPickable(!getSelected()->isPickable()); }

// VRGroup
void on_groupsync_clicked(GtkButton*, gpointer data) {
    if(!trigger_cbs) return;
    if(!selected_itr) return;

    VRGroup* obj = (VRGroup*) getSelected();
    obj->sync();
    syncSGTree(obj, selected_itr);
}

void on_scene_update(GtkButton*, gpointer data) {
    VRGuiScene* mgr = (VRGuiScene*) data;
    mgr->updateTreeView();
}

void on_groupapply_clicked(GtkButton*, gpointer data) {
    if(!trigger_cbs) return;
    if(!selected_itr) return;

    VRGroup* obj = (VRGroup*) getSelected();
    obj->apply();

    //VRGuiScene* mgr = (VRGuiScene*) data;

    vector<VRGroup*>* grps = obj->getGroupObjects();
    for (uint i=0; i< grps->size(); i++) {
        string path = grps->at(i)->getPath();
        Gtk::TreeModel::iterator itr = tree_store->get_iter(path);
        syncSGTree(grps->at(i), itr);
    }
}

void VRGuiScene_on_change_group(GtkComboBox* cb, gpointer data) {
    if(!trigger_cbs) return;

    VRGroup* obj = (VRGroup*) getSelected();
    obj->setGroup(getComboboxText("combobox14"));
}

//void VRGuiScene_on_group_edited(GtkCellRendererText *cell, gchar *path_string, gchar *_new_group, gpointer d) { //TODO
//void VRGuiScene_on_group_edited(Glib::ustring path_string, Glib::ustring _new_group) { //TODO
void VRGuiScene_on_group_edited(GtkEntry* e, gpointer data) {
    if(!trigger_cbs) return;

    VRGroup* obj = (VRGroup*) getSelected();
    string old_group = getComboboxText("combobox14");
    string new_group = gtk_entry_get_text(e);

    // update group list
    Glib::RefPtr<Gtk::ListStore> list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("liststore3"));
    Gtk::TreeModel::Row row = *list->append();
    ModelColumns mcols;
    row[mcols.name] = new_group;

    // add group
    obj->setGroup(new_group);
    setComboboxLastActive("combobox14");

    setTextEntry("entry41", "");
}

// VR3DEntities
void on_change_from(Vec3f v) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();
    obj->setFrom(v);
    updateObjectForms();
}

void on_change_at(Vec3f v) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();
    obj->setAt(v);
    updateObjectForms();
}

void on_change_dir(Vec3f v) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();
    obj->setDir(v);
    updateObjectForms();
}

void on_change_up(Vec3f v) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();
    obj->setUp(v);
    updateObjectForms();
}

void on_change_lod_center(Vec3f v) {
    if(!trigger_cbs) return;
    VRLod* obj = (VRLod*) getSelected();
    obj->setCenter(v);
    updateObjectForms();
}

void on_scale_changed(Vec3f v) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();
    obj->setScale(v);
    updateObjectForms();
}

void on_focus_clicked(GtkButton*, gpointer data) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();

    VRSceneManager::getCurrent()->getActiveCamera()->setAt( obj->getWorldPosition() );
}

void on_identity_clicked(GtkButton*, gpointer data) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();
    obj->setPose(Vec3f(0,0,0), Vec3f(0,0,-1), Vec3f(0,1,0));
    updateObjectForms();
}

void on_edit_T_constraint(Vec3f v) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();
    obj->setTConstraint(v);
}

void on_toggle_T_constraint(GtkToggleButton* tb, gpointer data) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();

    bool b = getCheckButtonState("checkbutton21");
    obj->toggleTConstraint(b);
}

void on_toggle_R_constraint(GtkToggleButton* tb, gpointer data) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();

    bool b = getCheckButtonState("checkbutton22");
    obj->toggleRConstraint(b);
}

void on_toggle_rc_x(GtkToggleButton* tb, gpointer data) {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();

    Vec3i rc;
    if (getCheckButtonState("checkbutton18") ) rc[0] = 1;
    if (getCheckButtonState("checkbutton19") ) rc[1] = 1;
    if (getCheckButtonState("checkbutton20") ) rc[2] = 1;

    obj->setRConstraint(rc);
}

void on_toggle_rc_y(GtkToggleButton* tb, gpointer data) { on_toggle_rc_x(0,NULL); }

void on_toggle_rc_z(GtkToggleButton* tb, gpointer data) { on_toggle_rc_x(0,NULL); }

// geometry
void on_change_primitive(GtkComboBox* cb, gpointer data) {
    if(!trigger_cbs) return;
    string prim = getComboboxText("combobox21");

    VRGeometry* obj = (VRGeometry*) getSelected();

    obj->setPrimitive(prim);
    updateObjectForms();
}

void on_edit_primitive_params(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer d) {
    if(!trigger_cbs) return;
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview12"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    string prim = getComboboxText("combobox21");
    string args;

    PrimModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.val] = new_text; // set the cell with new name

    Glib::RefPtr<Gtk::ListStore> store  = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("primitive_opts"));
    int N = gtk_tree_model_iter_n_children( (GtkTreeModel*) store->gobj(), NULL );
    for (int i=0; i<N; i++) {
        string _i = toString(i);
        iter = store->get_iter(_i.c_str());
        if (!iter) continue;

        row = *iter;
        string c = row.get_value(cols.val);
        args += c;
        if (i<N-1) args += " ";
    }

    VRGeometry* obj = (VRGeometry*) getSelected();
    obj->setPrimitive(prim, args);
}

// VRLod
class LodModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        LodModelColumns() { add(child); add(distance); add(active); }

        Gtk::TreeModelColumn<int> child;
        Gtk::TreeModelColumn<float> distance;
        Gtk::TreeModelColumn<bool> active;
};

void on_edit_distance(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer d) {
    if(!trigger_cbs) return;
    VRLod* lod = (VRLod*) getSelected();

    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview8"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    float f = toFloat(string(new_text));
    int i = toInt(path_string);
    lod->setDistance(i,f);

    // set the cell with new name
    LodModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.distance] = f;
}

void VRGuiScene::on_lod_decimate_changed() {
    if(!trigger_cbs) return;
    VRLod* lod = (VRLod*) getSelected();
    lod->setDecimate(getCheckButtonState("checkbutton35"), toInt(getTextEntry("entry9")));
}

// CSG

/*void on_toggle_CSG_editmode(GtkToggleButton* tb, gpointer data) {
    if(!trigger_cbs) return;
    CSGGeometry* obj = (CSGGeometry*) getSelected();

    bool b = getCheckButtonState("checkbutton27");
    obj->setEditMode(b);
}

void on_change_CSG_operation(GtkComboBox* cb, gpointer data) {
    if(!trigger_cbs) return;
    string op = getComboboxText("combobox19");

    CSGGeometry* obj = (CSGGeometry*) getSelected();

    obj->setOperation(op);
}*/

// Camera

void on_toggle_camera_accept_realroot(GtkToggleButton* tb, gpointer data) {
    if(!trigger_cbs) return;
    VRCamera* obj = (VRCamera*) getSelected();

    bool b = getCheckButtonState("checkbutton17");
    obj->setAcceptRoot(b);
}




// --------------------------
// ---------TreeViewCallbacks
// --------------------------

void on_treeview_select(GtkTreeView* tv, gpointer user_data) {
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;
    setTableSensitivity("table11", true);

    ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;

    //string name = row.get_value(cols.name);
    //string type = row.get_value(cols.type);
    updateObjectForms(true);
    selected = row.get_value(cols.obj);
    selected_itr = iter;
    updateObjectForms();

    selected_geometry = 0;
    if (getSelected() == 0) return;
    if (getSelected()->hasAttachment("geometry")) selected_geometry = (VRGeometry*)getSelected();
}

void VRGuiScene::on_edit_object_name(string path, string new_text) {
    if(!trigger_cbs) return;
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview6"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new name
    ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.name] = new_text;

    // do something
    getSelected()->setName(new_text);
    row[cols.name] = getSelected()->getName();
    updateObjectForms();
    if (getSelected()->getType() == "Camera") VRGuiSignals::get()->getSignal("camera_added")->trigger<VRDevice>();
}

template <class T>
void VRGuiScene::on_menu_add() {
    if(!selected_itr) return;
    T* obj = new T("None");
    getSelected()->addChild(obj);
    parseSGTree(obj, selected_itr);
}

void VRGuiScene::on_menu_add_animation() {
    if(!selected_itr) return;
    //VRAnimation* obj = new VRAnimation("None");
    //getSelected()->addChild(obj);
    //parseSGTree(obj, selected_itr);
}

void VRGuiScene::on_menu_add_file() {
    if(!selected_itr) return;
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;
    VRGuiFile::gotoPath( scene->getWorkdir() );
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRGuiScene::on_collada_import_clicked) );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("All", "*");
    VRGuiFile::addFilter("COLLADA", "*.dae");
    VRGuiFile::addFilter("VRML", "*.wrl");
    VRGuiFile::addFilter("3DS", "*.3ds");
    VRGuiFile::addFilter("OBJ", "*.obj");
    VRGuiFile::open( "Load", Gtk::FILE_CHOOSER_ACTION_OPEN, "Load geometric data" );
}

void VRGuiScene::on_menu_add_light() {
    if(!selected_itr) return;
    VRLight* light = VRSceneManager::getCurrent()->addLight("light");
    VRLightBeacon* lb = new VRLightBeacon("light_beacon");
    light->addChild(lb);
    light->setBeacon(lb);
    getSelected()->addChild(light);
    parseSGTree(light, selected_itr);
}

void VRGuiScene::on_menu_add_camera() {
    if(!selected_itr) return;
    VRTransform* cam = VRSceneManager::getCurrent()->addCamera("camera");
    getSelected()->addChild(cam);
    parseSGTree(cam, selected_itr);
    VRGuiSignals::get()->getSignal("camera_added")->trigger<VRDevice>();
}

void VRGuiScene::on_menu_add_primitive(string s) {
    if(!selected_itr) return;

    VRGeometry* geo = new VRGeometry(s);
    geo->setPrimitive(s);

    getSelected()->addChild(geo);
    parseSGTree(geo, selected_itr);
}

void VRGuiScene::on_menu_delete() {
    if(!selected_itr) return;
    if (getSelected()->hasAttachment("global")) return;
    // todo: check for camera!!

    string msg1 = "Delete object " + getSelected()->getName();
    if (!askUser(msg1, "Are you sure you want to delete this object?")) return;
    getSelected()->destroy();
    selected = "";

    Glib::RefPtr<Gtk::TreeModel> model = tree_view->get_model();
    removeTreeStoreBranch(selected_itr);
}

void VRGuiScene::on_menu_copy() {
    if(!selected_itr) return;
    VRGuiScene_copied = getSelected();
}

void VRGuiScene::on_menu_paste() {
    if(!selected_itr) return;
    if (VRGuiScene_copied == 0) return;

    VRObject* tmp = VRGuiScene_copied->duplicate();
    tmp->switchParent(getSelected());
    VRGuiScene_copied = 0;

    parseSGTree(tmp, selected_itr);
}

/*void VRGuiScene::on_menu_add_csg() {
    if(!selected_itr) return;
    CSGGeometry* g = new CSGGeometry("csg_geo");
    getSelected()->addChild(g);
    parseSGTree(g, selected_itr);
}*/

void VRGuiScene::on_collada_import_clicked() {
    string rel_path = VRGuiFile::getRelativePath_toWorkdir();
    string path = VRGuiFile::getPath(); // absolute path
    cout << "Data import: " << path << ", relative: " << rel_path << endl;
    VRGuiFile::close();

    // import stuff
    VRObject* tmp = VRSceneLoader::get()->load3DContent(rel_path, getSelected(), cache_override);
    parseSGTree(tmp, selected_itr);
}

void VRGuiScene::on_toggle_cache_override() {
    cache_override = getCheckButtonState("cache_override");
}

class TestDnDcols : public Gtk::TreeModel::ColumnRecord {
    public:
        TestDnDcols() { add(test); }
        Gtk::TreeModelColumn<Glib::ustring> test;
};

void test_tree_dnd() {
    Gtk::Window* win = Gtk::manage( new Gtk::Window() );
    Gtk::TreeView* tview = Gtk::manage( new Gtk::TreeView() );
    Glib::RefPtr<Gtk::TreeStore> tstore;

    win->set_title("Gtk::TreeView DnD test");
    win->set_default_size(400, 200);
    win->add(*tview);

    TestDnDcols columns;
    tstore = Gtk::TreeStore::create(columns);
    tview->set_model(tstore);

    Gtk::TreeModel::Row row;
    row = *(tstore->append()); row[columns.test] = "AAA";
    row = *(tstore->append()); row[columns.test] = "BBB";
    row = *(tstore->append()); row[columns.test] = "CCC";
    row = *(tstore->append()); row[columns.test] = "DDD";

    tview->append_column("test", columns.test);

    tview->enable_model_drag_source();
    tview->enable_model_drag_dest();

    win->show_all();
}

void VRGuiScene::on_drag_end(const Glib::RefPtr<Gdk::DragContext>& dc) {
    Gdk::DragAction ac = dc->get_selected_action();
    if (dragDest == 0) return;
    if (ac == 0) return;
    VRObject* selected = getSelected();
    if (selected == 0) return;
    selected->switchParent(dragDest, dragPos);
    cout << "drag_end " << selected->getPath() << endl;
    Gtk::TreeModel::iterator iter = tree_view->get_model()->get_iter(selected->getPath());
    setSGRow(iter, selected);
}

void VRGuiScene::on_drag_beg(const Glib::RefPtr<Gdk::DragContext>& dc) {
    //cout << "\nDRAG BEGIN " << dc->get_selection() << endl;
}

void VRGuiScene::on_drag_data_receive(const Glib::RefPtr<Gdk::DragContext>& dc , int x, int y ,const Gtk::SelectionData& sd, guint i3, guint i4) {
    Gtk::TreeModel::Path path;
    Gtk::TreeViewDropPosition pos; // enum
    tree_view->get_drag_dest_row(path, pos);
    if (path == 0) return;

    dragDest = 0;
    VRObject* selected = getSelected();
    if (selected == 0) return;

    dragPath = path.to_string();
    dragPos = 0;
    if (pos <= 1) { // between two rows
        int d = dragPath.rfind(':');
        dragPos = toInt( dragPath.substr(d+1) );
        dragPath = dragPath.substr(0,d);
    }
    cout << "drag dest " << dragPath << " " << pos << endl;

    if (selected->hasAttachment("treeviewNotDragable")) { dc->drag_status(Gdk::DragAction(0),0); return; } // object is not dragable
    if (dragPath == "0" && pos <= 1) { dc->drag_status(Gdk::DragAction(0),0); return; } // drag out of root

    /*Gtk::TreeModel::iterator iter = tree_view->get_model()->get_iter(path);
    if(!iter) { dc->drag_status(Gdk::DragAction(0),0); return; }
    ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string dest_path = row.get_value(cols.obj);*/

    dragDest = VRSceneManager::getCurrent()->getRoot()->getAtPath(dragPath);
}

void VRGuiScene_on_notebook_switched(GtkNotebook* notebook, GtkNotebookPage* page, guint pageN, gpointer data) {
    if (pageN == 1) {
        VRScene* scene = VRSceneManager::getCurrent();
        if (scene == 0) return;

        tree_store->clear();
        VRObject* root = scene->getRoot();
        parseSGTree( root );

        tree_view->expand_all();
    }
}

// ------------- transform -----------------------

void VRGuiScene::on_toggle_T_constraint_mode() {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();

    bool plane = getRadioButtonState("radiobutton2");
    obj->setTConstraintMode(plane);
}

void VRGuiScene::on_toggle_phys() {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();

    bool phys = getCheckButtonState("checkbutton13");
    obj->getPhysics()->setPhysicalized(phys);
    setComboboxSensitivity("combobox8", phys);
}

void VRGuiScene::on_toggle_dynamic() {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();

    bool dyn = getCheckButtonState("checkbutton33");
    obj->getPhysics()->setDynamic(dyn);
}

void VRGuiScene::on_mass_changed() {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();

    string m = getTextEntry("entry59");
    obj->getPhysics()->setMass(toFloat(m));
}

void VRGuiScene::on_change_phys_shape() {
    if(!trigger_cbs) return;
    VRTransform* obj = (VRTransform*) getSelected();
    string t = getComboboxText("combobox8");
    obj->getPhysics()->setShape(t);
}


// ------------- geometry -----------------------
void VRGuiScene::setGeometry_gui() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setGeometry_material() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setGeometry_shadow_toggle() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setGeometry_shadow_type() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setGeometry_type() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setGeometry_file() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setGeometry_object() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setGeometry_vertex_count() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setGeometry_face_count() {
    cout << "\nNot yet implemented\n";
}
// ----------------------------------------------


// ------------- camera -----------------------
void VRGuiScene::on_cam_aspect_changed() {
    if(!trigger_cbs) return;
    VRCamera* obj = (VRCamera*) getSelected();
    string a = getTextEntry("entry60");
    obj->setAspect(toFloat(a));
}

void VRGuiScene::on_cam_fov_changed() {
    if(!trigger_cbs) return;
    VRCamera* obj = (VRCamera*) getSelected();
    string f = getTextEntry("entry61");
    obj->setFov(toFloat(f));
}

void VRGuiScene::on_cam_near_changed() {
    if(!trigger_cbs) return;
    VRCamera* obj = (VRCamera*) getSelected();
    string f = getTextEntry("entry6");
    obj->setNear(toFloat(f));
}

void VRGuiScene::on_cam_far_changed() {
    if(!trigger_cbs) return;
    VRCamera* obj = (VRCamera*) getSelected();
    string f = getTextEntry("entry7");
    obj->setFar(toFloat(f));
}
// ----------------------------------------------

// ------------- light -----------------------
void VRGuiScene::on_toggle_light() {
    if(!trigger_cbs) return;
    VRLight* obj = (VRLight*) getSelected();
    bool b = getCheckButtonState("checkbutton31");
    obj->setOn(b);
}

void VRGuiScene::on_toggle_light_shadow() {
    if(!trigger_cbs) return;
    VRLight* obj = (VRLight*) getSelected();
    bool b = getCheckButtonState("checkbutton32");
    obj->setShadows(b);
}

void VRGuiScene::on_change_light_type() {
    if(!trigger_cbs) return;
    VRLight* obj = (VRLight*) getSelected();
    string t = getComboboxText("combobox2");
    obj->setType(t);
}

void VRGuiScene::on_change_light_shadow() {
    if(!trigger_cbs) return;
    VRLight* obj = (VRLight*) getSelected();
    string t = getComboboxText("combobox22");
    obj->setShadowType(t);
}

void VRGuiScene::on_edit_light_attenuation() {
    if(!trigger_cbs) return;
    VRLight* obj = (VRLight*) getSelected();
    string ac = getTextEntry("entry44");
    string al = getTextEntry("entry45");
    string aq = getTextEntry("entry46");
    obj->setAttenuation(Vec3f(toFloat(ac), toFloat(al), toFloat(aq)));
}

bool VRGuiScene::setShadow_color(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    VRLight* obj = (VRLight*) getSelected();
    Color4f c = chooseColor("shadow_col", obj->getShadowColor());
    obj->setShadowColor(c);
    return true;
}

bool VRGuiScene::setLight_diff_color(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    VRLight* obj = (VRLight*) getSelected();
    Color4f c = chooseColor("light_diff", obj->getLightDiffColor());
    obj->setLightDiffColor(c);
    return true;
}

bool VRGuiScene::setLight_amb_color(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    VRLight* obj = (VRLight*) getSelected();
    Color4f c = chooseColor("light_amb", obj->getLightAmbColor());
    obj->setLightAmbColor(c);
    return true;
}

bool VRGuiScene::setLight_spec_color(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    VRLight* obj = (VRLight*) getSelected();
    Color4f c = chooseColor("light_spec", obj->getLightSpecColor());
    obj->setLightSpecColor(c);
    return true;
}
// ----------------------------------------------


// ------------- material -----------------------
void VRGuiScene::setMaterial_gui() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setMaterial_lit() {
    if(!trigger_cbs) return;
    if(!selected_geometry) return;
    bool b = getCheckButtonState("checkbutton3");
    selected_geometry->getMaterial()->setLit(b);
}

bool VRGuiScene::setMaterial_diffuse(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    if(!selected_geometry) return true;
    Color4f c = toColor4f(selected_geometry->getMaterial()->getDiffuse());
    c[3] = selected_geometry->getMaterial()->getTransparency();
    c = chooseColor("mat_diffuse", c);
    selected_geometry->getMaterial()->setDiffuse(toColor3f(c));
    selected_geometry->getMaterial()->setTransparency(c[3]);
    return true;
}

bool VRGuiScene::setMaterial_specular(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    if(!selected_geometry) return true;
    Color4f c = chooseColor("mat_specular", toColor4f(selected_geometry->getMaterial()->getSpecular()));
    selected_geometry->getMaterial()->setSpecular(toColor3f(c));
    return true;
}

bool VRGuiScene::setMaterial_ambient(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    if(!selected_geometry) return true;
    Color4f c = chooseColor("mat_ambient", toColor4f(selected_geometry->getMaterial()->getAmbient()));
    selected_geometry->getMaterial()->setAmbient(toColor3f(c));
    return true;
}

void VRGuiScene::setMaterial_pointsize() { // TODO
    if(!trigger_cbs) return;
    if(!selected_geometry) return;
    //int ps = 5;
    //selected_geometry->setPointSize(ps);
}

void VRGuiScene::setMaterial_texture_toggle() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setMaterial_texture_name() {
    cout << "\nNot yet implemented\n";
}
// ----------------------------------------------

void VRGuiScene::initMenu() {
    menu = new VRGuiContextMenu("SGMenu");
    menu->connectWidget("SGMenu", tree_view);

    menu->appendMenu("SGMenu", "Add", "SGM_AddMenu");
    menu->appendMenu("SGM_AddMenu", "Primitive", "SGM_AddPrimMenu" );
    menu->appendMenu("SGM_AddMenu", "Urban", "SGM_AddUrbMenu" );
    menu->appendMenu("SGM_AddMenu", "Nature", "SGM_AddNatMenu" );
    menu->appendMenu("SGM_AddUrbMenu", "Building", "SGM_AddUrbBuiMenu" );

    menu->appendItem("SGMenu", "Copy", sigc::mem_fun(*this, &VRGuiScene::on_menu_copy));
    menu->appendItem("SGMenu", "Paste", sigc::mem_fun(*this, &VRGuiScene::on_menu_paste));
    menu->appendItem("SGMenu", "Delete", sigc::mem_fun(*this, &VRGuiScene::on_menu_delete));

    menu->appendItem("SGM_AddMenu", "Object", sigc::mem_fun(*this, &VRGuiScene::on_menu_add<VRObject>) );
    menu->appendItem("SGM_AddMenu", "Transform", sigc::mem_fun(*this, &VRGuiScene::on_menu_add<VRTransform>) );
    menu->appendItem("SGM_AddMenu", "Geometry", sigc::mem_fun(*this, &VRGuiScene::on_menu_add<VRGeometry>) );
    menu->appendItem("SGM_AddMenu", "Light", sigc::mem_fun(*this, &VRGuiScene::on_menu_add_light));
    menu->appendItem("SGM_AddMenu", "Camera", sigc::mem_fun(*this, &VRGuiScene::on_menu_add_camera));
    menu->appendItem("SGM_AddMenu", "Group", sigc::mem_fun(*this, &VRGuiScene::on_menu_add<VRGroup>));
    menu->appendItem("SGM_AddMenu", "LoD", sigc::mem_fun(*this, &VRGuiScene::on_menu_add<VRLod>));
    menu->appendItem("SGM_AddMenu", "Animation", sigc::mem_fun(*this, &VRGuiScene::on_menu_add_animation));
    menu->appendItem("SGM_AddMenu", "File", sigc::mem_fun(*this, &VRGuiScene::on_menu_add_file) );
    //menu->appendItem("SGM_AddMenu", "CSGGeometry", sigc::mem_fun(*this, &VRGuiScene::on_menu_add_csg) );

    // primitives
    vector<string> prims = VRPrimitive::getTypes();
    for (uint i=0; i<prims.size(); i++)
        menu->appendItem("SGM_AddPrimMenu", prims[i], sigc::bind( sigc::mem_fun(*this, &VRGuiScene::on_menu_add_primitive) , prims[i]) );

    // building elements
    //menu->appendItem("SGM_AddUrbBuiMenu", "Opening", sigc::mem_fun(*this, &VRGuiScene::on_menu_add<VROpening>) );
    //menu->appendItem("SGM_AddUrbBuiMenu", "Device", sigc::mem_fun(*this, &VRGuiScene::on_menu_add<VRElectricDevice>) );
}

void VRGuiScene::initCallbacks() {
    Glib::RefPtr<Gtk::CheckButton> cbutton;

    cbutton = Glib::RefPtr<Gtk::CheckButton>::cast_static(VRGuiBuilder()->get_object("checkbutton3"));
    cbutton->signal_toggled().connect( sigc::mem_fun(*this, &VRGuiScene::setMaterial_lit) );
}


// TODO: remove groups
VRGuiScene::VRGuiScene() { // TODO: reduce callbacks with templated functions
    dragDest = 0;

    initCallbacks();

    //test_tree_dnd();

    // treeviewer
    tree_store = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("scenegraph"));
    tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview6"));
    g_signal_connect (tree_view->gobj(), "cursor-changed", G_CALLBACK (on_treeview_select), NULL);

    initMenu();

    tree_view->signal_drag_begin().connect( sigc::mem_fun(*this, &VRGuiScene::on_drag_beg) );
    tree_view->signal_drag_end().connect( sigc::mem_fun(*this, &VRGuiScene::on_drag_end) );
    tree_view->signal_drag_data_received().connect( sigc::mem_fun(*this, &VRGuiScene::on_drag_data_receive) );

    setCellRendererCallback("cellrenderertext7", sigc::mem_fun(*this, &VRGuiScene::on_edit_object_name) );
    setCellRendererCallback("cellrenderertext4", on_edit_distance);
    setCellRendererCallback("cellrenderertext33", on_edit_primitive_params);

    setEntryCallback("entry41", VRGuiScene_on_group_edited);

    //light
    fillStringListstore("light_types", VRLight::getTypes());
    fillStringListstore("shadow_types", VRLight::getShadowTypes());
    //fillStringListstore("csg_operations", CSGGeometry::getOperations());
    fillStringListstore("phys_shapes", VRPhysics::getPhysicsShapes());
    fillStringListstore("cam_proj", VRCamera::getProjectionTypes());

    // object form
    setCheckButtonCallback("checkbutton16", on_toggle_liveupdate);
    setCheckButtonCallback("checkbutton6", on_toggle_visible);
    setCheckButtonCallback("checkbutton15", on_toggle_pickable);
    setCheckButtonCallback("checkbutton18", on_toggle_rc_x);
    setCheckButtonCallback("checkbutton19", on_toggle_rc_y);
    setCheckButtonCallback("checkbutton20", on_toggle_rc_z);
    setCheckButtonCallback("checkbutton21", on_toggle_T_constraint);
    setCheckButtonCallback("checkbutton22", on_toggle_R_constraint);
    //setCheckButtonCallback("checkbutton27", on_toggle_CSG_editmode);
    setCheckButtonCallback("checkbutton17", on_toggle_camera_accept_realroot);
    setCheckButtonCallback("radiobutton1", sigc::mem_fun(*this, &VRGuiScene::on_toggle_T_constraint_mode) );
    setCheckButtonCallback("checkbutton31", sigc::mem_fun(*this, &VRGuiScene::on_toggle_light) );
    setCheckButtonCallback("checkbutton32", sigc::mem_fun(*this, &VRGuiScene::on_toggle_light_shadow) );
    setCheckButtonCallback("checkbutton13", sigc::mem_fun(*this, &VRGuiScene::on_toggle_phys) );
    setCheckButtonCallback("checkbutton33", sigc::mem_fun(*this, &VRGuiScene::on_toggle_dynamic) );
    setCheckButtonCallback("cache_override", sigc::mem_fun(*this, &VRGuiScene::on_toggle_cache_override) );
    setCheckButtonCallback("checkbutton35", sigc::mem_fun(*this, &VRGuiScene::on_lod_decimate_changed) );

    setComboboxCallback("combobox14", VRGuiScene_on_change_group);
    //setComboboxCallback("combobox19", on_change_CSG_operation);
    setComboboxCallback("combobox21", on_change_primitive);
    setComboboxCallback("combobox2", sigc::mem_fun(*this, &VRGuiScene::on_change_light_type) );
    setComboboxCallback("combobox22", sigc::mem_fun(*this, &VRGuiScene::on_change_light_shadow) );
    setComboboxCallback("combobox8", sigc::mem_fun(*this, &VRGuiScene::on_change_phys_shape) );

    setEntryCallback("entry44", sigc::mem_fun(*this, &VRGuiScene::on_edit_light_attenuation) );
    setEntryCallback("entry45", sigc::mem_fun(*this, &VRGuiScene::on_edit_light_attenuation) );
    setEntryCallback("entry46", sigc::mem_fun(*this, &VRGuiScene::on_edit_light_attenuation) );

    posEntry.init("pos_entry", "from", sigc::ptr_fun(on_change_from));
    atEntry.init("at_entry", "at", sigc::ptr_fun(on_change_at));
    dirEntry.init("dir_entry", "dir", sigc::ptr_fun(on_change_dir));
    upEntry.init("up_entry", "up", sigc::ptr_fun(on_change_up));
    scaleEntry.init("scale_entry", "scale", sigc::ptr_fun(on_scale_changed));
    lodCEntry.init("lod_center", "center", sigc::ptr_fun(on_change_lod_center));
    ctEntry.init("ct_entry", "", sigc::ptr_fun(on_edit_T_constraint));

    setEntryCallback("entry59", sigc::mem_fun(*this, &VRGuiScene::on_mass_changed) );
    setEntryCallback("entry60", sigc::mem_fun(*this, &VRGuiScene::on_cam_aspect_changed) );
    setEntryCallback("entry61", sigc::mem_fun(*this, &VRGuiScene::on_cam_fov_changed) );
    setEntryCallback("entry6", sigc::mem_fun(*this, &VRGuiScene::on_cam_near_changed) );
    setEntryCallback("entry7", sigc::mem_fun(*this, &VRGuiScene::on_cam_far_changed) );
    setEntryCallback("entry9", sigc::mem_fun(*this, &VRGuiScene::on_lod_decimate_changed) );

    setButtonCallback("button4", on_focus_clicked);
    setButtonCallback("button11", on_identity_clicked);
    setButtonCallback("button19", on_groupsync_clicked);
    setButtonCallback("button20", on_groupapply_clicked, this);
    setButtonCallback("button17", on_scene_update, this);

    setColorChooser("mat_diffuse", sigc::mem_fun(*this, &VRGuiScene::setMaterial_diffuse));
    setColorChooser("mat_specular", sigc::mem_fun(*this, &VRGuiScene::setMaterial_specular));
    setColorChooser("mat_ambient", sigc::mem_fun(*this, &VRGuiScene::setMaterial_ambient));
    setColorChooser("shadow_col", sigc::mem_fun(*this, &VRGuiScene::setShadow_color));
    setColorChooser("light_diff", sigc::mem_fun(*this, &VRGuiScene::setLight_diff_color));
    setColorChooser("light_amb", sigc::mem_fun(*this, &VRGuiScene::setLight_amb_color));
    setColorChooser("light_spec", sigc::mem_fun(*this, &VRGuiScene::setLight_spec_color));

    selected = "";
    updateObjectForms();
}

// new scene, update stuff here
void VRGuiScene::updateTreeView() {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    tree_store->clear();
    VRObject* root = scene->getRoot();
    parseSGTree( root );

    tree_view->expand_all();

    setTableSensitivity("table11", false);
}

// check if currently getSelected() object has been modified
void VRGuiScene::update() {
    if (!liveUpdate) return;
    updateObjectForms();
}

OSG_END_NAMESPACE;
