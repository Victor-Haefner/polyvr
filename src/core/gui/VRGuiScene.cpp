#include "VRGuiScene.h"

#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/table.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/notebook.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/builder.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "core/scene/VRSceneLoader.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRCamera.h"
#include "core/objects/VRGroup.h"
#include "core/objects/VRLod.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/scene/VRScene.h"
#include "core/scene/import/VRImport.h"
#include "core/utils/toString.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include "addons/Semantics/Reasoning/VRConcept.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "addons/construction/building/VRElectricDevice.h"
//#include "addons/Engineering/CSG/CSGGeometry.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

// --------------------------
// ---------ObjectForms------
// --------------------------

VRObjectPtr VRGuiScene::getSelected() {
    if (selected < 0) return 0;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return 0;

    VRObjectPtr root = scene->getRoot();
    //VRObjectPtr res = root->getAtPath(selected);
    VRObjectPtr res = root->find(selected);
    if (!res) return 0;

    /*if (res == 0) {
        cout << "did not find " << selected << endl;
        tree_store->clear();
        parseSGTree( root );
        tree_view->expand_all();
        setTableSensitivity("table11", false);
    }*/

    return res;
}

void VRGuiScene::setObject(VRObjectPtr o) {
    setExpanderSensitivity("expander2", true);

    // set object properties
    setCheckButton("checkbutton6", o->isVisible());
    setCheckButton("checkbutton43", o->isVisible("SHADOW"));
    setCheckButton("checkbutton15", o->isPickable());

    VRObjectPtr parent = o->getParent();
    if (parent) setTextEntry("entry17", parent->getName());
    else setTextEntry("entry17", "");
}

void VRGuiScene::setTransform(VRTransformPtr e) {
    setExpanderSensitivity("expander1", true);

    Vec3d f,a,u,d,s;
    if (transformModeLocal) {
        f = e->getFrom();
        a = e->getAt();
        u = e->getUp();
        d = e->getDir();
        s = e->getScale();
    } else {
        f = e->getWorldPosition();
        a = e->getWorldAt();
        u = e->getWorldUp();
        d = e->getWorldDirection();
        //s = e->getWorldScale(); TODO!
        s = e->getScale();
    }

    /*auto c = e->getConstraint();
    Vec3d tc = c->getTConstraint();
    Vec3d rc = c->getRConstraint();*/

    posEntry.set(f);
    atEntry.set(a);
    dirEntry.set(d);
    upEntry.set(u);
    scaleEntry.set(s);
    //ctEntry.set(tc);

    atEntry.setFontColor(Vec3d(0, 0, 0));
    dirEntry.setFontColor(Vec3d(0, 0, 0));
    if (e->get_orientation_mode())  atEntry.setFontColor(Vec3d(0.6, 0.6, 0.6));
    else                            dirEntry.setFontColor(Vec3d(0.6, 0.6, 0.6));

    /*bool doTc = c->hasTConstraint();
    bool doRc = c->hasRConstraint();

    setCheckButton("checkbutton18", rc[0]);
    setCheckButton("checkbutton19", rc[1]);
    setCheckButton("checkbutton20", rc[2]);

    setCheckButton("checkbutton21", doTc);
    setCheckButton("checkbutton22", doRc);

    setRadioButton("radiobutton1", !c->getTMode());
    setRadioButton("radiobutton2", c->getTMode());*/

    setRadioButton("radiobutton19",  transformModeLocal);
    setRadioButton("radiobutton20", !transformModeLocal);

    if (e->getPhysics()) {
        setCheckButton("checkbutton13", e->getPhysics()->isPhysicalized());
        setCheckButton("checkbutton33", e->getPhysics()->isDynamic());
        setTextEntry("entry59", toString(e->getPhysics()->getMass()));
        setCombobox("combobox8", getListStorePos("phys_shapes", e->getPhysics()->getShape()));
        setComboboxSensitivity("combobox8", e->getPhysics()->isPhysicalized());
    }
}

void VRGuiScene::setMaterial(VRMaterialPtr mat) {
    bool lit = false;
    Color3f _cd, _cs, _ca;
    VRTexturePtr tex;

    if (mat) {
        setLabel("label60", mat->getName());
        _cd = mat->getDiffuse();
        _cs = mat->getSpecular();
        _ca = mat->getAmbient();
        lit = mat->isLit();
        tex = mat->getTexture();
    } else setLabel("label60", "NONE");

    setColorChooserColor("mat_diffuse", _cd);
    setColorChooserColor("mat_specular", _cs);
    setColorChooserColor("mat_ambient", _ca);

    setCheckButton("checkbutton3", lit);
    setCheckButton("checkbutton5", bool(tex));
    setTableSensitivity("table44", bool(tex));

    if (tex) {
        setLabel("label158", toString(tex->getSize()) + " (" + toString(tex->getByteSize()/1048576.0) + " mb)");
        setLabel("label157", toString(tex->getChannels()));
    }
}

void VRGuiScene::setGeometry(VRGeometryPtr g) {
    setExpanderSensitivity("expander11", true);
    setExpanderSensitivity("expander14", true);
    setExpanderSensitivity("expander16", true);
    VRMaterialPtr mat = g->getMaterial();
    setMaterial(mat);

    setCheckButton("checkbutton28", false);
    setCombobox("combobox21", -1);

    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("primitive_opts"));
    store->clear();
    stringstream params;
    params << g->getReference().parameter;

    switch (g->getReference().type) {
        case VRGeometry::FILE:
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

    auto appendGeoData = [](Glib::RefPtr<Gtk::ListStore>& store, string name, int N) {
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, name.c_str(), -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 1, N, -1);
    };

    VRGeoData data(g);
    store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("geodata"));
    store->clear();
    for (int i=0; i<=8; i++) {
        int N = data.getDataSize(i);
        if (N) appendGeoData(store, data.getDataName(i), N);
    }
}

void VRGuiScene::setLight(VRLightPtr l) {
    setExpanderSensitivity("expander13", true);

    setCheckButton("checkbutton31", l->isOn());
    setCheckButton("checkbutton32", l->getShadows());
    setCheckButton("checkbutton2", l->getShadowVolume().volume() > 1e-4);
    setCombobox("combobox2", getListStorePos("light_types", l->getLightType()));
    setCombobox("combobox22", getListStorePos("shadow_types", toString(l->getShadowMapRes())));

    setColorChooserColor("shadow_col", toColor3f(l->getShadowColor()));
    setColorChooserColor("light_diff", toColor3f(l->getDiffuse()));
    setColorChooserColor("light_amb", toColor3f(l->getAmbient()));
    setColorChooserColor("light_spec", toColor3f(l->getSpecular()));

    Vec3d a = l->getAttenuation();
    setTextEntry("entry44", toString(a[0]));
    setTextEntry("entry45", toString(a[1]));
    setTextEntry("entry46", toString(a[2]));
    setTextEntry("entry36", toString(l->getShadowVolume().size()[0]));

    string bname = "NONE";
    auto beacon = l->getBeacon();
    if (beacon) bname = beacon->getName();
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

void VRGuiScene::setCamera(VRCameraPtr c) {
    setExpanderSensitivity("expander12", true);
    setCheckButton("checkbutton17", c->getAcceptRoot());
    setTextEntry("entry60", toString(c->getAspect()));
    setTextEntry("entry61", toString(c->getFov()));
    setTextEntry("entry6", toString(c->getNear()));
    setTextEntry("entry7", toString(c->getFar()));
}

void VRGuiScene::setGroup(VRGroupPtr g) {
    setExpanderSensitivity("expander2", true);
    setExpanderSensitivity("expander9", true);
    setCheckButton("checkbutton23", g->getActive() );

    fillStringListstore("liststore3", g->getGroups());

    setCombobox("combobox14", getListStorePos("liststore3",g->getGroup()));
}

void VRGuiScene::setLod(VRLodPtr lod) {
    setExpanderSensitivity("expander2", true);
    setExpanderSensitivity("expander10", true);

    setTextEntry("entry9", toString(lod->getDecimateNumber()));
    setCheckButton("checkbutton35", lod->getDecimate());
    lodCEntry.set(lod->getCenter());

    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("liststore5"));
    store->clear();

    vector<float> dists = lod->getDistances();
    for(uint i=0; i< dists.size(); i++) {
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, i, -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 1, dists[i], -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 2, true, -1);
    }
}

void VRGuiScene::setEntity(VREntityPtr e) {
    setExpanderSensitivity("expander27", true);

    setLabel("label145", e->getConceptList());

    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("properties"));
    store->clear();

    for(auto pvec : e->properties) {
        for (auto p : pvec.second) {
            Gtk::ListStore::Row row = *store->append();
            gtk_list_store_set (store->gobj(), row.gobj(), 0, pvec.first.c_str(), -1);
            gtk_list_store_set (store->gobj(), row.gobj(), 1, p->value.c_str(), -1);
            gtk_list_store_set (store->gobj(), row.gobj(), 2, p->type.c_str(), -1);
        }
    }
}

/*void setCSG(CSGGeometryPtr g) {
    setExpanderSensitivity("expander15", true);

    bool b = g->getEditMode();
    string op = g->getOperation();
    setCheckButton("checkbutton27", b);
    setCombobox("combobox19", getListStorePos("csg_operations",op));
}*/

void VRGuiScene::on_toggle_liveupdate() { liveUpdate = !liveUpdate; }

void VRGuiScene::updateObjectForms(bool disable) {
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
    setExpanderSensitivity("expander27", false);
    if (disable) return;

    VRObjectPtr obj = getSelected();
    if (obj == 0) return;

    // set object label && path
    setLabel( "current_object_lab", obj->getName() + "\npath " + obj->getPath() );

    string type = obj->getType();
    trigger_cbs = false;

    setObject(obj);
    if (obj->hasTag("transform")) setTransform(static_pointer_cast<VRTransform>(obj));
    if (obj->hasTag("geometry")) setGeometry(static_pointer_cast<VRGeometry>(obj));

    if (type == "Light") setLight(static_pointer_cast<VRLight>(obj));
    if (type == "Camera") setCamera(static_pointer_cast<VRCamera>(obj));
    if (type == "Group") setGroup(static_pointer_cast<VRGroup>(obj));
    if (type == "Lod") setLod(static_pointer_cast<VRLod>(obj));

    auto entity = obj->getEntity();
    if (entity) setEntity(entity);

    //if (type == "CSGGeometry") setCSG((CSGGeometryPtr)obj);

    trigger_cbs = true;
}

// --------------------------
// ---------TreeView---------
// --------------------------

class ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        ModelColumns() { add(name); add(type); add(obj); add(fg); add(bg); add(ID); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<Glib::ustring> obj;
        Gtk::TreeModelColumn<Glib::ustring> fg;
        Gtk::TreeModelColumn<Glib::ustring> bg;
        Gtk::TreeModelColumn<int> ID;
};

class PrimModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        PrimModelColumns() { add(name); add(val); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> val;
};

void VRGuiScene::getTypeColors(VRObjectPtr o, string& fg, string& bg) {
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

void VRGuiScene::setSGRow(Gtk::TreeModel::iterator itr, VRObjectPtr o) {
    if (o == 0) return;
    if (!itr) return;

    string fg, bg;
    getTypeColors(o, fg, bg);

    auto e = o->getEntity();

    string name = o->getName();
    if (e) name += "<sub><u><span foreground=\"blue\">" + e->getConceptList() + "</span></u></sub>";

    Gtk::TreeStore::Row row = *itr;
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 0, name.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 1, o->getType().c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 2, o->getPath().c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 3, fg.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 4, bg.c_str(), -1);
    gtk_tree_store_set (tree_store->gobj(), row.gobj(), 5, o->getID(), -1);
}

void VRGuiScene::parseSGTree(VRObjectPtr o, Gtk::TreeModel::iterator itr) {
    if (o == 0) return;
    if (!itr) itr = tree_store->append();
    else itr = tree_store->append(itr->children());
    setSGRow( itr, o );
    for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), itr );
}

void VRGuiScene::removeTreeStoreBranch(Gtk::TreeModel::iterator iter, bool self) {
    int N = tree_store->iter_depth(iter);

    if (!self) {
        if (iter->children().size() == 0) return;
        iter = iter->children()[0];
    } else iter = tree_store->erase(iter); //returns next row

    while (tree_store->iter_depth(iter) > N) iter = tree_store->erase(iter); //returns next row
}

void VRGuiScene::syncSGTree(VRObjectPtr o, Gtk::TreeModel::iterator itr) {
    if (o == 0) return;
    removeTreeStoreBranch(itr, false);
    for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), itr );
}

void VRGuiScene::on_treeview_select() {
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;
    setTableSensitivity("table11", true);

    ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;

    //string name = row.get_value(cols.name);
    //string type = row.get_value(cols.type);
    updateObjectForms(true);
    //selected = row.get_value(cols.obj);
    selected = row.get_value(cols.ID);
    selected_itr = iter;
    updateObjectForms();

    selected_geometry.reset();
    if (getSelected() == 0) return;
    if (getSelected()->hasTag("geometry")) selected_geometry = static_pointer_cast<VRGeometry>(getSelected());
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
    if (getSelected()->getType() == "Camera") VRGuiSignals::get()->getSignal("camera_added")->triggerPtr<VRDevice>();
}

// --------------------------
// ---------ObjectForms Callbacks
// --------------------------

// VRObjects
void VRGuiScene::on_toggle_visible() {
    if (trigger_cbs) getSelected()->toggleVisible();
    setSGRow(selected_itr, getSelected());
}

void VRGuiScene::on_toggle_throw_shadow() {
    if (trigger_cbs) getSelected()->toggleVisible("SHADOW");
}

void VRGuiScene::on_toggle_pickable() { if (trigger_cbs) getSelected()->setPickable(!getSelected()->isPickable()); }

// VRGroup
void VRGuiScene::on_groupsync_clicked() {
    if(!trigger_cbs) return;
    if(!selected_itr) return;

    VRGroupPtr obj = static_pointer_cast<VRGroup>( getSelected() );
    obj->sync();
    syncSGTree(obj, selected_itr);
}

void VRGuiScene::on_scene_update() {
    updateTreeView();
}

void VRGuiScene::on_groupapply_clicked() {
    if(!trigger_cbs) return;
    if(!selected_itr) return;

    VRGroupPtr obj = static_pointer_cast<VRGroup>( getSelected() );
    obj->apply();

    vector<VRGroupWeakPtr> grps = obj->getGroupObjects();
    for (auto grp : grps) {
        auto g = grp.lock();
        if (!g) continue;
        string path = g->getPath();
        syncSGTree(g, tree_store->get_iter(path));
    }
}

void VRGuiScene::on_change_group() {
    if(!trigger_cbs) return;

    VRGroupPtr obj = static_pointer_cast<VRGroup>( getSelected() );
    obj->setGroup(getComboboxText("combobox14"));
}

void VRGuiScene::on_group_edited() {
    if(!trigger_cbs) return;

    VRGroupPtr obj = static_pointer_cast<VRGroup>( getSelected() );
    string old_group = getComboboxText("combobox14");
    string new_group = getTextEntry("entry41");

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
void VRGuiScene::on_change_from(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (transformModeLocal) obj->setFrom(v);
    else obj->setWorldPosition(v);
    updateObjectForms();
}

void VRGuiScene::on_change_at(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (transformModeLocal) obj->setAt(v);
    else obj->setWorldAt(v);
    updateObjectForms();
}

void VRGuiScene::on_change_dir(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (transformModeLocal) obj->setDir(v);
    else obj->setWorldDir(v);
    updateObjectForms();
}

void VRGuiScene::on_change_up(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (transformModeLocal) obj->setUp(v);
    else obj->setWorldUp(v);
    updateObjectForms();
}

void VRGuiScene::on_scale_changed(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (transformModeLocal) obj->setScale(v);
    //else obj->setWorldScale(v); TODO
    else obj->setScale(v);
    updateObjectForms();
}

void VRGuiScene::on_change_lod_center(Vec3d v) {
    if(!trigger_cbs) return;
    VRLodPtr obj = static_pointer_cast<VRLod>( getSelected() );
    obj->setCenter(v);
    updateObjectForms();
}

void VRGuiScene::on_focus_clicked() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    auto scene = VRScene::getCurrent();
    if (scene) scene->getActiveCamera()->focusObject( obj );
}

void VRGuiScene::on_identity_clicked() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    obj->setIdentity();
    updateObjectForms();
}

void VRGuiScene::on_edit_T_constraint(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    //obj->getConstraint()->setTConstraint(v, obj->getConstraint()->getTMode());
}

void VRGuiScene::on_toggle_T_constraint() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    bool bT = getCheckButtonState("checkbutton21");
    bool bR = getCheckButtonState("checkbutton22");
    obj->getConstraint()->setActive(bT || bR);
}

void VRGuiScene::on_toggle_R_constraint() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    bool bT = getCheckButtonState("checkbutton21");
    bool bR = getCheckButtonState("checkbutton22");
    obj->getConstraint()->setActive(bT || bR);
}

void VRGuiScene::on_toggle_rc() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    Vec3d rc;
    if (getCheckButtonState("checkbutton18") ) rc[0] = 1;
    if (getCheckButtonState("checkbutton19") ) rc[1] = 1;
    if (getCheckButtonState("checkbutton20") ) rc[2] = 1;

    //obj->getConstraint()->setRConstraint(rc, obj->getConstraint()->getRMode());
}

// geometry
void VRGuiScene::on_change_primitive() {
    if(!trigger_cbs) return;
    string prim = getComboboxText("combobox21");

    VRGeometryPtr obj = static_pointer_cast<VRGeometry>( getSelected() );

    obj->setPrimitive(prim);
    updateObjectForms();
}

void VRGuiScene::on_edit_primitive_params(string path, string new_text) {
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

    VRGeometryPtr obj = static_pointer_cast<VRGeometry>( getSelected() );
    obj->setPrimitive(prim + " " + args);
}

// VRLod
class LodModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        LodModelColumns() { add(child); add(distance); add(active); }

        Gtk::TreeModelColumn<int> child;
        Gtk::TreeModelColumn<float> distance;
        Gtk::TreeModelColumn<bool> active;
};

void VRGuiScene::on_edit_distance(string path, string new_text) {
//void VRGuiScene::on_edit_distance(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer d) {
    if(!trigger_cbs) return;
    VRLodPtr lod = static_pointer_cast<VRLod>( getSelected() );

    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview8"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    float f = toFloat(new_text);
    int i = toInt(path);
    lod->setDistance(i,f);

    // set the cell with new name
    LodModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.distance] = f;
}

void VRGuiScene::on_lod_decimate_changed() {
    if(!trigger_cbs) return;
    VRLodPtr lod = static_pointer_cast<VRLod>( getSelected() );
    lod->setDecimate(getCheckButtonState("checkbutton35"), toInt(getTextEntry("entry9")));
}

// CSG

/*void on_toggle_CSG_editmode(GtkToggleButton* tb, gpointer data) {
    if(!trigger_cbs) return;
    CSGGeometryPtr obj = (CSGGeometryPtr) getSelected();

    bool b = getCheckButtonState("checkbutton27");
    obj->setEditMode(b);
}

void on_change_CSG_operation(GtkComboBox* cb, gpointer data) {
    if(!trigger_cbs) return;
    string op = getComboboxText("combobox19");

    CSGGeometryPtr obj = (CSGGeometryPtr) getSelected();

    obj->setOperation(op);
}*/

// Camera

void VRGuiScene::on_toggle_camera_accept_realroot() {
    if(!trigger_cbs) return;
    VRCameraPtr obj = static_pointer_cast<VRCamera>( getSelected() );

    bool b = getCheckButtonState("checkbutton17");
    obj->setAcceptRoot(b);
}




// --------------------------
// ---------Menu callbacks
// --------------------------

template <class T>
void VRGuiScene::on_menu_add() {
    if(!selected_itr) return;
    auto obj = T::create("None");
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
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    VRGuiFile::gotoPath( scene->getWorkdir() );
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRGuiScene::on_collada_import_clicked) );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::addFilter("COLLADA", 1, "*.dae");
    VRGuiFile::addFilter("VRML", 1, "*.wrl");
    VRGuiFile::addFilter("3DS", 1, "*.3ds");
    VRGuiFile::addFilter("OBJ", 1, "*.obj");
    VRGuiFile::setGeoLoadWidget();
    VRGuiFile::open( "Load", Gtk::FILE_CHOOSER_ACTION_OPEN, "Load geometric data" );
}

void VRGuiScene::on_menu_add_light() {
    if(!selected_itr) return;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    VRLightPtr light = VRLight::create("light");
    VRLightBeaconPtr lb = VRLightBeacon::create("light_beacon");
    light->addChild(lb);
    light->setBeacon(lb);
    getSelected()->addChild(light);
    parseSGTree(light, selected_itr);
}

void VRGuiScene::on_menu_add_camera() {
    if(!selected_itr) return;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    VRTransformPtr cam = VRCamera::create("camera");
    getSelected()->addChild(cam);
    parseSGTree(cam, selected_itr);
    VRGuiSignals::get()->getSignal("camera_added")->triggerPtr<VRDevice>();
}

void VRGuiScene::on_menu_add_primitive(string s) {
    if(!selected_itr) return;

    VRGeometryPtr geo = VRGeometry::create(s);
    geo->setPrimitive(s);

    getSelected()->addChild(geo);
    parseSGTree(geo, selected_itr);
}

void VRGuiScene::on_menu_delete() {
    if(!selected_itr) return;
    //if (getSelected()->getPersistency() == 0) return; // if this behavior is intended, explain why..
    // todo: check for camera!!

    string msg1 = "Delete object " + getSelected()->getName();
    if (!askUser(msg1, "Are you sure you want to delete this object?")) return;
    getSelected()->destroy();
    selected = -1;

    Glib::RefPtr<Gtk::TreeModel> model = tree_view->get_model();
    removeTreeStoreBranch(selected_itr);
}

void VRGuiScene::on_menu_copy() {
    if(!selected_itr) return;
    VRGuiScene_copied = getSelected();
}

void VRGuiScene::on_menu_paste() {
    if(!selected_itr) return;
    auto obj = VRGuiScene_copied.lock();
    if (obj == 0) return;

    VRObjectPtr tmp = obj->duplicate();
    tmp->switchParent(getSelected());
    VRGuiScene_copied.reset();

    parseSGTree(tmp, selected_itr);
}

/*void VRGuiScene::on_menu_add_csg() {
    if(!selected_itr) return;
    CSGGeometryPtr g = new CSGGeometry("csg_geo");
    getSelected()->addChild(g);
    parseSGTree(g, selected_itr);
}*/

void VRGuiScene::on_collada_import_clicked() {
    string rel_path = VRGuiFile::getRelativePath_toWorkdir();
    string path = VRGuiFile::getPath(); // absolute path
    cout << "Data import: " << path << ", relative: " << rel_path << endl;
    VRGuiFile::close();

    // import stuff
    VRObjectPtr tmp = VRImport::get()->load(rel_path, getSelected());
    parseSGTree(tmp, selected_itr);
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
    auto dest = dragDest.lock();
    if (dest == 0) return;
    if (ac == 0) return;
    VRObjectPtr obj = getSelected();
    if (obj == 0) return;
    obj->switchParent(dest, dragPos);
    //cout << "drag_end " << obj->getPath() << endl;
    Gtk::TreeModel::iterator iter = tree_view->get_model()->get_iter(obj->getPath());
    setSGRow(iter, obj);
}

void VRGuiScene::on_drag_beg(const Glib::RefPtr<Gdk::DragContext>& dc) {
    //cout << "\nDRAG BEGIN " << dc->get_selection() << endl;
}

void VRGuiScene::on_drag_data_receive(const Glib::RefPtr<Gdk::DragContext>& dc , int x, int y ,const Gtk::SelectionData& sd, guint i3, guint i4) {
    Gtk::TreeModel::Path path;
    Gtk::TreeViewDropPosition pos; // enum
    tree_view->get_drag_dest_row(path, pos);
    if (path == 0) return;

    dragDest.reset();
    VRObjectPtr obj = getSelected();
    if (obj == 0) return;

    dragPath = path.to_string();
    dragPos = 0;
    if (pos <= 1) { // between two rows
        int d = dragPath.rfind(':');
        dragPos = toInt( dragPath.substr(d+1) );
        dragPath = dragPath.substr(0,d);
    }
    //cout << "drag dest " << dragPath << " " << pos << endl;

    if (obj->hasTag("treeviewNotDragable")) { dc->drag_status(Gdk::DragAction(0),0); return; } // object is not dragable
    if (dragPath == "0" && pos <= 1) { dc->drag_status(Gdk::DragAction(0),0); return; } // drag out of root

    /*Gtk::TreeModel::iterator iter = tree_view->get_model()->get_iter(path);
    if(!iter) { dc->drag_status(Gdk::DragAction(0),0); return; }
    ModelColumns cols;
    Gtk::TreeModel::Row row = *iter;
    string dest_path = row.get_value(cols.obj);*/

    auto scene = VRScene::getCurrent();
    if (scene) dragDest = scene->getRoot()->getAtPath(dragPath);
}


// ------------- transform -----------------------

void VRGuiScene::on_toggle_T_mode() {
    if (!trigger_cbs) return;
    transformModeLocal = getRadioButtonState("radiobutton19");

    VRObjectPtr obj = getSelected();
    if (obj) {
        if (obj->hasTag("transform")) setTransform(static_pointer_cast<VRTransform>(obj));
    }
}

void VRGuiScene::on_toggle_T_constraint_mode() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    bool plane = getRadioButtonState("radiobutton2");
    //obj->getConstraint()->setTConstraint( obj->getConstraint()->getTConstraint(), plane? OSG::VRConstraint::PLANE : OSG::VRConstraint::LINE);
}

void VRGuiScene::on_toggle_phys() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    bool phys = getCheckButtonState("checkbutton13");
    if (obj->getPhysics()) obj->getPhysics()->setPhysicalized(phys);
    setComboboxSensitivity("combobox8", phys);
}

void VRGuiScene::on_toggle_dynamic() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    bool dyn = getCheckButtonState("checkbutton33");
    if (obj->getPhysics()) obj->getPhysics()->setDynamic(dyn);
}

void VRGuiScene::on_mass_changed() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    string m = getTextEntry("entry59");
    if (obj->getPhysics()) obj->getPhysics()->setMass(toFloat(m));
}

void VRGuiScene::on_change_phys_shape() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    string t = getComboboxText("combobox8");
    if (obj->getPhysics()) obj->getPhysics()->setShape(t);
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
    VRCameraPtr obj = static_pointer_cast<VRCamera>( getSelected() );
    string a = getTextEntry("entry60");
    obj->setAspect(toFloat(a));
}

void VRGuiScene::on_cam_fov_changed() {
    if(!trigger_cbs) return;
    VRCameraPtr obj = static_pointer_cast<VRCamera>( getSelected() );
    string f = getTextEntry("entry61");
    obj->setFov(toFloat(f));
}

void VRGuiScene::on_cam_near_changed() {
    if(!trigger_cbs) return;
    VRCameraPtr obj = static_pointer_cast<VRCamera>( getSelected() );
    string f = getTextEntry("entry6");
    obj->setNear(toFloat(f));
}

void VRGuiScene::on_cam_far_changed() {
    if(!trigger_cbs) return;
    VRCameraPtr obj = static_pointer_cast<VRCamera>( getSelected() );
    string f = getTextEntry("entry7");
    obj->setFar(toFloat(f));
}

void VRGuiScene::on_change_cam_proj() {
    if(!trigger_cbs) return;
    VRCameraPtr obj = static_pointer_cast<VRCamera>( getSelected() );
    obj->setType(getComboboxI("combobox23"));
}
// ----------------------------------------------

// ------------- light -----------------------
void VRGuiScene::on_toggle_light() {
    if(!trigger_cbs) return;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    bool b = getCheckButtonState("checkbutton31");
    obj->setOn(b);
}

void VRGuiScene::on_toggle_light_shadow() {
    if(!trigger_cbs) return;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    bool b = getCheckButtonState("checkbutton32");
    obj->setShadows(b);
}

void VRGuiScene::on_toggle_light_shadow_volume() {
    if(!trigger_cbs) return;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    bool b = getCheckButtonState("checkbutton2");
    float D = toFloat( getTextEntry("entry36") );
    Boundingbox bb;
    if (!b) obj->setShadowVolume( bb );
    else {
        bb.inflate( 0.5*D );
        cout << "VRGuiScene::on_toggle_light_shadow_volume " << 0.5*D << " " << bb.volume() << " " << bb.size() << endl;
        obj->setShadowVolume( bb );
    }
}

void VRGuiScene::on_change_light_type() {
    if(!trigger_cbs) return;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    string t = getComboboxText("combobox2");
    obj->setType(t);
}

void VRGuiScene::on_change_light_shadow() {
    if(!trigger_cbs) return;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    string t = getComboboxText("combobox22");
    obj->setShadowMapRes(toInt(t));
}

void VRGuiScene::on_edit_light_attenuation() {
    if(!trigger_cbs) return;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    string ac = getTextEntry("entry44");
    string al = getTextEntry("entry45");
    string aq = getTextEntry("entry46");
    obj->setAttenuation(Vec3d(toFloat(ac), toFloat(al), toFloat(aq)));
}

bool VRGuiScene::setShadow_color(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return true;
    Color4f c = chooseColor("shadow_col", obj->getShadowColor());
    obj->setShadowColor(c);
    return true;
}

bool VRGuiScene::setLight_diff_color(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return true;
    Color4f c = chooseColor("light_diff", obj->getDiffuse());
    obj->setDiffuse(c);
    return true;
}

bool VRGuiScene::setLight_amb_color(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return true;
    Color4f c = chooseColor("light_amb", obj->getAmbient());
    obj->setAmbient(c);
    return true;
}

bool VRGuiScene::setLight_spec_color(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    VRLightPtr obj = static_pointer_cast<VRLight>( getSelected() );
    if (!obj) return true;
    Color4f c = chooseColor("light_spec", obj->getSpecular());
    obj->setSpecular(c);
    return true;
}
// ----------------------------------------------


// ------------- material -----------------------
void VRGuiScene::setMaterial_gui() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setMaterial_lit() {
    if(!trigger_cbs) return;
    auto geo = selected_geometry.lock();
    if(!geo) return;
    bool b = getCheckButtonState("checkbutton3");
    geo->getMaterial()->setLit(b);
}

bool VRGuiScene::setMaterial_diffuse(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    auto geo = selected_geometry.lock();
    if(!geo) return true;
    Color4f c = toColor4f(geo->getMaterial()->getDiffuse());
    c[3] = geo->getMaterial()->getTransparency();
    c = chooseColor("mat_diffuse", c);
    geo->getMaterial()->setDiffuse(toColor3f(c));
    geo->getMaterial()->setTransparency(c[3]);
    return true;
}

bool VRGuiScene::setMaterial_specular(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    auto geo = selected_geometry.lock();
    if(!geo) return true;
    Color4f c = chooseColor("mat_specular", toColor4f(geo->getMaterial()->getSpecular()));
    geo->getMaterial()->setSpecular(toColor3f(c));
    return true;
}

bool VRGuiScene::setMaterial_ambient(GdkEventButton* b) {
    if(!trigger_cbs) return true;
    auto geo = selected_geometry.lock();
    if(!geo) return true;
    Color4f c = chooseColor("mat_ambient", toColor4f(geo->getMaterial()->getAmbient()));
    geo->getMaterial()->setAmbient(toColor3f(c));
    return true;
}

void VRGuiScene::setMaterial_pointsize() { // TODO
    if(!trigger_cbs) return;
    auto geo = selected_geometry.lock();
    if(!geo) return;
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
    dragDest.reset();

    initCallbacks();

    //test_tree_dnd();

    // treeviewer
    tree_store = Glib::RefPtr<Gtk::TreeStore>::cast_static(VRGuiBuilder()->get_object("scenegraph"));
    tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview6"));
    //tree_view->signal_cursor_changed().connect( sigc::mem_fun(*this, &VRGuiScene::on_treeview_select) );
    setTreeviewSelectCallback("treeview6", sigc::mem_fun(*this, &VRGuiScene::on_treeview_select));

    initMenu();

    tree_view->signal_drag_begin().connect( sigc::mem_fun(*this, &VRGuiScene::on_drag_beg) );
    tree_view->signal_drag_end().connect( sigc::mem_fun(*this, &VRGuiScene::on_drag_end) );
    tree_view->signal_drag_data_received().connect( sigc::mem_fun(*this, &VRGuiScene::on_drag_data_receive) );

    setCellRendererCallback("cellrenderertext7", sigc::mem_fun(*this, &VRGuiScene::on_edit_object_name) );
    setCellRendererCallback("cellrenderertext4", sigc::mem_fun(*this, &VRGuiScene::on_edit_distance) );
    setCellRendererCallback("cellrenderertext33", sigc::mem_fun(*this, &VRGuiScene::on_edit_primitive_params) );

    setEntryCallback("entry41", sigc::mem_fun(*this, &VRGuiScene::on_group_edited));

    //light
    fillStringListstore("light_types", VRLight::getTypes());
    fillStringListstore("shadow_types", VRLight::getShadowMapResolutions());
    //fillStringListstore("csg_operations", CSGGeometry::getOperations());
    fillStringListstore("phys_shapes", VRPhysics::getPhysicsShapes());
    fillStringListstore("cam_proj", VRCamera::getProjectionTypes());

    // object form
    setCheckButtonCallback("checkbutton16", sigc::mem_fun(*this, &VRGuiScene::on_toggle_liveupdate));
    setCheckButtonCallback("checkbutton6", sigc::mem_fun(*this, &VRGuiScene::on_toggle_visible));
    setCheckButtonCallback("checkbutton15", sigc::mem_fun(*this, &VRGuiScene::on_toggle_pickable));
    setCheckButtonCallback("checkbutton18", sigc::mem_fun(*this, &VRGuiScene::on_toggle_rc));
    setCheckButtonCallback("checkbutton19", sigc::mem_fun(*this, &VRGuiScene::on_toggle_rc));
    setCheckButtonCallback("checkbutton20", sigc::mem_fun(*this, &VRGuiScene::on_toggle_rc));
    setCheckButtonCallback("checkbutton21", sigc::mem_fun(*this, &VRGuiScene::on_toggle_T_constraint));
    setCheckButtonCallback("checkbutton22", sigc::mem_fun(*this, &VRGuiScene::on_toggle_R_constraint));
    //setCheckButtonCallback("checkbutton27", on_toggle_CSG_editmode);
    setCheckButtonCallback("checkbutton17", sigc::mem_fun(*this, &VRGuiScene::on_toggle_camera_accept_realroot));
    setCheckButtonCallback("radiobutton1", sigc::mem_fun(*this, &VRGuiScene::on_toggle_T_constraint_mode) );
    setCheckButtonCallback("radiobutton19", sigc::mem_fun(*this, &VRGuiScene::on_toggle_T_mode) );
    setCheckButtonCallback("checkbutton31", sigc::mem_fun(*this, &VRGuiScene::on_toggle_light) );
    setCheckButtonCallback("checkbutton32", sigc::mem_fun(*this, &VRGuiScene::on_toggle_light_shadow) );
    setCheckButtonCallback("checkbutton2", sigc::mem_fun(*this, &VRGuiScene::on_toggle_light_shadow_volume) );
    setCheckButtonCallback("checkbutton13", sigc::mem_fun(*this, &VRGuiScene::on_toggle_phys) );
    setCheckButtonCallback("checkbutton33", sigc::mem_fun(*this, &VRGuiScene::on_toggle_dynamic) );
    setCheckButtonCallback("checkbutton35", sigc::mem_fun(*this, &VRGuiScene::on_lod_decimate_changed) );
    setCheckButtonCallback("checkbutton43", sigc::mem_fun(*this, &VRGuiScene::on_toggle_throw_shadow) );

    setComboboxCallback("combobox14", sigc::mem_fun(*this, &VRGuiScene::on_change_group));
    setComboboxCallback("combobox23", sigc::mem_fun(*this, &VRGuiScene::on_change_cam_proj));
    //setComboboxCallback("combobox19", on_change_CSG_operation);
    setComboboxCallback("combobox21", sigc::mem_fun(*this, &VRGuiScene::on_change_primitive));
    setComboboxCallback("combobox2", sigc::mem_fun(*this, &VRGuiScene::on_change_light_type) );
    setComboboxCallback("combobox22", sigc::mem_fun(*this, &VRGuiScene::on_change_light_shadow) );
    setComboboxCallback("combobox8", sigc::mem_fun(*this, &VRGuiScene::on_change_phys_shape) );

    setEntryCallback("entry44", sigc::mem_fun(*this, &VRGuiScene::on_edit_light_attenuation) );
    setEntryCallback("entry45", sigc::mem_fun(*this, &VRGuiScene::on_edit_light_attenuation) );
    setEntryCallback("entry46", sigc::mem_fun(*this, &VRGuiScene::on_edit_light_attenuation) );
    setEntryCallback("entry36", sigc::mem_fun(*this, &VRGuiScene::on_toggle_light_shadow_volume) );

    posEntry.init("pos_entry", "from", sigc::mem_fun(*this, &VRGuiScene::on_change_from));
    atEntry.init("at_entry", "at", sigc::mem_fun(*this, &VRGuiScene::on_change_at));
    dirEntry.init("dir_entry", "dir", sigc::mem_fun(*this, &VRGuiScene::on_change_dir));
    upEntry.init("up_entry", "up", sigc::mem_fun(*this, &VRGuiScene::on_change_up));
    scaleEntry.init("scale_entry", "scale", sigc::mem_fun(*this, &VRGuiScene::on_scale_changed));
    lodCEntry.init("lod_center", "center", sigc::mem_fun(*this, &VRGuiScene::on_change_lod_center));
    ctEntry.init("ct_entry", "", sigc::mem_fun(*this, &VRGuiScene::on_edit_T_constraint));

    setEntryCallback("entry59", sigc::mem_fun(*this, &VRGuiScene::on_mass_changed) );
    setEntryCallback("entry60", sigc::mem_fun(*this, &VRGuiScene::on_cam_aspect_changed) );
    setEntryCallback("entry61", sigc::mem_fun(*this, &VRGuiScene::on_cam_fov_changed) );
    setEntryCallback("entry6", sigc::mem_fun(*this, &VRGuiScene::on_cam_near_changed) );
    setEntryCallback("entry7", sigc::mem_fun(*this, &VRGuiScene::on_cam_far_changed) );
    setEntryCallback("entry9", sigc::mem_fun(*this, &VRGuiScene::on_lod_decimate_changed) );

    setButtonCallback("button4", sigc::mem_fun(*this, &VRGuiScene::on_focus_clicked));
    setButtonCallback("button11", sigc::mem_fun(*this, &VRGuiScene::on_identity_clicked));
    setButtonCallback("button19", sigc::mem_fun(*this, &VRGuiScene::on_groupsync_clicked));
    setButtonCallback("button20", sigc::mem_fun(*this, &VRGuiScene::on_groupapply_clicked));
    setButtonCallback("button17", sigc::mem_fun(*this, &VRGuiScene::on_scene_update));

    setColorChooser("mat_diffuse", sigc::mem_fun(*this, &VRGuiScene::setMaterial_diffuse));
    setColorChooser("mat_specular", sigc::mem_fun(*this, &VRGuiScene::setMaterial_specular));
    setColorChooser("mat_ambient", sigc::mem_fun(*this, &VRGuiScene::setMaterial_ambient));
    setColorChooser("shadow_col", sigc::mem_fun(*this, &VRGuiScene::setShadow_color));
    setColorChooser("light_diff", sigc::mem_fun(*this, &VRGuiScene::setLight_diff_color));
    setColorChooser("light_amb", sigc::mem_fun(*this, &VRGuiScene::setLight_amb_color));
    setColorChooser("light_spec", sigc::mem_fun(*this, &VRGuiScene::setLight_spec_color));

    updateObjectForms();
}

// new scene, update stuff here
void VRGuiScene::updateTreeView() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    tree_store->clear();
    VRObjectPtr root = scene->getRoot();
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
