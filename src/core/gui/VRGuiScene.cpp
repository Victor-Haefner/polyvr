#include <gtk/gtk.h>
#include "VRGuiScene.h"

#include <iostream>
#include <stdio.h>
//#include <unistd.h>

#include "core/scene/VRSceneLoader.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRCamera.h"
#include "core/objects/VRGroup.h"
#include "core/objects/VRLod.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#endif
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
#include "VRGuiBuilder.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "addons/construction/building/VRElectricDevice.h"
//#include "addons/Engineering/CSG/CSGGeometry.h"

#include "wrapper/VRGuiTreeView.h"

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
        setWidgetSensitivity("table11", false);
    }*/

    return res;
}

void VRGuiScene::setObject(VRObjectPtr o) {
    setWidgetVisibility("expander2", true, true);

    // set object properties
    setToggleButton("checkbutton6", o->isVisible());
    setToggleButton("checkbutton43", o->isVisible("SHADOW"));
    setToggleButton("checkbutton15", o->isPickable());

    VRObjectPtr parent = o->getParent();
    string pName = parent ? parent->getName() : "";
    setTextEntry("entry17", pName);
    setTextEntry("entry21", toString(o->getPersistency()) );
}

void VRGuiScene::setTransform(VRTransformPtr e) {
    setWidgetVisibility("expander1", true, true);

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

    setToggleButton("checkbutton18", rc[0]);
    setToggleButton("checkbutton19", rc[1]);
    setToggleButton("checkbutton20", rc[2]);

    setToggleButton("checkbutton21", doTc);
    setToggleButton("checkbutton22", doRc);

    setToggleButton("radiobutton1", !c->getTMode());
    setToggleButton("radiobutton2", c->getTMode());*/

    setToggleButton("radiobutton19",  transformModeLocal);
    setToggleButton("radiobutton20", !transformModeLocal);

#ifndef WITHOUT_BULLET
    if (e->getPhysics()) {
        setToggleButton("checkbutton13", e->getPhysics()->isPhysicalized());
        setToggleButton("checkbutton33", e->getPhysics()->isDynamic());
        setTextEntry("entry59", toString(e->getPhysics()->getMass()));
        setCombobox("combobox8", getListStorePos("phys_shapes", e->getPhysics()->getShape()));
        setWidgetSensitivity("combobox8", e->getPhysics()->isPhysicalized());
    }
#endif
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

    setToggleButton("checkbutton3", lit);
    setToggleButton("checkbutton5", bool(tex));
    setWidgetSensitivity("table44", bool(tex));

    if (tex) {
        setLabel("label158", toString(tex->getSize()) + " (" + toString(tex->getByteSize()/1048576.0) + " mb)");
        setLabel("label157", toString(tex->getChannels()));
    }
}

void VRGuiScene::on_geo_menu_print() {
    if(!selected_itr) return;

    VRGuiTreeView tree_view("treeview9");

    string name = tree_view.getSelectedStringValue(0);
    int index = toInt(tree_view.getSelectedStringValue(2));

    VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(getSelected());
    VRGeoData data(geo);
    string msg = data.getDataAsString(index);
    VRGuiManager::get()->getConsole( "Console" )->write( geo->getName() + " " + name + ": " + msg + "\n" );
}

void VRGuiScene::setGeometry(VRGeometryPtr g) {
    setWidgetVisibility("expander11", true, true);
    setWidgetVisibility("expander14", true, true);
    setWidgetVisibility("expander16", true, true);
    VRMaterialPtr mat = g->getMaterial();
    setMaterial(mat);

    setToggleButton("checkbutton28", false);
    setCombobox("combobox21", -1);

    auto store = (GtkListStore*)VRGuiBuilder::get()->get_object("primitive_opts");
    gtk_list_store_clear(store);

    stringstream params;
    params << g->getReference().parameter;

    switch (g->getReference().type) {
        case VRGeometry::FILE:
            break;
        case VRGeometry::CODE:
            break;
        case VRGeometry::PRIMITIVE:
            setToggleButton("checkbutton28", true);
            string ptype; params >> ptype;
            setCombobox("combobox21", getListStorePos("prim_list", ptype));
            vector<string> param_names = VRPrimitive::getTypeParameter(ptype);
            for (uint i=0; i<param_names.size(); i++) {
                string val; params >> val;
                GtkTreeIter row;
                gtk_list_store_append(store, &row);
                gtk_list_store_set(store, &row, 0, param_names[i].c_str(), -1);
                gtk_list_store_set(store, &row, 1, val.c_str(), -1);
            }
            break;
    }

    auto appendGeoData = [](GtkListStore* store, string name, int N, int I) {
        GtkTreeIter row;
        gtk_list_store_append(store, &row);
        gtk_list_store_set(store, &row, 0, name.c_str(), -1);
        gtk_list_store_set(store, &row, 1, N, -1);
        gtk_list_store_set(store, &row, 2, I, -1);
    };

    VRGeoData data(g);
    store = (GtkListStore*)VRGuiBuilder::get()->get_object("geodata");
    gtk_list_store_clear(store);
    for (int i=0; i<=8; i++) {
        int N = data.getDataSize(i);
        if (N) appendGeoData(store, data.getDataName(i), N, i);
    }
}

void VRGuiScene::setLight(VRLightPtr l) {
    setWidgetVisibility("expander13", true, true);

    setToggleButton("checkbutton31", l->isOn());
    setToggleButton("checkbutton32", l->getShadows());
    setToggleButton("checkbutton2", l->getShadowVolume().volume() > 1e-4);
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

    auto store = (GtkListStore*)VRGuiBuilder::get()->get_object("light_params");
    vector<string> param_names = VRLight::getTypeParameter(l->getLightType());
    for (uint i=0; i<param_names.size(); i++) {
        string val; // TODO
        GtkTreeIter row;
        gtk_list_store_append(store, &row);
        gtk_list_store_set(store, &row, 0, param_names[i].c_str(), -1);
        gtk_list_store_set(store, &row, 1, val.c_str(), -1);
    }
}

void VRGuiScene::setCamera(VRCameraPtr c) {
    setWidgetVisibility("expander12", true, true);
    setToggleButton("checkbutton17", c->getAcceptRoot());
    setTextEntry("entry60", toString(c->getAspect()));
    setTextEntry("entry61", toString(c->getFov()));
    setTextEntry("entry6", toString(c->getNear()));
    setTextEntry("entry7", toString(c->getFar()));
}

void VRGuiScene::setGroup(VRGroupPtr g) {
    setWidgetVisibility("expander2", true, true);
    setWidgetVisibility("expander9", true, true);
    setToggleButton("checkbutton23", g->getActive() );

    fillStringListstore("liststore3", g->getGroups());

    setCombobox("combobox14", getListStorePos("liststore3",g->getGroup()));
}

void VRGuiScene::setLod(VRLodPtr lod) {
    setWidgetVisibility("expander2", true, true);
    setWidgetVisibility("expander10", true, true);

    setTextEntry("entry9", toString(lod->getDecimateNumber()));
    setToggleButton("checkbutton35", lod->getDecimate());
    lodCEntry.set(lod->getCenter());

    auto store = (GtkListStore*)VRGuiBuilder::get()->get_object("liststore5");
    gtk_list_store_clear(store);

    vector<float> dists = lod->getDistances();
    for(uint i=0; i< dists.size(); i++) {
        GtkTreeIter row;
        gtk_list_store_append(store, &row);
        gtk_list_store_set(store, &row, 0, i, -1);
        gtk_list_store_set(store, &row, 1, dists[i], -1);
        gtk_list_store_set(store, &row, 2, true, -1);
    }
}

void VRGuiScene::setEntity(VREntityPtr e) {
    setWidgetVisibility("expander27", true, true);

    setLabel("label145", e->getConceptList());

    auto store = (GtkListStore*)VRGuiBuilder::get()->get_object("properties");
    gtk_list_store_clear(store);

    for(auto pvec : e->properties) {
        for (auto p : pvec.second) {
            GtkTreeIter row;
            gtk_list_store_append(store, &row);
            gtk_list_store_set(store, &row, 0, pvec.first.c_str(), -1);
            gtk_list_store_set(store, &row, 1, p.second->value.c_str(), -1);
            gtk_list_store_set(store, &row, 2, p.second->type.c_str(), -1);
        }
    }
}

/*void setCSG(CSGGeometryPtr g) {
    setWidgetVisibility("expander15", true, true);

    bool b = g->getEditMode();
    string op = g->getOperation();
    setToggleButton("checkbutton27", b);
    setCombobox("combobox19", getListStorePos("csg_operations",op));
}*/

void VRGuiScene::on_toggle_liveupdate() { liveUpdate = !liveUpdate; }

void VRGuiScene::updateObjectForms(bool disable) {
    setWidgetVisibility("expander1", false, true);
    setWidgetVisibility("expander2", false, true);
    setWidgetVisibility("expander9", false, true);
    setWidgetVisibility("expander10", false, true);
    setWidgetVisibility("expander11", false, true);
    setWidgetVisibility("expander12", false, true);
    setWidgetVisibility("expander13", false, true);
    setWidgetVisibility("expander14", false, true);
    setWidgetVisibility("expander15", false, true);
    setWidgetVisibility("expander16", false, true);
    setWidgetVisibility("expander27", false, true);
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

void VRGuiScene::setSGRow(GtkTreeIter* itr, VRObjectPtr o) {
    if (o == 0) return;
    if (!itr) return;

    string fg, bg;
    getTypeColors(o, fg, bg);

    auto e = o->getEntity();

    string name = o->getName();
    if (e) name += "<sub><u><span foreground=\"blue\">" + e->getConceptList() + "</span></u></sub>";

    gtk_tree_store_set (tree_store, itr, 0, name.c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 1, o->getType().c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 2, o->getPath().c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 3, fg.c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 4, bg.c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 5, o->getID(), -1);
}

void VRGuiScene::parseSGTree(VRObjectPtr o, GtkTreeIter* itr) {
    if (o == 0) return;
    GtkTreeIter nItr;
    gtk_tree_store_append(tree_store, &nItr, itr);
    setSGRow( &nItr, o );
    for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), &nItr );
}

void VRGuiScene::removeTreeStoreBranch(GtkTreeIter* iter, bool self) {
    int N = gtk_tree_store_iter_depth(tree_store, iter);

    if (!self) {
        int Nc = gtk_tree_model_iter_n_children((GtkTreeModel*)tree_store, iter);
        if (Nc == 0) return;
        gtk_tree_model_iter_children((GtkTreeModel*)tree_store, iter, iter);
    } else gtk_tree_store_remove(tree_store, iter); // removes iter and sets it to next row

    while (gtk_tree_store_iter_depth(tree_store, iter) > N) gtk_tree_store_remove(tree_store, iter); // removes iter and sets it to next row
}

void VRGuiScene::syncSGTree(VRObjectPtr o, GtkTreeIter* itr) {
    if (o == 0) return;
    removeTreeStoreBranch(itr, false);
    for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), itr );
}

void VRGuiScene::on_treeview_select() {
    GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

    if (!selected_itr) selected_itr = new GtkTreeIter();
    if (!gtk_tree_selection_get_selected(sel, &model, selected_itr)) return;

    setWidgetSensitivity("table11", true);

    updateObjectForms(true);
    gtk_tree_model_get(model, selected_itr, 5, &selected, -1);
    updateObjectForms();

    selected_geometry.reset();
    if (getSelected() == 0) return;
    if (getSelected()->hasTag("geometry")) selected_geometry = static_pointer_cast<VRGeometry>(getSelected());
}

void VRGuiScene::on_edit_object_name(const char* path_string, const char* new_text) {
    if(!trigger_cbs) return;
    getSelected()->setName(new_text);
    VRGuiTreeView tree_view("treeview6");
    tree_view.setSelectedStringValue(0, getSelected()->getName());
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
        GtkTreeIter itr;
        gtk_tree_model_get_iter_from_string((GtkTreeModel*)tree_store, &itr, path.c_str());
        syncSGTree(g, &itr);
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
    if (!obj) return;
    string old_group = getComboboxText("combobox14");
    string new_group = getTextEntry("entry41");

    // update group list
    GtkTreeIter itr;
    auto store = (GtkListStore*)VRGuiBuilder::get()->get_object("liststore3");
    gtk_list_store_append(store, &itr);
    gtk_list_store_set(store, &itr, 0, new_group.c_str(), -1);

    // add group
    obj->setGroup(new_group);
    setComboboxLastActive("combobox14");

    setTextEntry("entry41", "");
}

// VR3DEntities
void VRGuiScene::on_change_from(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setFrom(v);
    else obj->setWorldPosition(v);
    //updateObjectForms(); // TODO: fix bug!
}

void VRGuiScene::on_change_at(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setAt(v);
    else obj->setWorldAt(v);
    updateObjectForms();
}

void VRGuiScene::on_change_dir(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setDir(v);
    else obj->setWorldDir(v);
    updateObjectForms();
}

void VRGuiScene::on_change_up(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setUp(v);
    else obj->setWorldUp(v);
    updateObjectForms();
}

void VRGuiScene::on_scale_changed(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setScale(v);
    //else obj->setWorldScale(v); TODO
    else obj->setScale(v);
    updateObjectForms();
}

void VRGuiScene::on_change_lod_center(Vec3d v) {
    if(!trigger_cbs) return;
    VRLodPtr obj = static_pointer_cast<VRLod>( getSelected() );
    if (!obj) return;
    obj->setCenter(v);
    updateObjectForms();
}

void VRGuiScene::on_focus_clicked() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->getActiveCamera()->focusObject( obj );
}

void VRGuiScene::on_identity_clicked() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
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

void VRGuiScene::on_edit_primitive_params(const char* path_string, const char* new_text) {
    if (!trigger_cbs) return;

    string prim = getComboboxText("combobox21");
    string args;

    VRGuiTreeView tree_view("treeview12");
    tree_view.setSelectedStringValue(1, new_text);

    auto store = VRGuiBuilder::get()->get_object("primitive_opts");
    int N = gtk_tree_model_iter_n_children((GtkTreeModel*)store, NULL );
    for (int i=0; i<N; i++) {
        GtkTreeIter iter;
        gtk_tree_model_get_iter_from_string((GtkTreeModel*)store, &iter, toString(i).c_str());

        gchar* item = NULL;
        gtk_tree_model_get((GtkTreeModel*)store, &iter, 1, &item, -1);
        string c = item;
        g_free(item);

        args += c;
        if (i<N-1) args += " ";
    }

    VRGeometryPtr obj = static_pointer_cast<VRGeometry>( getSelected() );
    obj->setPrimitive(prim + " " + args);
}

void VRGuiScene::on_edit_distance(const char* path_string, const char* new_text) {
    if(!trigger_cbs) return;
    VRLodPtr lod = static_pointer_cast<VRLod>( getSelected() );

    VRGuiTreeView tree_view("treeview8");
    tree_view.setSelectedStringValue(1, new_text);

    float f = toFloat(new_text);
    int i = toInt(path_string);
    lod->setDistance(i,f);
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
    VRGuiFile::setCallbacks( bind(&VRGuiScene::on_collada_import_clicked, this) );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::addFilter("COLLADA", 1, "*.dae");
    VRGuiFile::addFilter("VRML", 1, "*.wrl");
    VRGuiFile::addFilter("3DS", 1, "*.3ds");
    VRGuiFile::addFilter("OBJ", 1, "*.obj");
    VRGuiFile::setGeoLoadWidget();
    VRGuiFile::open( "Load", GTK_FILE_CHOOSER_ACTION_OPEN, "Load geometric data" );
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

void VRGuiScene::on_drag_end(GdkDragContext* dc) {
    auto act = gdk_drag_context_get_selected_action(dc);
    auto dest = dragDest.lock();
    if (dest == 0) return;
    if (act == 0) return;
    VRObjectPtr obj = dragObj.lock();
    if (obj == 0) return;
    obj->switchParent(dest, dragPos);

    GtkTreeIter iter;
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_get_iter_from_string(model, &iter, obj->getPath().c_str());
    setSGRow(&iter, obj);
}

void VRGuiScene::on_drag_beg(GdkDragContext* dc) {
    //cout << "\nDRAG BEGIN " << dc->get_selection() << endl;
}

void VRGuiScene::on_drag_data_receive(GdkDragContext* dc, int x, int y, GtkSelectionData* sd, guint info, guint time) {
    GtkTreePath* path = 0;
    GtkTreeViewDropPosition pos; // enum
    gtk_tree_view_get_drag_dest_row(tree_view, &path, &pos);
    if (path == 0) return;

    dragDest.reset();
    dragObj.reset();
    VRObjectPtr obj = getSelected();
    if (obj == 0) return;
    dragObj = obj;

    dragPath = gtk_tree_path_to_string(path);
    dragPos = 0;
    if (pos <= 1) { // between two rows
        int d = dragPath.rfind(':');
        dragPos = toInt( dragPath.substr(d+1) );
        dragPath = dragPath.substr(0,d);
    }
    //cout << "drag dest " << dragPath << " " << pos << endl;

    if (obj->hasTag("treeviewNotDragable")) { gdk_drag_status(dc, GdkDragAction(0),0); return; } // object is not dragable
    if (dragPath == "0" && pos <= 1) { gdk_drag_status(dc, GdkDragAction(0),0); return; } // drag out of root

    /*GtkTreeIter* iter = tree_view->get_model()->get_iter(path);
    if(!iter) { dc->drag_status(Gdk::DragAction(0),0); return; }
    ModelColumns cols; // name, type, obj, fg, bg
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

    //bool plane = getRadioButtonState("radiobutton2");
    //obj->getConstraint()->setTConstraint( obj->getConstraint()->getTConstraint(), plane? OSG::VRConstraint::PLANE : OSG::VRConstraint::LINE);
}

void VRGuiScene::on_toggle_phys() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    bool phys = getCheckButtonState("checkbutton13");
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()) obj->getPhysics()->setPhysicalized(phys);
#endif
    setWidgetSensitivity("combobox8", phys);
}

void VRGuiScene::on_toggle_dynamic() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    bool dyn = getCheckButtonState("checkbutton33");
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()) obj->getPhysics()->setDynamic(dyn);
#endif
}

void VRGuiScene::on_mass_changed() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );

    string m = getTextEntry("entry59");
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()) obj->getPhysics()->setMass(toFloat(m));
#endif
}

void VRGuiScene::on_change_phys_shape() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = static_pointer_cast<VRTransform>( getSelected() );
    string t = getComboboxText("combobox8");
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()) obj->getPhysics()->setShape(t);
#endif
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
    menu->connectWidget("SGMenu", (GtkWidget*)tree_view);

    menu->appendMenu("SGMenu", "Add", "SGM_AddMenu");
    menu->appendMenu("SGM_AddMenu", "Primitive", "SGM_AddPrimMenu" );
    menu->appendMenu("SGM_AddMenu", "Urban", "SGM_AddUrbMenu" );
    menu->appendMenu("SGM_AddMenu", "Nature", "SGM_AddNatMenu" );
    menu->appendMenu("SGM_AddUrbMenu", "Building", "SGM_AddUrbBuiMenu" );

    menu->appendItem("SGMenu", "Copy", bind(&VRGuiScene::on_menu_copy, this));
    menu->appendItem("SGMenu", "Paste", bind(&VRGuiScene::on_menu_paste, this));
    menu->appendItem("SGMenu", "Delete", bind(&VRGuiScene::on_menu_delete, this));

    menu->appendItem("SGM_AddMenu", "Object", bind(&VRGuiScene::on_menu_add<VRObject>, this) );
    menu->appendItem("SGM_AddMenu", "Transform", bind(&VRGuiScene::on_menu_add<VRTransform>, this) );
    menu->appendItem("SGM_AddMenu", "Geometry", bind(&VRGuiScene::on_menu_add<VRGeometry>, this) );
    menu->appendItem("SGM_AddMenu", "Material", bind(&VRGuiScene::on_menu_add<VRMaterial>, this) );
    menu->appendItem("SGM_AddMenu", "Light", bind(&VRGuiScene::on_menu_add_light, this));
    menu->appendItem("SGM_AddMenu", "Camera", bind(&VRGuiScene::on_menu_add_camera, this));
    menu->appendItem("SGM_AddMenu", "Group", bind(&VRGuiScene::on_menu_add<VRGroup>, this));
    menu->appendItem("SGM_AddMenu", "LoD", bind(&VRGuiScene::on_menu_add<VRLod>, this));
    menu->appendItem("SGM_AddMenu", "Animation", bind(&VRGuiScene::on_menu_add_animation, this));
    menu->appendItem("SGM_AddMenu", "File", bind(&VRGuiScene::on_menu_add_file, this) );
    //menu->appendItem("SGM_AddMenu", "CSGGeometry", bind(&VRGuiScene::on_menu_add_csg) );

    // primitives
    vector<string> prims = VRPrimitive::getTypes();
    for (uint i=0; i<prims.size(); i++)
        menu->appendItem("SGM_AddPrimMenu", prims[i], bind(&VRGuiScene::on_menu_add_primitive, this, prims[i]) );

    // building elements
    //menu->appendItem("SGM_AddUrbBuiMenu", "Opening", bind(&VRGuiScene::on_menu_add<VROpening>) );
    //menu->appendItem("SGM_AddUrbBuiMenu", "Device", bind(&VRGuiScene::on_menu_add<VRElectricDevice>) );
}

void VRGuiScene::initCallbacks() {
    setToggleButtonCallback("checkbutton3", bind(&VRGuiScene::setMaterial_lit, this));
}

// TODO: remove groups
VRGuiScene::VRGuiScene() { // TODO: reduce callbacks with templated functions
    dragDest.reset();

    initCallbacks();

    //test_tree_dnd();

    // treeviewer
    tree_store = (GtkTreeStore*)VRGuiBuilder::get()->get_object("scenegraph");
    tree_view  = (GtkTreeView*)VRGuiBuilder::get()->get_widget("treeview6");
    //tree_view->signal_cursor_changed().connect( bind(&VRGuiScene::on_treeview_select) );
    setTreeviewSelectCallback("treeview6", bind(&VRGuiScene::on_treeview_select, this));

    initMenu();

    connect_signal<void, GdkDragContext*>((GtkWidget*)tree_view, bind(&VRGuiScene::on_drag_beg, this, placeholders::_1), "drag_begin" );
    connect_signal<void, GdkDragContext*>((GtkWidget*)tree_view, bind(&VRGuiScene::on_drag_end, this, placeholders::_1), "drag_end" );
    connect_signal<void, GdkDragContext*, int, int, GtkSelectionData*, guint, guint>((GtkWidget*)tree_view, bind(&VRGuiScene::on_drag_data_receive, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5, placeholders::_6), "drag_data_received" );
    
    setCellRendererCallback("cellrenderertext7", bind(&VRGuiScene::on_edit_object_name, this, placeholders::_1, placeholders::_2) );
    setCellRendererCallback("cellrenderertext4", bind(&VRGuiScene::on_edit_distance, this, placeholders::_1, placeholders::_2) );
    setCellRendererCallback("cellrenderertext33", bind(&VRGuiScene::on_edit_primitive_params, this, placeholders::_1, placeholders::_2) );

    setEntryCallback("entry41", bind(&VRGuiScene::on_group_edited, this));

    //light
    fillStringListstore("light_types", VRLight::getTypes());
    fillStringListstore("shadow_types", VRLight::getShadowMapResolutions());
    //fillStringListstore("csg_operations", CSGGeometry::getOperations());
#ifndef WITHOUT_BULLET
    fillStringListstore("phys_shapes", VRPhysics::getPhysicsShapes());
#endif
    fillStringListstore("cam_proj", VRCamera::getProjectionTypes());

    // object form
    setToggleButtonCallback("checkbutton16", bind(&VRGuiScene::on_toggle_liveupdate, this));
    setToggleButtonCallback("checkbutton6", bind(&VRGuiScene::on_toggle_visible, this));
    setToggleButtonCallback("checkbutton15", bind(&VRGuiScene::on_toggle_pickable, this));
    setToggleButtonCallback("checkbutton18", bind(&VRGuiScene::on_toggle_rc, this));
    setToggleButtonCallback("checkbutton19", bind(&VRGuiScene::on_toggle_rc, this));
    setToggleButtonCallback("checkbutton20", bind(&VRGuiScene::on_toggle_rc, this));
    setToggleButtonCallback("checkbutton21", bind(&VRGuiScene::on_toggle_T_constraint, this));
    setToggleButtonCallback("checkbutton22", bind(&VRGuiScene::on_toggle_R_constraint, this));
    //setToggleButtonCallback("checkbutton27", on_toggle_CSG_editmode);
    setToggleButtonCallback("checkbutton17", bind(&VRGuiScene::on_toggle_camera_accept_realroot, this));
    setToggleButtonCallback("radiobutton1", bind(&VRGuiScene::on_toggle_T_constraint_mode, this) );
    setToggleButtonCallback("radiobutton19", bind(&VRGuiScene::on_toggle_T_mode, this) );
    setToggleButtonCallback("checkbutton31", bind(&VRGuiScene::on_toggle_light, this) );
    setToggleButtonCallback("checkbutton32", bind(&VRGuiScene::on_toggle_light_shadow, this) );
    setToggleButtonCallback("checkbutton2", bind(&VRGuiScene::on_toggle_light_shadow_volume, this) );
    setToggleButtonCallback("checkbutton13", bind(&VRGuiScene::on_toggle_phys, this) );
    setToggleButtonCallback("checkbutton33", bind(&VRGuiScene::on_toggle_dynamic, this) );
    setToggleButtonCallback("checkbutton35", bind(&VRGuiScene::on_lod_decimate_changed, this) );
    setToggleButtonCallback("checkbutton43", bind(&VRGuiScene::on_toggle_throw_shadow, this) );

    setComboboxCallback("combobox14", bind(&VRGuiScene::on_change_group, this));
    setComboboxCallback("combobox23", bind(&VRGuiScene::on_change_cam_proj, this));
    //setComboboxCallback("combobox19", on_change_CSG_operation);
    setComboboxCallback("combobox21", bind(&VRGuiScene::on_change_primitive, this));
    setComboboxCallback("combobox2", bind(&VRGuiScene::on_change_light_type, this) );
    setComboboxCallback("combobox22", bind(&VRGuiScene::on_change_light_shadow, this) );
    setComboboxCallback("combobox8", bind(&VRGuiScene::on_change_phys_shape, this) );

    setEntryCallback("entry44", bind(&VRGuiScene::on_edit_light_attenuation, this) );
    setEntryCallback("entry45", bind(&VRGuiScene::on_edit_light_attenuation, this) );
    setEntryCallback("entry46", bind(&VRGuiScene::on_edit_light_attenuation, this) );
    setEntryCallback("entry36", bind(&VRGuiScene::on_toggle_light_shadow_volume, this) );

    posEntry.init("pos_entry", "from", bind(&VRGuiScene::on_change_from, this, placeholders::_1));
    atEntry.init("at_entry", "at", bind(&VRGuiScene::on_change_at, this, placeholders::_1));
    dirEntry.init("dir_entry", "dir", bind(&VRGuiScene::on_change_dir, this, placeholders::_1));
    upEntry.init("up_entry", "up", bind(&VRGuiScene::on_change_up, this, placeholders::_1));
    scaleEntry.init("scale_entry", "scale", bind(&VRGuiScene::on_scale_changed, this, placeholders::_1));
    lodCEntry.init("lod_center", "center", bind(&VRGuiScene::on_change_lod_center, this, placeholders::_1));
    ctEntry.init("ct_entry", "", bind(&VRGuiScene::on_edit_T_constraint, this, placeholders::_1));

    setEntryCallback("entry59", bind(&VRGuiScene::on_mass_changed, this) );
    setEntryCallback("entry60", bind(&VRGuiScene::on_cam_aspect_changed, this) );
    setEntryCallback("entry61", bind(&VRGuiScene::on_cam_fov_changed, this) );
    setEntryCallback("entry6", bind(&VRGuiScene::on_cam_near_changed, this) );
    setEntryCallback("entry7", bind(&VRGuiScene::on_cam_far_changed, this) );
    setEntryCallback("entry9", bind(&VRGuiScene::on_lod_decimate_changed, this) );

    setButtonCallback("button4", bind(&VRGuiScene::on_focus_clicked, this));
    setButtonCallback("button11", bind(&VRGuiScene::on_identity_clicked, this));
    setButtonCallback("button19", bind(&VRGuiScene::on_groupsync_clicked, this));
    setButtonCallback("button20", bind(&VRGuiScene::on_groupapply_clicked, this));
    setButtonCallback("button17", bind(&VRGuiScene::on_scene_update, this));

    setColorChooser("mat_diffuse", bind(&VRGuiScene::setMaterial_diffuse, this, placeholders::_1));
    setColorChooser("mat_specular", bind(&VRGuiScene::setMaterial_specular, this, placeholders::_1));
    setColorChooser("mat_ambient", bind(&VRGuiScene::setMaterial_ambient, this, placeholders::_1));
    setColorChooser("shadow_col", bind(&VRGuiScene::setShadow_color, this, placeholders::_1));
    setColorChooser("light_diff", bind(&VRGuiScene::setLight_diff_color, this, placeholders::_1));
    setColorChooser("light_amb", bind(&VRGuiScene::setLight_amb_color, this, placeholders::_1));
    setColorChooser("light_spec", bind(&VRGuiScene::setLight_spec_color, this, placeholders::_1));

    auto tree_view9 = VRGuiBuilder::get()->get_widget("treeview9");
    menu = new VRGuiContextMenu("GeoMenu");
    menu->connectWidget("GeoMenu", (GtkWidget*)tree_view9);
    menu->appendItem("GeoMenu", "Print", bind(&VRGuiScene::on_geo_menu_print, this));

    updateObjectForms();
}

// new scene, update stuff here
void VRGuiScene::updateTreeView() {
	cout << "VRGuiScene::updateTreeView" << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    gtk_tree_store_clear(tree_store);
    VRObjectPtr root = scene->getRoot();
    parseSGTree( root );
    gtk_tree_view_expand_all(tree_view);

    setWidgetSensitivity("table11", false);
}

// check if currently getSelected() object has been modified
void VRGuiScene::update() {
    if (!liveUpdate) return;
    updateObjectForms();
}

OSG_END_NAMESPACE;
