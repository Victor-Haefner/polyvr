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
                                camFar("sgCamFar", "far", "512") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("set_sg_title", [&](OSG::VRGuiSignals::Options o){ title = o["title"]; return true; } );
    mgr->addCallback("on_sg_tree_clear", [&](OSG::VRGuiSignals::Options o){ treeClear(); return true; } );
    mgr->addCallback("on_sg_tree_append", [&](OSG::VRGuiSignals::Options o) {
            treeAppend(o["ID"], o["label"], o["parent"], o["type"], o["cla"], o["mod"], o["col"]);
        return true; } );
    mgr->addCallback("on_sg_setup_obj", [&](OSG::VRGuiSignals::Options o){ setupObject(o); return true; } );
    mgr->addCallback("on_sg_setup_trans", [&](OSG::VRGuiSignals::Options o){ setupTransform(o); return true; } );
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

        // transform
        ImGui::Separator();
        ImGui::Text("Transformation:");
        ImGui::Indent(10);
        if (ImGui::Button("Focus")) uiSignal( "sg_focus_transform");
        ImGui::SameLine();
        if (ImGui::Button("Identity")) uiSignal( "sg_set_identity");

        position.render(region2.x-10);
        atvector.render(region2.x-10);
        direction.render(region2.x-10);
        upvector.render(region2.x-10);
        scale.render(region2.x-10);

        if (ImGui::Checkbox("global", &global)) uiSignal( "sg_toggle_global", {{"global",toString(global)}} );
        ImGui::Unindent(10);

        ImGui::Text("Constraints:");
        ImGui::Indent(10);
        if (ImGui::Checkbox("translation:", &doConstrTranslation)) ;//uiSignal( "sg_toggle_global", {{"global",toString(global)}} );
        if (doConstrTranslation) {
            ImGui::Indent(10);
            constrTranslation.render(region2.x-20);
            if (ImGui::RadioButton("point", &cTransMode, 0)) ;
            ImGui::SameLine();
            if (ImGui::RadioButton("line", &cTransMode, 1)) ;
            ImGui::SameLine();
            if (ImGui::RadioButton("plane", &cTransMode, 2)) ;
            ImGui::Unindent(10);
        }
        if (ImGui::Checkbox("rotation:", &doConstrRotation)) ;//uiSignal( "sg_toggle_global", {{"global",toString(global)}} );
        if (doConstrRotation) {
            ImGui::Indent(10);
            if (ImGui::Checkbox("x", &cRotX)) ;
            ImGui::SameLine();
            if (ImGui::Checkbox("y", &cRotY)) ;
            ImGui::SameLine();
            if (ImGui::Checkbox("z", &cRotZ)) ;
            ImGui::Unindent(10);
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

        // geometry
        ImGui::Separator();
        ImGui::Text("Geometry:");
        ImGui::Indent(10);
        ImGui::Unindent(10);

        // material
        ImGui::Separator();
        ImGui::Text("Material:");
        ImGui::Indent(10);
        ImGui::Unindent(10);

        // camera
        ImGui::Separator();
        ImGui::Text("Camera:");
        ImGui::Indent(10);
        if (ImGui::Checkbox("accept setup root:", &doAcceptRoot)) ;
        camAspect.render(50);
        ImGui::SameLine();
        camFov.render(50);
        camNear.render(50);
        ImGui::SameLine();
        camFar.render(50);
        ImGui::Unindent(10);

        // light
        ImGui::Separator();
        ImGui::Text("Light:");
        ImGui::Indent(10);
        ImGui::Unindent(10);

        // lod
        ImGui::Separator();
        ImGui::Text("LoD:");
        ImGui::Indent(10);
        ImGui::Unindent(10);

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

    //useAt = toBool(o["useAt"]);
    global = !toBool(o["local"]);

    setVector(position, "pos");
    setVector(atvector, "at");
    setVector(direction, "dir");
    setVector(upvector, "up");
    setVector(scale, "scale");

    cout << "  ImScenegraph::setupTransform " << o["pos"] << endl;
}

void ImScenegraph::treeClear() { tree.clear(); }

void ImScenegraph::treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col) {
    //cout << "treeAppend, ID: " << ID << ", parent: " << parent << endl;
    //tree.add([parent].push_back({label, type, cla, mod, col});
    tree.add(ID, label, IM_TV_NODE_EDITABLE, parent);
}
