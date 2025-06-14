#include "VRImguiSceneScenegraph.h"
#include "imWidgets/VRImguiInput.h"

#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

using namespace OSG;

ImScenegraph::ImScenegraph() :  tree("scenegraph"),
                                position("pos", "pos"),
                                atvector("at", "at"),
                                direction("dir", "dir"),
                                upvector("up", "up"),
                                scale("scale", "scale"),
                                constrTranslation("cTrans", "cTrans"),
                                camAspect("sgCamAspect", "aspect", "1"),
                                camFov("sgCamFov", "fov", "1.0472"),
                                camNear("sgCamNear", "near", "0.1"),
                                camFar("sgCamFar", "far", "512"),
                                constrDof1("dof0", "DoF 0 tx"),
                                constrDof2("dof1", "DoF 1 ty"),
                                constrDof3("dof2", "DoF 2 tz"),
                                constrDof4("dof3", "DoF 3 rx"),
                                constrDof5("dof4", "DoF 4 ry"),
                                constrDof6("dof5", "DoF 5 rz"),
                                lodCenter("lodCenter", "center"),
                                matAmbient("colAmbient", "Ambient: "),
                                matDiffuse("colDiffuse", "Diffuse: "),
                                matSpecular("colSpecular", "Specular:"),
                                matEmission("colEmission", "Emission:") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("set_sg_title", [&](OSG::VRGuiSignals::Options o){ title = o["title"]; return true; } );
    mgr->addCallback("on_sg_tree_clear", [&](OSG::VRGuiSignals::Options o){ treeClear(); return true; } );
    mgr->addCallback("on_sg_tree_append", [&](OSG::VRGuiSignals::Options o) {
            treeAppend(o["ID"], o["label"], o["parent"], o["type"], o["cla"], o["mod"], o["col"]);
        return true; } );

    mgr->addCallback("on_sg_set_obj_type", [&](OSG::VRGuiSignals::Options o){ objType = o["objType"]; return true; } );
    mgr->addCallback("on_sg_setup_obj", [&](OSG::VRGuiSignals::Options o){ setupObject(o); return true; } );
    mgr->addCallback("on_sg_setup_trans", [&](OSG::VRGuiSignals::Options o){ setupTransform(o); return true; } );
    mgr->addCallback("on_sg_setup_lod", [&](OSG::VRGuiSignals::Options o){ setupLod(o); return true; } );
    mgr->addCallback("on_sg_setup_cam", [&](OSG::VRGuiSignals::Options o){ setupCamera(o); return true; } );
    mgr->addCallback("on_sg_setup_light", [&](OSG::VRGuiSignals::Options o){ setupLight(o); return true; } );
    mgr->addCallback("on_sg_setup_geo", [&](OSG::VRGuiSignals::Options o){ setupGeometry(o); return true; } );
    mgr->addCallback("on_sg_setup_mat", [&](OSG::VRGuiSignals::Options o){ setupMaterial(o); return true; } );

    camProjections = {"perspective", "orthographic"};
    lightTypes = {"point", "directional", "spot", "photometric"};
    shadowResolutions = {"1024", "2048", "4096", "8192"};
}

void ImScenegraph::render() {
    auto region1 = ImGui::GetContentRegionAvail();
    auto region2 = ImGui::GetContentRegionAvail();
    region1.x *= 0.5;
    region2.x *= 0.5;
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::BeginChild("sgTree", region1, false, flags);
    tree.render();
    ImGui::EndChild();

    ImGui::SameLine();

    bool isGeometry = (objType == "Geometry" || objType == "Sprite");
    bool isTransform = (isGeometry || objType == "Transform" || objType == "Camera" || objType == "LightBeacon");
    bool isMaterial = (isGeometry || objType == "Material");

    ImGui::BeginChild("scProps", region2, false, flags);
        // object
        ImGui::Text(("Object: " + selected).c_str());
        ImGui::Indent(10);
            ImGui::Text(("Parent: " + parent).c_str());
            ImGui::Text(("Persistency: " + persistency).c_str());

            if (ImGui::Checkbox("visible", &visible)) uiSignal( "sg_toggle_visible", {{"visible",toString(visible)}} );
            ImGui::SameLine();
            if (ImGui::Checkbox("pickable", &pickable)) uiSignal( "sg_toggle_pickable", {{"pickable",toString(pickable)}} );
            ImGui::SameLine();
            if (ImGui::Checkbox("cast shadow", &castShadow)) uiSignal( "sg_toggle_cast_shadow", {{"castShadow",toString(castShadow)}} );
        ImGui::Unindent(10);

        // geometry
        if (isGeometry) {
            ImGui::Separator();
            ImGui::Text("Geometry:");
            ImGui::Indent(10);
            ImGui::Text(("Origin: " + geoOrigin).c_str());
            if (geoOrigin == "primitive") {
                ImGui::Text(("Primitive: " + geoParams[0]).c_str());
                for (int i=1; i<geoParams.size(); i++) {
                    ImGui::Text((" param " + geoParamNames[i-1] + ": " + geoParams[i]).c_str());
                }
            }
            for (auto& p : geoData) ImGui::Text((" data " + p.first + " : " + toString(p.second)).c_str());
            ImGui::Unindent(10);
        }

        // material
        if (isMaterial) {
            ImGui::Separator();
            ImGui::Text(("Material: " + matName).c_str());
            ImGui::Indent(10);

            if (matName != "") {
                if (matAmbient.render())  matAmbient.signal("sg_set_mat_ambient");
                if (matDiffuse.render())  matDiffuse.signal("sg_set_mat_diffuse");
                if (matSpecular.render()) matSpecular.signal("sg_set_mat_specular");
                if (matEmission.render()) matEmission.signal("sg_set_mat_emission");
                if (ImGui::Checkbox("Lit", &matLit)) uiSignal("sg_set_mat_lit", {{"state",toString(matLit)}});
                if (ImGui::Checkbox("Use mesh colors", &matMeshColors)) uiSignal("sg_set_mat_meshcolors", {{"state",toString(matMeshColors)}});
            }

            if (texDims != "") {
                ImGui::Text(("Tex dimensions: " + texDims).c_str());
                ImGui::Text(("Tex size (mb): " + texSize).c_str());
                ImGui::Text(("Tex N channels: " + texChannels).c_str());
            }

            ImGui::Unindent(10);
        }

        // camera
        if (objType == "Camera") {
            ImGui::Separator();
            ImGui::Text("Camera:");
            ImGui::Indent(10);
                if (ImGui::Checkbox("accept setup root:", &doAcceptRoot)) uiSignal("sg_set_cam_accept_root", {{"value", toString(doAcceptRoot)}});
                if (camAspect.render(50)) uiSignal("sg_set_cam_aspect", {{"value", camAspect.value}});
                ImGui::SameLine();
                if (camFov.render(50)) uiSignal("sg_set_cam_fov", {{"value", camFov.value}});
                if (camNear.render(50)) uiSignal("sg_set_cam_near", {{"value", camNear.value}});
                ImGui::SameLine();
                if (camFar.render(50)) uiSignal("sg_set_cam_far", {{"value", camFar.value}});

                ImGui::Text("Projection:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                if (ImGui::Combo("##camProj", &camProjection, &camProjections[0], camProjections.size())) {
                    uiSignal("sg_set_cam_projection", {{"projection",camProjections[camProjection]}});
                }
            ImGui::Unindent(10);
        }

        // light
        if (objType == "Light") {
            ImGui::Separator();
            ImGui::Text("Light:");
            ImGui::Indent(10);
                if (ImGui::Checkbox("Active", &lightOn)) uiSignal("sg_set_light_state", {{"state",toString(lightOn)}});
                ImGui::Text("Type:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                if (ImGui::Combo("##LightTypes", &lightType, &lightTypes[0], lightTypes.size())) {
                    uiSignal("sg_set_light_type", {{"type",lightTypes[lightType]}});
                }
                if (ImGui::Checkbox("Shadows", &shadowsOn)) uiSignal("sg_set_shadow", {{"state",toString(shadowsOn)}});
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                if (ImGui::Combo("##ShadowRes", &shadowResolution, &shadowResolutions[0], shadowResolutions.size())) {
                    uiSignal("sg_set_shadow_resolution", {{"resolution",shadowResolutions[shadowResolution]}});
                }
            ImGui::Unindent(10);
        }

        // lod
        if (objType == "Lod") {
            ImGui::Separator();
            ImGui::Text("LoD:");
            ImGui::Indent(10);
                if (lodCenter.render(region2.x-10)) lodCenter.signal("sg_set_lod_center");
                for (int i=0; i<lodDistances.size(); i++) {
                    string lbl = "child " + toString(i) + ", distance: " + toString(lodDistances[i]);
                    ImGui::Text(lbl.c_str());
                }
            ImGui::Unindent(10);
        }

        // transform
        if (isTransform) {
            ImGui::Separator();
            ImGui::Text("Transformation:");
            ImGui::Indent(10);
                if (ImGui::Button("Focus")) uiSignal( "sg_focus_transform");
                ImGui::SameLine();
                if (ImGui::Button("Identity")) uiSignal( "sg_set_identity");

                if (position.render(region2.x-10)) position.signal("sg_set_position");
                if (atvector.render(region2.x-10)) atvector.signal("sg_set_atvector");
                if (direction.render(region2.x-10)) direction.signal("sg_set_direction");
                if (upvector.render(region2.x-10)) upvector.signal("sg_set_upvector");
                if (scale.render(region2.x-10)) scale.signal("sg_set_scale");

                if (ImGui::Checkbox("global", &global)) uiSignal( "sg_toggle_global", {{"global",toString(global)}} );
            ImGui::Unindent(10);

            ImGui::Text("Constraints:");
            ImGui::Indent(10);
                if (ImGui::Checkbox("active", &constrActive)) uiSignal( "sg_set_constraint_active", {{"active",toString(constrActive)}} );
                if (constrActive) {
                    int L = region2.x-10;
                    ImGui::SameLine();
                    if (ImGui::Checkbox("local", &constrLocal)) uiSignal( "sg_set_constraint_local", {{"local",toString(constrLocal)}} );
                    if (constrDof1.render(L)) uiSignal( "sg_set_constraint_dof", {{"dof",toString(0)}, {"min",toString(constrDof1.vX)}, {"max",toString(constrDof1.vY)}} );
                    if (constrDof2.render(L)) uiSignal( "sg_set_constraint_dof", {{"dof",toString(1)}, {"min",toString(constrDof2.vX)}, {"max",toString(constrDof2.vY)}} );
                    if (constrDof3.render(L)) uiSignal( "sg_set_constraint_dof", {{"dof",toString(2)}, {"min",toString(constrDof3.vX)}, {"max",toString(constrDof3.vY)}} );
                    if (constrDof4.render(L)) uiSignal( "sg_set_constraint_dof", {{"dof",toString(3)}, {"min",toString(constrDof4.vX)}, {"max",toString(constrDof4.vY)}} );
                    if (constrDof5.render(L)) uiSignal( "sg_set_constraint_dof", {{"dof",toString(4)}, {"min",toString(constrDof5.vX)}, {"max",toString(constrDof5.vY)}} );
                    if (constrDof6.render(L)) uiSignal( "sg_set_constraint_dof", {{"dof",toString(5)}, {"min",toString(constrDof6.vX)}, {"max",toString(constrDof6.vY)}} );
                    ImGui::Text("Rotation:");
                    ImGui::SameLine();
                    if (ImGui::Button("lock")) uiSignal( "sg_set_constraint_lock_rotation" );
                    ImGui::SameLine();
                    if (ImGui::Button("unlock")) uiSignal( "sg_set_constraint_unlock_rotation" );
                }
            ImGui::Unindent(10);

            ImGui::Text("Physics:");
            ImGui::Indent(10);
                if (ImGui::Checkbox("physicalize:", &doPhysicalize)) ;//uiSignal( "sg_toggle_global", {{"global",toString(global)}} );
                if (doPhysicalize) {
                    ImGui::Indent(10);
                    // combobox for shape type
                    // mass
                    if (ImGui::Checkbox("dynamic", &physDynamic)) ;
                    ImGui::Unindent(10);
                }
            ImGui::Unindent(10);
        }

    ImGui::EndChild();
}

void ImScenegraph::setupObject(OSG::VRGuiSignals::Options o) {
    visible = toBool(o["visible"]);
    parent = o["parent"];
    persistency = o["persistency"];
    pickable = toBool(o["pickable"]);
    castShadow = toBool(o["castShadow"]);
    selected = o["name"];
}

void ImScenegraph::setupTransform(OSG::VRGuiSignals::Options o) {
    auto setVector = [&](Im_Vector& vec, string opt) {
        vector<float> v;
        toValue(o[opt], v);
        vec.set3(v[0], v[1], v[2]);
    };

    auto setVector2 = [&](Im_Vector& vec, string opt) {
        vector<float> v;
        toValue(o[opt], v);
        vec.set2(v[0], v[1]);
    };

    //useAt = toBool(o["useAt"]);
    global = !toBool(o["local"]);

    setVector(position, "pos");
    setVector(atvector, "at");
    setVector(direction, "dir");
    setVector(upvector, "up");
    setVector(scale, "scale");

    constrActive = toBool(o["constrActive"]);
    constrLocal = toBool(o["constrLocal"]);

    setVector2(constrDof1, "constrDof0");
    setVector2(constrDof2, "constrDof1");
    setVector2(constrDof3, "constrDof2");
    setVector2(constrDof4, "constrDof3");
    setVector2(constrDof5, "constrDof4");
    setVector2(constrDof6, "constrDof5");

    cout << "  ImScenegraph::setupTransform " << o["pos"] << endl;
}

void ImScenegraph::setupCamera(OSG::VRGuiSignals::Options o) {
    doAcceptRoot = toBool(o["acceptRoot"]);
    camAspect.value = o["aspect"];
    camFov.value = o["fov"];
    camNear.value = o["near"];
    camFar.value = o["far"];
    if (o["projection"] == "perspective") camProjection = 0;
    if (o["projection"] == "orthographic") camProjection = 1;
}

void ImScenegraph::setupLight(OSG::VRGuiSignals::Options o) {
    string lType;
    string sRes;
    toValue(o["type"], lType);
    toValue(o["shadowRes"], sRes);
    for (int i=0; i<lightTypes.size(); i++) if (string(lightTypes[i]) == lType) lightType = i;
    for (int i=0; i<shadowResolutions.size(); i++) if (string(shadowResolutions[i]) == sRes) shadowResolution = i;
    lightOn = toBool(o["state"]);
    shadowsOn = toBool(o["doShadows"]);
}

void ImScenegraph::setupLod(OSG::VRGuiSignals::Options o) {
    auto setVector = [&](Im_Vector& vec, string opt) {
        vector<float> v;
        toValue(o[opt], v);
        vec.set3(v[0], v[1], v[2]);
    };

    setVector(lodCenter, "center");
    toValue(o["distances"], lodDistances);
}

void ImScenegraph::setupGeometry(OSG::VRGuiSignals::Options o) {
    geoOrigin = o["origin"];
    geoParams = splitString(o["originParams"]);
    toValue(o["paramNames"], geoParamNames);

    geoData.clear();
    vector<string> tmp = splitString(o["geoData"], '$');
    for (auto&t : tmp) {
        auto v = splitString(t, '|');
        if (v.size() != 2) continue;
        geoData.push_back(make_pair(v[0], toInt(v[1])));
    }
}

void ImScenegraph::setupMaterial(OSG::VRGuiSignals::Options o) {
    if (!o.count("name")) matName = "";
    else {
        matName = o["name"];
        matAmbient.set(o["ambient"]);
        matDiffuse.set(o["diffuse"]);
        matSpecular.set(o["specular"]);
        matEmission.set(o["emission"]);
        matLit = toBool(o["isLit"]);
        matMeshColors = !toBool(o["ignoreMeshCols"]);

        if (o.count("texDims")) {
            texDims = o["texDims"];
            texSize = o["texSize"];
            texChannels = o["texChannels"];
        } else texDims = "";
    }
}

void ImScenegraph::treeClear() { tree.clear(); }

void ImScenegraph::treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col) {
    auto node = tree.add(ID, label, IM_TV_NODE_EDITABLE, parent);
    vector<pair<string, string>> menu;
    menu.push_back( make_pair("delete", "sg_menu_delete") );
    menu.push_back( make_pair("new Object", "sg_menu_newObject") );
    menu.push_back( make_pair("new Transform", "sg_menu_newTransform") );
    menu.push_back( make_pair("new Camera", "sg_menu_newCamera") );
    menu.push_back( make_pair("new Light", "sg_menu_newLight") );
    menu.push_back( make_pair("new Geometry", "sg_menu_newGeometry") );
    node->setMenu(menu);
}
