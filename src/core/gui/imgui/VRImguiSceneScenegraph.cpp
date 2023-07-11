#include "VRImguiSceneScenegraph.h"
#include "imWidgets/VRImguiInput.h"

#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

using namespace OSG;

ImScenegraph::ImScenegraph() : tree("scenegraph") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("set_sg_title", [&](OSG::VRGuiSignals::Options o){ title = o["title"]; return true; } );
    mgr->addCallback("on_sg_tree_clear", [&](OSG::VRGuiSignals::Options o){ treeClear(); return true; } );
    mgr->addCallback("on_sg_tree_append", [&](OSG::VRGuiSignals::Options o) {
            treeAppend(o["ID"], o["label"], o["parent"], o["type"], o["cla"], o["mod"], o["col"]);
        return true; } );
    mgr->addCallback("on_sg_setup_obj", [&](OSG::VRGuiSignals::Options o){ setupObject(o); return true; } );
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
        ImGui::Text(("Object: " + selected).c_str());
        ImGui::Text(("Parent: " + parent).c_str());
        ImGui::Text(("Persistency: " + persistency).c_str());
        if (ImGui::Checkbox("visible:", &visible)) uiSignal( "sg_toggle_visible", {{"visible",toString(visible)}} );
        if (ImGui::Checkbox("pickable:", &pickable)) uiSignal( "sg_toggle_pickable", {{"pickable",toString(pickable)}} );
        if (ImGui::Checkbox("castShadow:", &castShadow)) uiSignal( "sg_toggle_cast_shadow", {{"castShadow",toString(castShadow)}} );
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

void ImScenegraph::treeClear() { tree.clear(); }

void ImScenegraph::treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col) {
    //cout << "treeAppend, ID: " << ID << ", parent: " << parent << endl;
    //tree.add([parent].push_back({label, type, cla, mod, col});
    tree.add(ID, label, IM_TV_NODE_EDITABLE, parent);
}
