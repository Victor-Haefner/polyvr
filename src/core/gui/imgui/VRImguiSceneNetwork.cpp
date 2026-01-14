#include "VRImguiSceneNetwork.h"
#include "core/gui/VRGuiManager.h"

#include "core/utils/toString.h"

using namespace std;

ImNetwork::ImNetwork() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("on_net_ui_clear", [&](OSG::VRGuiSignals::Options o) { clear(); return true; } );
    mgr->addCallback("on_new_net_node", [&](OSG::VRGuiSignals::Options o) { addNode(o["ID"], o["name"]); return true; } );
    mgr->addCallback("on_new_data_flow", [&](OSG::VRGuiSignals::Options o) { addFlow(o["ID"], toInt(o["width"]), toInt(o["height"])); return true; } );
    mgr->addCallback("on_data_flow_resize", [&](OSG::VRGuiSignals::Options o) { resizeFlow(o["ID"], toInt(o["width"]), toInt(o["height"])); return true; } );
    mgr->addCallback("on_data_flow_changed", [&](OSG::VRGuiSignals::Options o) { setCurve(o["ID"], o["curve"]); return true; } );
    mgr->addCallback("canvas_widget_set_visible", [&](OSG::VRGuiSignals::Options o) { ; return true; } ); // TODO
    mgr->addCallback("canvas_widget_move", [&](OSG::VRGuiSignals::Options o) { placeWidget(o["ID"], toFloat(o["x"]), toFloat(o["y"])); return true; } );
}

#if (IMGUI_VERSION_NUM > 19000)
#define ImDrawCornerFlags_All ImDrawFlags_RoundCornersAll
#endif

void ImNetwork::render() {
    // toolbar
    ImGui::Spacing();
    if (ImGui::Button("Update")) uiSignal("ui_network_update");
    ImGui::Spacing();

    // canvas
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetContentRegionAvail().y;

    ImGui::BeginGroup();
    ImGui::BeginChild("ScriptListPanel", ImVec2(w, h), true, ImGuiWindowFlags_None);

    ImGui::Indent(5);
        ImVec2 p0 = ImGui::GetCursorScreenPos();

        for (auto& n : nodes) {
            ImVec2 p = p0;
            p.x += n.second.x;
            p.y += n.second.y;
            ImGui::SetCursorScreenPos(p);
            ImGui::TextUnformatted(n.second.name.c_str());
        }

        const ImU32 col32 = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        for (auto& n : flows) {
            ImVec2 p1 = p0;
            p1.x += n.second.x;
            p1.y += n.second.y;
            ImVec2 p2 = p1;
            p2.x += n.second.w;
            p2.y += n.second.h;
            draw_list->AddRect(p1, p2, col32, 0.0f, ImDrawCornerFlags_All, 1);

            for (int i=0; i<n.second.curve.size(); i++) {
                int h = n.second.curve[i] * n.second.h;
                p1.x += 1;
                p2.x = p1.x+1;
                p1.y = p2.y - h;
                draw_list->AddLine(p1, p2, col32);
                //draw_list->AddRectFilled(p1, p2, col32, 0.0f, ImDrawCornerFlags_All);
            }
        }
    ImGui::Unindent(5);

    ImGui::EndChild();
    ImGui::EndGroup();
}

void ImNetwork::placeWidget(string ID, float x, float y) {
    if (nodes.count(ID)) {
        nodes[ID].x = x;
        nodes[ID].y = y;
    }

    if (flows.count(ID)) {
        flows[ID].x = x;
        flows[ID].y = y;
    }
}

void ImNetwork::clear() {
    flows.clear();
    nodes.clear();
}

void ImNetwork::addFlow(string ID, int w, int h) {
    flows[ID] = {w,h};
}

void ImNetwork::addNode(string ID, string name) {
    nodes[ID] = {name};
}

void ImNetwork::resizeFlow(string ID, int w, int h) {
    flows[ID].w = w;
    flows[ID].h = h;
}

void ImNetwork::setCurve(string ID, string curveData) {
    flows[ID].curve.clear();
    toValue(curveData, flows[ID].curve);
}
