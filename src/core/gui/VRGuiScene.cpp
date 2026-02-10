#include <OpenSG/OSGRenderAction.h>

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
#include "VRGuiManager.h"

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
    if (!o) return;
    VRObjectPtr parent = o->getParent();
    string pName = parent ? parent->getName() : "";

    uiSignal("on_sg_set_obj_type", {{"objType", o->getType()}});

    uiSignal( "on_sg_setup_obj", {
        {"parent", pName},
        {"name", o->getName()},
        {"persistency", toString(o->getPersistency())},
        {"visible", toString(o->isVisible())},
        {"pickable", toString(o->isPickable())},
        {"castShadow", toString(o->isVisible("SHADOW"))},
        {"hasEntity", toString(bool(o->getEntity()))}
    } );
}

void VRGuiScene::setTransform(VRTransformPtr e) {
    if (!e) return;
    Vec3d f,a,u,d,s;
    cout << "VRGuiScene::setTransform " << transformModeLocal << endl;
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
        s = e->getWorldScale();
    }

    map<string, string> params = {
        {"pos", toString(f)},
        {"at", toString(a)},
        {"dir", toString(d)},
        {"up", toString(u)},
        {"scale", toString(s)},
        {"useAt", toString(e->get_orientation_mode())},
        {"local", toString(transformModeLocal)}
    };


    auto c = e->getConstraint();
    params["constrActive"] = toString(c->isActive());
    params["constrLocal"] = toString(c->isLocal());
    params["constrReferenceA"] = toString(c->getReferenceA());
    params["constrReferenceB"] = toString(c->getReferenceB());
    if (auto r = c->getReferential()) params["constrReferential"] = r->getName();

    for (int i=0; i<6; i++) {
        params["constrDof"+toString(i)] = toString(Vec2d(c->getMin(i), c->getMax(i)));
    }

#ifndef WITHOUT_BULLET
    auto phys = e->getPhysics();
    if (phys) {
        params["isPhysicalized"] = toString(phys->isPhysicalized());
        params["physDynamic"] = toString(phys->isDynamic());
        params["physMass"] = toString(phys->getMass());
        params["physShape"] = toString(phys->getShape());
    }
#endif

    uiSignal( "on_sg_setup_trans", params );
}

void VRGuiScene::setMaterial(VRMaterialPtr mat) {
    if (!mat) return;
    map<string, string> params;

    if (mat) {
        params["name"] = mat->getName();
        params["ambient"] = toString(mat->getAmbient());
        params["diffuse"] = toString(mat->getDiffuse());
        params["specular"] = toString(mat->getSpecular());
        params["emission"] = toString(mat->getEmission());
        params["isLit"] = toString(mat->isLit());
        params["ignoreMeshCols"] = toString(mat->doesIgnoreMeshColors());
        params["pointsize"] = toString(mat->getPointSize());
        params["linewidth"] = toString(mat->getLineWidth());

        VRTexturePtr tex = mat->getTexture();
        if (tex) {
            params["texDims"] = toString(tex->getSize());
            params["texSize"] = toString(tex->getByteSize()/1048576.0);
            params["texChannels"] = toString(tex->getChannels());
        }
    }

    uiSignal( "on_sg_setup_mat", params );
}

void VRGuiScene::on_geo_menu_print() {
    /*if(!selected_itr) return;

    VRGuiTreeView tree_view("treeview9");

    string name = tree_view.getSelectedStringValue(0);
    int index = toInt(tree_view.getSelectedStringValue(2));

    VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(getSelected());
    VRGeoData data(geo);
    string msg = data.getDataAsString(index);
    VRConsoleWidget::get( "Console" )->write( geo->getName() + " " + name + ": " + msg + "\n" );*/
}

void VRGuiScene::setGeometry(VRGeometryPtr g) {
    if (!g) return;
    VRMaterialPtr mat = g->getMaterial();
    setMaterial(mat);

    map<string, string> params;

    string origin;
    vector<string> primparams = splitString( g->getReference().parameter );

    switch (g->getReference().type) {
        case VRGeometry::FILE:
            origin = "file";
            break;
        case VRGeometry::CODE:
            origin = "code";
            break;
        case VRGeometry::PRIMITIVE:
            origin = "primitive";
            string ptype = primparams[0];
            vector<string> param_names = VRPrimitive::getTypeParameter(ptype);
            params["paramNames"] = toString(param_names);
            break;
    }

    params["origin"] = origin;
    params["originParams"] = g->getReference().parameter;
    params["meshVisible"] = toString(g->getMeshVisibility());

    string geoData;
    VRGeoData data(g);
    for (int i=0; i<=8; i++) {
        int N = data.getDataSize(i);
        string name = data.getDataName(i);
        if (geoData.size()) geoData += "$";
        geoData += name + "|" + toString(N);
    }

    params["geoData"] = geoData;

    uiSignal( "on_sg_setup_geo", params );
}

void VRGuiScene::setLight(VRLightPtr l) {
    if (!l) return;
    uiSignal( "on_sg_setup_light", {
        {"type", toString(l->getLightType())},
        {"shadowRes", toString(l->getShadowMapRes())},
        {"state", toString(l->isOn())},
        {"doShadows", toString(l->getShadows())},
    } );

    /*
    setToggleButton("checkbutton2", l->getShadowVolume().volume() > 1e-4);

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
    }*/
}

void VRGuiScene::setCamera(VRCameraPtr c) {
    if (!c) return;
    uiSignal( "on_sg_setup_cam", {
        {"acceptRoot", toString(c->getAcceptRoot())},
        {"aspect", toString(c->getAspect())},
        {"fov", toString(c->getFov())},
        {"near", toString(c->getNear())},
        {"far", toString(c->getFar())},
        {"projection", c->getType()}
    } );
}

void VRGuiScene::setGroup(VRGroupPtr g) {
    if (!g) return;
    /*setWidgetVisibility("expander2", true, true);
    setWidgetVisibility("expander9", true, true);
    setToggleButton("checkbutton23", g->getActive() );

    fillStringListstore("liststore3", g->getGroups());

    setCombobox("combobox14", getListStorePos("liststore3",g->getGroup()));*/
}

void VRGuiScene::setLod(VRLodPtr lod) {
    if (!lod) return;

    uiSignal( "on_sg_setup_lod", {
        {"center", toString(lod->getCenter())},
        {"distances", toString(lod->getDistances())}
    } );

    /*setWidgetVisibility("expander2", true, true);
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
    }*/
}

void VRGuiScene::setEntity(VREntityPtr e) {
    if (!e) return;

    vector<string> propNames;
    vector<string> propValues;
    for (auto& pvec : e->properties) {
        string val;
        for (auto& p : pvec.second) val += " " + p.second->value + " (" + p.second->type + ")";
        propNames.push_back(pvec.first);
        propValues.push_back(val);
    }

    uiSignal( "on_sg_setup_entity", {
        {"name", e->getName()},
        {"concepts", e->getConceptList()},
        {"propNames", toString(propNames)},
        {"propValues", toString(propValues)},
    } );
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
    /*setWidgetVisibility("expander1", false, true);
    setWidgetVisibility("expander2", false, true);
    setWidgetVisibility("expander9", false, true);
    setWidgetVisibility("expander10", false, true);
    setWidgetVisibility("expander11", false, true);
    setWidgetVisibility("expander12", false, true);
    setWidgetVisibility("expander13", false, true);
    setWidgetVisibility("expander14", false, true);
    setWidgetVisibility("expander15", false, true);
    setWidgetVisibility("expander16", false, true);
    setWidgetVisibility("expander27", false, true);*/
    if (disable) return;

    VRObjectPtr obj = getSelected();
    if (obj == 0) return;

    // set object label && path
    uiSignal("set_sg_title", {{ "title", obj->getName() + "\npath " + obj->getPath() }} );

    string type = obj->getType();
    trigger_cbs = false;

    setObject(obj);
    if (obj->hasTag("transform")) setTransform(dynamic_pointer_cast<VRTransform>(obj));
    if (obj->hasTag("geometry")) setGeometry(dynamic_pointer_cast<VRGeometry>(obj));

    if (type == "Light") setLight(dynamic_pointer_cast<VRLight>(obj));
    if (type == "Camera") setCamera(dynamic_pointer_cast<VRCamera>(obj));
    if (type == "Group") setGroup(dynamic_pointer_cast<VRGroup>(obj));
    if (type == "Lod") setLod(dynamic_pointer_cast<VRLod>(obj));

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

void VRGuiScene::setSGRow( VRObjectPtr o) {
    if (o == 0) return;
    //if (!itr) return;

    string fg, bg;
    getTypeColors(o, fg, bg);

    auto e = o->getEntity();

    string name = o->getName();
    if (e) name += "<sub><u><span foreground=\"blue\">" + e->getConceptList() + "</span></u></sub>";

    /*gtk_tree_store_set (tree_store, itr, 0, name.c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 1, o->getType().c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 2, o->getPath().c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 3, fg.c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 4, bg.c_str(), -1);
    gtk_tree_store_set (tree_store, itr, 5, o->getID(), -1);*/
}

void VRGuiScene::parseSGTree(VRObjectPtr o, string parent) {
    if (o == 0) return;

    string ID = toString(o->getID());
    uiSignal("on_sg_tree_append", {{ "ID",ID }, { "label",o->getName() }, { "parent",parent }});
    //setSGRow( &nItr, o );
    for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), ID );
}

void VRGuiScene::removeTreeStoreBranch(bool self) {
    /*int N = gtk_tree_store_iter_depth(tree_store, iter);

    if (!self) {
        int Nc = gtk_tree_model_iter_n_children((GtkTreeModel*)tree_store, iter);
        if (Nc == 0) return;
        gtk_tree_model_iter_children((GtkTreeModel*)tree_store, iter, iter);
    } else gtk_tree_store_remove(tree_store, iter); // removes iter and sets it to next row

    while (gtk_tree_store_iter_depth(tree_store, iter) > N) gtk_tree_store_remove(tree_store, iter); // removes iter and sets it to next row*/
}

void VRGuiScene::syncSGTree(VRObjectPtr o) {
    if (o == 0) return;
    //removeTreeStoreBranch(itr, false);
    //for (uint i=0; i<o->getChildrenCount(); i++) parseSGTree( o->getChild(i), itr );
}

void VRGuiScene::on_treeview_select(string sID) {
    selected = toInt(sID);
    updateObjectForms();

    selected_geometry.reset();
    if (getSelected() == 0) return;
    if (getSelected()->hasTag("geometry")) selected_geometry = dynamic_pointer_cast<VRGeometry>(getSelected());
}

void VRGuiScene::on_treeview_rename(string ID, string name) {
    if(!trigger_cbs) return;
    auto obj = getSelected();
    obj->setName(name);
    if (obj->getType() == "Camera") VRGuiSignals::get()->getSignal("camera_added")->triggerAll<VRDevice>();
    uiSignal("on_tv_node_rename", {{"treeview","scenegraph"}, {"node",toString(obj->getID())}, {"name",obj->getName()}});
}

// --------------------------
// ---------ObjectForms Callbacks
// --------------------------

// VRObjects
void VRGuiScene::on_toggle_visible(bool b) { getSelected()->setVisible(b); }
void VRGuiScene::on_toggle_throw_shadow(bool b) { getSelected()->setVisible(b, "SHADOW"); }
void VRGuiScene::on_toggle_pickable(bool b) { getSelected()->setPickable(b); }

// VRGroup
void VRGuiScene::on_groupsync_clicked() {
    if(!trigger_cbs) return;
    /*if(!selected_itr) return;

    VRGroupPtr obj = dynamic_pointer_cast<VRGroup>( getSelected() );
    obj->sync();
    syncSGTree(obj, selected_itr);*/
}

void VRGuiScene::on_scene_update() {
    updateTreeView();
}

void VRGuiScene::on_groupapply_clicked() {
    if(!trigger_cbs) return;
    /*if(!selected_itr) return;

    VRGroupPtr obj = dynamic_pointer_cast<VRGroup>( getSelected() );
    obj->apply();

    vector<VRGroupWeakPtr> grps = obj->getGroupObjects();
    for (auto grp : grps) {
        auto g = grp.lock();
        if (!g) continue;

        string path = g->getPath();
        GtkTreeIter itr;
        gtk_tree_model_get_iter_from_string((GtkTreeModel*)tree_store, &itr, path.c_str());
        syncSGTree(g, &itr);
    }*/
}

void VRGuiScene::on_change_group() {
    if(!trigger_cbs) return;

    VRGroupPtr obj = dynamic_pointer_cast<VRGroup>( getSelected() );
    //obj->setGroup(getComboboxText("combobox14"));
}

void VRGuiScene::on_group_edited() {
    if(!trigger_cbs) return;

    VRGroupPtr obj = dynamic_pointer_cast<VRGroup>( getSelected() );
    if (!obj) return;
    /*string old_group = getComboboxText("combobox14");
    string new_group = getTextEntry("entry41");

    // update group list
    GtkTreeIter itr;
    auto store = (GtkListStore*)VRGuiBuilder::get()->get_object("liststore3");
    gtk_list_store_append(store, &itr);
    gtk_list_store_set(store, &itr, 0, new_group.c_str(), -1);

    // add group
    obj->setGroup(new_group);
    setComboboxLastActive("combobox14");

    setTextEntry("entry41", "");*/
}

// VR3DEntities
void VRGuiScene::on_change_from(Vec3d v) {
    cout << "on_change_from " << v << endl;
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setFrom(v);
    else obj->setWorldPosition(v);
    //updateObjectForms(); // TODO: fix bug!
}

void VRGuiScene::on_change_at(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setAt(v);
    else obj->setWorldAt(v);
    updateObjectForms();
}

void VRGuiScene::on_change_dir(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setDir(v);
    else obj->setWorldDir(v);
    updateObjectForms();
}

void VRGuiScene::on_change_up(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setUp(v);
    else obj->setWorldUp(v);
    updateObjectForms();
}

void VRGuiScene::on_scale_changed(Vec3d v) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    if (transformModeLocal) obj->setScale(v);
    else obj->setWorldScale(v);
    updateObjectForms();
}

void VRGuiScene::on_change_lod_center(Vec3d v) {
    if(!trigger_cbs) return;
    VRLodPtr obj = dynamic_pointer_cast<VRLod>( getSelected() );
    if (!obj) return;
    obj->setCenter(v);
    updateObjectForms();
}

void VRGuiScene::on_focus_clicked() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->getActiveCamera()->focusObject( obj );
}

void VRGuiScene::on_identity_clicked() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (!obj) return;
    obj->setIdentity();
    updateObjectForms();
}

void VRGuiScene::on_constraint_set_active(bool b) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (auto c = obj->getConstraint()) c->setActive(b);
}

void VRGuiScene::on_constraint_set_local(bool b) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (auto c = obj->getConstraint()) c->setLocal(b);
}

void VRGuiScene::on_constraint_lock_rotation(bool b) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (auto c = obj->getConstraint()) c->lockRotation(b);
    setTransform( obj );
}

void VRGuiScene::on_constraint_set_dof(int dof, double min, double max) {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    if (auto c = obj->getConstraint()) c->setMinMax(dof, min, max);
}

// geometry
void VRGuiScene::on_toggle_mesh_visible(bool b) {
    VRGeometryPtr obj = dynamic_pointer_cast<VRGeometry>( getSelected() );
    obj->setMeshVisibility(b);
}

void VRGuiScene::on_change_primitive() {
    if(!trigger_cbs) return;
    /*string prim = getComboboxText("combobox21");

    VRGeometryPtr obj = dynamic_pointer_cast<VRGeometry>( getSelected() );

    obj->setPrimitive(prim);
    updateObjectForms();*/
}

void VRGuiScene::on_edit_primitive_params(const char* path_string, const char* new_text) {
    if (!trigger_cbs) return;

    /*string prim = getComboboxText("combobox21");
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

    VRGeometryPtr obj = dynamic_pointer_cast<VRGeometry>( getSelected() );
    obj->setPrimitive(prim + " " + args);*/
}

void VRGuiScene::on_edit_distance(const char* path_string, const char* new_text) {
    if(!trigger_cbs) return;
    VRLodPtr lod = dynamic_pointer_cast<VRLod>( getSelected() );

    /*VRGuiTreeView tree_view("treeview8");
    tree_view.setSelectedStringValue(1, new_text);

    float f = toFloat(new_text);
    int i = toInt(path_string);
    lod->setDistance(i,f);*/
}

void VRGuiScene::on_lod_decimate_changed() {
    if(!trigger_cbs) return;
    VRLodPtr lod = dynamic_pointer_cast<VRLod>( getSelected() );
    //lod->setDecimate(getCheckButtonState("checkbutton35"), toInt(getTextEntry("entry9")));
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

void VRGuiScene::on_toggle_camera_accept_realroot(bool b) {
    if(!trigger_cbs) return;
    VRCameraPtr obj = dynamic_pointer_cast<VRCamera>( getSelected() );
    if (obj) obj->setAcceptRoot(b);
}




// --------------------------
// ---------Menu callbacks
// --------------------------

template <class T>
void VRGuiScene::on_menu_add(string sID) {
    selected = toInt(sID);
    VRObjectPtr obj = getSelected();
    if (obj == 0) return;
    auto child = T::create("NEW");
    getSelected()->addChild(child);
    updateTreeView();
}

void VRGuiScene::on_menu_add_animation(string sID) {
    //if(!selected_itr) return;
    //VRAnimation* obj = new VRAnimation("None");
    //getSelected()->addChild(obj);
    //parseSGTree(obj, selected_itr);
}

void VRGuiScene::on_menu_add_file(string sID) {
    /*if(!selected_itr) return;
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
    VRGuiFile::open( "Load", "open", "Load geometric data" );*/
}

void VRGuiScene::on_menu_add_light(string sID) {
    selected = toInt(sID);
    VRObjectPtr obj = getSelected();
    if (obj == 0) return;
    auto light = VRLight::create("NEW-light");
    VRLightBeaconPtr lb = VRLightBeacon::create("NEW-light-beacon");
    light->addChild(lb);
    light->setBeacon(lb);
    getSelected()->addChild(light);
    updateTreeView();
}

void VRGuiScene::on_menu_add_camera(string sID) {
    selected = toInt(sID);
    VRObjectPtr obj = getSelected();
    if (obj == 0) return;
    auto cam = VRCamera::create("NEW-camera");
    getSelected()->addChild(cam);
    updateTreeView();
    VRGuiSignals::get()->getSignal("camera_added")->triggerAll<VRDevice>();
}

void VRGuiScene::on_menu_delete(string sID) {
    selected = toInt(sID);
    VRObjectPtr obj = getSelected();
    if (obj == 0) return;

    // todo: check for camera!!
    obj->destroy();
    selected = -1;
    updateTreeView();
}

void VRGuiScene::on_menu_copy(string sID) {
    //if(!selected_itr) return;
    //VRGuiScene_copied = getSelected();
}

void VRGuiScene::on_menu_paste(string sID) {
    /*if(!selected_itr) return;
    auto obj = VRGuiScene_copied.lock();
    if (obj == 0) return;

    VRObjectPtr tmp = obj->duplicate();
    tmp->switchParent(getSelected());
    VRGuiScene_copied.reset();

    parseSGTree(tmp, selected_itr);*/
}

/*void VRGuiScene::on_menu_add_csg() {
    if(!selected_itr) return;
    CSGGeometryPtr g = new CSGGeometry("csg_geo");
    getSelected()->addChild(g);
    parseSGTree(g, selected_itr);
}*/

void VRGuiScene::on_collada_import_clicked() {
    /*string rel_path = VRGuiFile::getRelativePath_toWorkdir();
    string path = VRGuiFile::getPath(); // absolute path
    cout << "Data import: " << path << ", relative: " << rel_path << endl;
    VRGuiFile::close();

    // import stuff
    VRObjectPtr tmp = VRImport::get()->load(rel_path, getSelected());
    parseSGTree(tmp, selected_itr);*/
}

void VRGuiScene::on_treeview_drop(string sID, string tID) {
    auto scene = VRScene::getCurrent();
    if (!scene) return;

    VRObjectPtr source = scene->get( toInt(sID) );
    VRObjectPtr target = scene->get( toInt(tID) );
    if ( !source || !target ) return;

    source->switchParent(target, 0);
    updateTreeView();
}


// ------------- transform -----------------------

void VRGuiScene::on_toggle_global(bool global) {
    if (!trigger_cbs) return;
    transformModeLocal = !global;

    VRObjectPtr obj = getSelected();
    if (obj) {
        if (obj->hasTag("transform")) setTransform(dynamic_pointer_cast<VRTransform>(obj));
    }
}

void VRGuiScene::on_toggle_phys() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );

    /*bool phys = getCheckButtonState("checkbutton13");
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()) obj->getPhysics()->setPhysicalized(phys);
#endif
    setWidgetSensitivity("combobox8", phys);*/
}

void VRGuiScene::on_toggle_dynamic() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );

    /*bool dyn = getCheckButtonState("checkbutton33");
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()) obj->getPhysics()->setDynamic(dyn);
#endif*/
}

void VRGuiScene::on_mass_changed() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );

    /*string m = getTextEntry("entry59");
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()) obj->getPhysics()->setMass(toFloat(m));
#endif*/
}

void VRGuiScene::on_change_phys_shape() {
    if(!trigger_cbs) return;
    VRTransformPtr obj = dynamic_pointer_cast<VRTransform>( getSelected() );
    /*string t = getComboboxText("combobox8");
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()) obj->getPhysics()->setShape(t);
#endif*/
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
void VRGuiScene::on_cam_aspect_changed(float v) {
    if(!trigger_cbs) return;
    VRCameraPtr obj = dynamic_pointer_cast<VRCamera>( getSelected() );
    obj->setAspect(v);
}

void VRGuiScene::on_cam_fov_changed(float v) {
    if(!trigger_cbs) return;
    VRCameraPtr obj = dynamic_pointer_cast<VRCamera>( getSelected() );
    obj->setFov(v);
}

void VRGuiScene::on_cam_near_changed(float v) {
    if(!trigger_cbs) return;
    VRCameraPtr obj = dynamic_pointer_cast<VRCamera>( getSelected() );
    obj->setNear(v);
}

void VRGuiScene::on_cam_far_changed(float v) {
    if(!trigger_cbs) return;
    VRCameraPtr obj = dynamic_pointer_cast<VRCamera>( getSelected() );
    obj->setFar(v);
}

void VRGuiScene::on_change_cam_proj(string mode) {
    if(!trigger_cbs) return;
    VRCameraPtr obj = dynamic_pointer_cast<VRCamera>( getSelected() );
    obj->setType(mode);
}
// ----------------------------------------------

// ------------- light -----------------------
void VRGuiScene::on_toggle_light(bool b) {
    if(!trigger_cbs) return;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    obj->setOn(b);
}

void VRGuiScene::on_toggle_light_shadow(bool b) {
    if(!trigger_cbs) return;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    obj->setShadows(b);
}

void VRGuiScene::on_toggle_light_shadow_volume(bool b, float D) {
    if(!trigger_cbs) return;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    Boundingbox bb;
    if (!b) obj->setShadowVolume( bb );
    else {
        bb.inflate( 0.5*D );
        cout << "VRGuiScene::on_toggle_light_shadow_volume " << 0.5*D << " " << bb.volume() << " " << bb.size() << endl;
        obj->setShadowVolume( bb );
    }
}

void VRGuiScene::on_change_light_type(string type) {
    if(!trigger_cbs) return;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    obj->setType(type);
}

void VRGuiScene::on_change_light_shadow(int res) {
    if(!trigger_cbs) return;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    obj->setShadowMapRes(res);
}

void VRGuiScene::on_edit_light_attenuation(Vec3d a) {
    if(!trigger_cbs) return;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return;
    obj->setAttenuation(a);
}

bool VRGuiScene::setShadow_color(Color4f c) {
    if(!trigger_cbs) return true;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return true;
    obj->setShadowColor(c);
    return true;
}

bool VRGuiScene::setLight_diff_color(Color4f c) {
    if(!trigger_cbs) return true;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return true;
    obj->setDiffuse(c);
    return true;
}

bool VRGuiScene::setLight_amb_color(Color4f c) {
    if(!trigger_cbs) return true;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return true;
    obj->setAmbient(c);
    return true;
}

bool VRGuiScene::setLight_spec_color(Color4f c) {
    if(!trigger_cbs) return true;
    VRLightPtr obj = dynamic_pointer_cast<VRLight>( getSelected() );
    if (!obj) return true;
    obj->setSpecular(c);
    return true;
}
// ----------------------------------------------


// ------------- material -----------------------
void VRGuiScene::setMaterial_gui() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setMaterial_lit(bool b) {
    if(!trigger_cbs) return;
    auto geo = selected_geometry.lock();
    if(!geo) return;
    geo->getMaterial()->setLit(b);
}

void VRGuiScene::setMaterial_meshcolors(bool b) {
    if(!trigger_cbs) return;
    auto geo = selected_geometry.lock();
    if(!geo) return;
    geo->getMaterial()->ignoreMeshColors(!b);
}

bool VRGuiScene::setMaterial_diffuse(Color4f c) {
    if(!trigger_cbs) return true;
    auto geo = selected_geometry.lock();
    if(!geo) return true;
    geo->getMaterial()->setDiffuse(toColor3f(c));
    return true;
}

bool VRGuiScene::setMaterial_specular(Color4f c) {
    if(!trigger_cbs) return true;
    auto geo = selected_geometry.lock();
    if(!geo) return true;
    geo->getMaterial()->setSpecular(toColor3f(c));
    return true;
}

bool VRGuiScene::setMaterial_ambient(Color4f c) {
    if(!trigger_cbs) return true;
    auto geo = selected_geometry.lock();
    if(!geo) return true;
    geo->getMaterial()->setAmbient(toColor3f(c));
    return true;
}

bool VRGuiScene::setMaterial_emission(Color4f c) {
    if(!trigger_cbs) return true;
    auto geo = selected_geometry.lock();
    if(!geo) return true;
    geo->getMaterial()->setEmission(toColor3f(c));
    return true;
}

void VRGuiScene::setMaterial_pointsize(int ps) {
    if(!trigger_cbs) return;
    auto geo = selected_geometry.lock();
    if(!geo) return;
    geo->getMaterial()->setPointSize(ps);
}

void VRGuiScene::setMaterial_linewidth(int ps) {
    if(!trigger_cbs) return;
    auto geo = selected_geometry.lock();
    if(!geo) return;
    geo->getMaterial()->setLineWidth(ps);
}

void VRGuiScene::setMaterial_texture_toggle() {
    cout << "\nNot yet implemented\n";
}

void VRGuiScene::setMaterial_texture_name() {
    cout << "\nNot yet implemented\n";
}
// ----------------------------------------------

void VRGuiScene::initMenu() {
    /*menu = new VRGuiContextMenu("SGMenu");
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
    //menu->appendItem("SGM_AddUrbBuiMenu", "Device", bind(&VRGuiScene::on_menu_add<VRElectricDevice>) );*/
}

void VRGuiScene::initCallbacks() {
    //setToggleButtonCallback("checkbutton3", bind(&VRGuiScene::setMaterial_lit, this));
}

// TODO: remove groups
VRGuiScene::VRGuiScene() { // TODO: reduce callbacks with templated functions
    /*dragDest.reset();

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
    setToggleButtonCallback("checkbutton18", bind(&VRGuiScene::on_toggle_rc, this));
    setToggleButtonCallback("checkbutton19", bind(&VRGuiScene::on_toggle_rc, this));
    setToggleButtonCallback("checkbutton20", bind(&VRGuiScene::on_toggle_rc, this));
    setToggleButtonCallback("checkbutton21", bind(&VRGuiScene::on_toggle_T_constraint, this));
    setToggleButtonCallback("checkbutton22", bind(&VRGuiScene::on_toggle_R_constraint, this));
    //setToggleButtonCallback("checkbutton27", on_toggle_CSG_editmode);
    setToggleButtonCallback("checkbutton17", bind(&VRGuiScene::on_toggle_camera_accept_realroot, this));
    setToggleButtonCallback("radiobutton1", bind(&VRGuiScene::on_toggle_T_constraint_mode, this) );
    setToggleButtonCallback("checkbutton31", bind(&VRGuiScene::on_toggle_light, this) );
    setToggleButtonCallback("checkbutton32", bind(&VRGuiScene::on_toggle_light_shadow, this) );
    setToggleButtonCallback("checkbutton2", bind(&VRGuiScene::on_toggle_light_shadow_volume, this) );
    setToggleButtonCallback("checkbutton13", bind(&VRGuiScene::on_toggle_phys, this) );
    setToggleButtonCallback("checkbutton33", bind(&VRGuiScene::on_toggle_dynamic, this) );
    setToggleButtonCallback("checkbutton35", bind(&VRGuiScene::on_lod_decimate_changed, this) );

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

    updateObjectForms();*/

    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("ui_change_scene_tab", [&](OSG::VRGuiSignals::Options o) { if (o["tab"] == "Scenegraph") updateTreeView(); return true; }, true );
    mgr->addCallback("treeview_select", [&](OSG::VRGuiSignals::Options o) { if (o["treeview"] == "scenegraph") on_treeview_select( o["node"] ); return true; }, true );
    mgr->addCallback("treeview_rename", [&](OSG::VRGuiSignals::Options o) { if (o["treeview"] == "scenegraph") on_treeview_rename( o["node"], o["name"] ); return true; }, true );
    mgr->addCallback("treeview_drop", [&](OSG::VRGuiSignals::Options o) { if (o["treeview"] == "scenegraph") on_treeview_drop( o["source"], o["target"] ); return true; }, true );

    mgr->addCallback("sg_menu_delete",
            [&](OSG::VRGuiSignals::Options o) {
                if (o["treeview"] == "scenegraph") {
                    toBeDeleted = o["ID"];
                    uiSignal("askUser", {{"msg1","This will remove the selected object!"}, {"msg2","Are you sure?"}, {"sig","sg_menu_delete_confirmed"}});
                }
                return true;
            }, true
        );

    mgr->addCallback("sg_menu_delete_confirmed", [&](OSG::VRGuiSignals::Options o) { on_menu_delete( toBeDeleted ); return true; }, true );
    mgr->addCallback("sg_menu_newObject", [&](OSG::VRGuiSignals::Options o) { on_menu_add<VRObject>( o["ID"] ); return true; }, true );
    mgr->addCallback("sg_menu_newTransform", [&](OSG::VRGuiSignals::Options o) { on_menu_add<VRTransform>( o["ID"] ); return true; }, true );
    mgr->addCallback("sg_menu_newCamera", [&](OSG::VRGuiSignals::Options o) { on_menu_add_camera( o["ID"] ); return true; }, true );
    mgr->addCallback("sg_menu_newLight", [&](OSG::VRGuiSignals::Options o) { on_menu_add_light( o["ID"] ); return true; }, true );
    mgr->addCallback("sg_menu_newGeometry", [&](OSG::VRGuiSignals::Options o) { on_menu_add<VRGeometry>( o["ID"] ); return true; }, true );

    mgr->addCallback("sg_toggle_visible", [&](OSG::VRGuiSignals::Options o) { on_toggle_visible(toBool(o["visible"])); return true; }, true );
    mgr->addCallback("sg_toggle_pickable", [&](OSG::VRGuiSignals::Options o) { on_toggle_pickable(toBool(o["pickable"])); return true; }, true );
    mgr->addCallback("sg_toggle_cast_shadow", [&](OSG::VRGuiSignals::Options o) { on_toggle_throw_shadow(toBool(o["castShadow"])); return true; }, true );
    mgr->addCallback("sg_toggle_global", [&](OSG::VRGuiSignals::Options o) { on_toggle_global(toBool(o["global"])); return true; }, true );

    mgr->addCallback("sg_set_position", [&](OSG::VRGuiSignals::Options o) { on_change_from(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("sg_set_atvector", [&](OSG::VRGuiSignals::Options o) { on_change_at(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("sg_set_direction", [&](OSG::VRGuiSignals::Options o) { on_change_dir(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("sg_set_upvector", [&](OSG::VRGuiSignals::Options o) { on_change_up(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("sg_set_scale", [&](OSG::VRGuiSignals::Options o) { on_scale_changed(Vec3d(toFloat(o["x"]), toFloat(o["y"]), toFloat(o["z"]))); return true; }, true );
    mgr->addCallback("sg_focus_transform", [&](OSG::VRGuiSignals::Options o) { on_focus_clicked(); return true; }, true );
    mgr->addCallback("sg_set_identity", [&](OSG::VRGuiSignals::Options o) { on_identity_clicked(); return true; }, true );
    mgr->addCallback("sg_set_constraint_active", [&](OSG::VRGuiSignals::Options o) { on_constraint_set_active(toBool(o["active"])); return true; }, true );
    mgr->addCallback("sg_set_constraint_local", [&](OSG::VRGuiSignals::Options o) { on_constraint_set_local(toBool(o["local"])); return true; }, true );
    mgr->addCallback("sg_set_constraint_lock_rotation", [&](OSG::VRGuiSignals::Options o) { on_constraint_lock_rotation(1); return true; }, true );
    mgr->addCallback("sg_set_constraint_unlock_rotation", [&](OSG::VRGuiSignals::Options o) { on_constraint_lock_rotation(0); return true; }, true );
    mgr->addCallback("sg_set_constraint_dof", [&](OSG::VRGuiSignals::Options o) { on_constraint_set_dof(toInt(o["dof"]), toFloat(o["min"]), toFloat(o["max"])); return true; }, true );

    mgr->addCallback("sg_toggle_mesh_visible", [&](OSG::VRGuiSignals::Options o) { on_toggle_mesh_visible(toBool(o["visible"])); return true; }, true );

    mgr->addCallback("sg_set_cam_accept_root", [&](OSG::VRGuiSignals::Options o) { on_toggle_camera_accept_realroot(toBool(o["value"])); return true; }, true );
    mgr->addCallback("sg_set_cam_aspect", [&](OSG::VRGuiSignals::Options o) { on_cam_aspect_changed(toFloat(o["value"])); return true; }, true );
    mgr->addCallback("sg_set_cam_fov", [&](OSG::VRGuiSignals::Options o) { on_cam_fov_changed(toFloat(o["value"])); return true; }, true );
    mgr->addCallback("sg_set_cam_near", [&](OSG::VRGuiSignals::Options o) { on_cam_near_changed(toFloat(o["value"])); return true; }, true );
    mgr->addCallback("sg_set_cam_far", [&](OSG::VRGuiSignals::Options o) { on_cam_far_changed(toFloat(o["value"])); return true; }, true );
    mgr->addCallback("sg_set_cam_projection", [&](OSG::VRGuiSignals::Options o) { on_change_cam_proj(o["projection"]); return true; }, true );

    mgr->addCallback("sg_set_light_state", [&](OSG::VRGuiSignals::Options o) { on_toggle_light(toBool(o["state"])); return true; }, true );
    mgr->addCallback("sg_set_shadow", [&](OSG::VRGuiSignals::Options o) { on_toggle_light_shadow(toBool(o["state"])); return true; }, true );
    mgr->addCallback("sg_set_shadow_volume", [&](OSG::VRGuiSignals::Options o) { on_toggle_light_shadow_volume(toBool(o["state"]), toFloat(o["volume"])); return true; }, true );
    mgr->addCallback("sg_set_light_type", [&](OSG::VRGuiSignals::Options o) { on_change_light_type(o["type"]); return true; }, true );
    mgr->addCallback("sg_set_shadow_resolution", [&](OSG::VRGuiSignals::Options o) { on_change_light_shadow(toInt(o["resolution"])); return true; }, true );
    mgr->addCallback("sg_set_light_attenuation", [&](OSG::VRGuiSignals::Options o) { on_edit_light_attenuation(toValue<Vec3d>(o["attenuation"])); return true; }, true );
    mgr->addCallback("sg_set_shadow_color", [&](OSG::VRGuiSignals::Options o) { setShadow_color(toValue<Color4f>(o["color"])); return true; }, true );
    mgr->addCallback("sg_set_light_diffuse", [&](OSG::VRGuiSignals::Options o) { setLight_diff_color(toValue<Color4f>(o["color"])); return true; }, true );
    mgr->addCallback("sg_set_light_ambient", [&](OSG::VRGuiSignals::Options o) { setLight_amb_color(toValue<Color4f>(o["color"])); return true; }, true );
    mgr->addCallback("sg_set_light_specular", [&](OSG::VRGuiSignals::Options o) { setLight_spec_color(toValue<Color4f>(o["color"])); return true; }, true );

    mgr->addCallback("sg_set_mat_ambient", [&](OSG::VRGuiSignals::Options o) { setMaterial_ambient(toValue<Color4f>(o["color"])); return true; }, true );
    mgr->addCallback("sg_set_mat_diffuse", [&](OSG::VRGuiSignals::Options o) { setMaterial_diffuse(toValue<Color4f>(o["color"])); return true; }, true );
    mgr->addCallback("sg_set_mat_specular", [&](OSG::VRGuiSignals::Options o) { setMaterial_specular(toValue<Color4f>(o["color"])); return true; }, true );
    mgr->addCallback("sg_set_mat_emission", [&](OSG::VRGuiSignals::Options o) { setMaterial_emission(toValue<Color4f>(o["color"])); return true; }, true );
    mgr->addCallback("sg_set_mat_lit", [&](OSG::VRGuiSignals::Options o) { setMaterial_lit(toBool(o["state"])); return true; }, true );
    mgr->addCallback("sg_set_mat_meshcolors", [&](OSG::VRGuiSignals::Options o) { setMaterial_meshcolors(toBool(o["state"])); return true; }, true );
    mgr->addCallback("sg_set_mat_pointsize", [&](OSG::VRGuiSignals::Options o) { setMaterial_pointsize(toInt(o["selection"])); return true; }, true );
    mgr->addCallback("sg_set_mat_linewidth", [&](OSG::VRGuiSignals::Options o) { setMaterial_linewidth(toInt(o["selection"])); return true; }, true );
}

// new scene, update stuff here
bool VRGuiScene::updateTreeView() {
	cout << "VRGuiScene::updateTreeView" << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return true;

    uiSignal("on_sg_tree_clear");
    VRObjectPtr root = scene->getRoot();
    parseSGTree( root );
    //gtk_tree_view_expand_all(tree_view);

    //setWidgetSensitivity("table11", false);
    return true;
}

// check if currently getSelected() object has been modified
void VRGuiScene::update() {
    if (!liveUpdate) return;
    updateObjectForms();
}

void VRGuiScene::selectObject(VRObjectPtr obj) {
    /*string path = obj->getPath();
    GtkTreePath* tpath = gtk_tree_path_new_from_string(path.c_str());
    GtkTreeViewColumn* focus_column = gtk_tree_view_get_column(tree_view, 0);
    gtk_tree_view_expand_to_path(tree_view, tpath);
    gtk_tree_view_set_cursor(tree_view, tpath, focus_column, false);
    gtk_tree_path_free(tpath);

    setWidgetSensitivity("table11", true);

    updateObjectForms(true);
    selected = obj->getID();
    updateObjectForms();

    selected_geometry.reset();
    if (obj && obj->hasTag("geometry")) selected_geometry = dynamic_pointer_cast<VRGeometry>(obj);*/
}

OSG_END_NAMESPACE;
