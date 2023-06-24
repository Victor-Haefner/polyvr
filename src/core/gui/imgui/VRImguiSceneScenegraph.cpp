#include "VRImguiSceneScenegraph.h"
#include "imWidgets/VRImguiInput.h"

#include "core/gui/VRGuiManager.h"

using namespace OSG;

ImScenegraph::ImScenegraph() : tree("scenegraph") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("on_change_sg_selection", [&](OSG::VRGuiSignals::Options o){ selected = o["selected"]; return true; } );
    mgr->addCallback("set_sg_title", [&](OSG::VRGuiSignals::Options o){ title = o["title"]; return true; } );
    mgr->addCallback("on_sg_tree_clear", [&](OSG::VRGuiSignals::Options o){ treeClear(); return true; } );
    mgr->addCallback("on_sg_tree_append", [&](OSG::VRGuiSignals::Options o) {
            treeAppend(o["ID"], o["label"], o["parent"], o["type"], o["cla"], o["mod"], o["col"]);
        return true; } );
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
    ImGui::Text("props");
    ImGui::EndChild();
}

void ImScenegraph::treeClear() { tree.clear(); }

void ImScenegraph::treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col) {
    //cout << "treeAppend, ID: " << ID << ", parent: " << parent << endl;
    //tree.add([parent].push_back({label, type, cla, mod, col});
    tree.add(ID, label, 0, parent);
}
