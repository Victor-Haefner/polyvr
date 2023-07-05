#include "VRImguiConsoles.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include <imgui_internal.h>

ImConsole::ImConsole(string ID) : ID(ID), name(ID) {
    color = ImGui::GetColorU32(ImVec4(255,255,255,255));
}

void ImConsole::render() {
    if (!sensitive) ImGui::BeginDisabled();

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    bool tabOpen = ImGui::BeginTabItem(name.c_str());
    ImGui::PopStyleColor();

    if (tabOpen) {
        color = ImGui::GetColorU32(ImVec4(255,255,255,255)); // reset color
        auto r = ImGui::GetContentRegionAvail();
        string wID = "##"+name+"_text";

        if (changed > 0) {
            size_t N = countLines(data);
            ImGui::SetNextWindowScroll(ImVec2(-1, N * ImGui::GetTextLineHeight()));
            changed -= 1; // for some reason needs two passes
        }

		ImGui::InputTextMultiline(wID.c_str(), &data[0], data.size(), r, ImGuiInputTextFlags_ReadOnly);
        /*bool hovered = ImGui::IsItemHovered();
        if (hovered) {
            cout << "hovered" << endl;
        }*/

        ImGui::EndTabItem();
    }

    if (!sensitive) ImGui::EndDisabled();
}

ImConsoles::ImConsoles() : ImWidget("Consoles") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("newConsole", [&](OSG::VRGuiSignals::Options o){ newConsole(o["ID"]); return true; } );
    mgr->addCallback("setupConsole", [&](OSG::VRGuiSignals::Options o){ setupConsole(o["ID"], o["name"]); return true; } );
    mgr->addCallback("pushConsole", [&](OSG::VRGuiSignals::Options o){ pushConsole(o["ID"], o["string"], o["style"], o["mark"]); return true; } );
    mgr->addCallback("clearConsole", [&](OSG::VRGuiSignals::Options o){ clearConsole(o["ID"]); return true; } );
    mgr->addCallback("clearConsoles", [&](OSG::VRGuiSignals::Options o){ for (auto& c : consoles) c.second.data = ""; return true; } );
    mgr->addCallback("setConsoleLabelColor", [&](OSG::VRGuiSignals::Options o){ setConsoleLabelColor(o["ID"], o["color"]); return true; } );
}

ImVec4 colorFromString(const string& c) {
    auto conv = [](char c1, char c2) {
        if (c1 >= 'A' && c1 <= 'F') c1 -= ('A'-'a');
        if (c2 >= 'A' && c2 <= 'F') c2 -= ('A'-'a');
        int C1 = c1-'0';
        int C2 = c2-'0';
        if (c1 >= 'a' && c1 <= 'f') C1 = (c1-'a')+10;
        if (c2 >= 'a' && c2 <= 'f') C2 = (c2-'a')+10;
        return (C1*16+C2)/256.0;
    };

    if (c[0] == '#') {
        if (c.size() == 4) return ImVec4(conv(c[1],'f'), conv(c[2],'f'), conv(c[3],'f'), 255);
        if (c.size() == 5) return ImVec4(conv(c[1],'f'), conv(c[2],'f'), conv(c[3],'f'), conv(c[4],'f'));
        if (c.size() == 7) return ImVec4(conv(c[1],c[2]), conv(c[3],c[4]), conv(c[5],c[6]), 255);
        if (c.size() == 9) return ImVec4(conv(c[1],c[2]), conv(c[3],c[4]), conv(c[5],c[6]), conv(c[7],c[8]));
    }
    return ImVec4(255,255,255,255);
}

void ImConsoles::setConsoleLabelColor(string ID, string color) {
    auto c = colorFromString(color);
    consoles[ID].color = ImGui::GetColorU32(c);
}

void ImConsoles::newConsole(string ID) {
    consoles[ID] = ImConsole(ID);
    consolesOrder.push_back(ID);
}

void ImConsoles::clearConsole(string ID) {
    if (!consoles.count(ID)) return;
    consoles[ID].data = "";
}

void ImConsoles::setupConsole(string ID, string name) {
    if (!consoles.count(ID)) return;
    consoles[ID].name = name;
}

void ImConsoles::pushConsole(string ID, string data, string style, string mark) {
    //cout << " - - - - - - - ImConsoles::pushConsole " << ID << "  " << data << "  " << style << "  " << mark << endl;
    if (!consoles.count(ID)) return;
    consoles[ID].data += data;
    consoles[ID].changed = 2;
}

ImViewControls::ImViewControls() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("ui_clear_navigations", [&](OSG::VRGuiSignals::Options o){ navigations.clear(); return true; } );
    mgr->addCallback("ui_add_navigation", [&](OSG::VRGuiSignals::Options o){ navigations[o["nav"]] = toBool(o["active"]); return true; } );

    mgr->addCallback("ui_clear_cameras", [&](OSG::VRGuiSignals::Options o){ cameras.clear(); return true; } );
    mgr->addCallback("ui_add_camera", [&](OSG::VRGuiSignals::Options o){ cameras.push_back(o["cam"]); return true; } );
    mgr->addCallback("ui_set_active_camera", [&](OSG::VRGuiSignals::Options o){ current_camera = toInt(o["camIndex"]); return true; } );
}

void ImViewControls::render() {
    const char* tmpCameras[cameras.size()];
    for (int i=0; i<cameras.size(); i++) tmpCameras[i] = cameras[i].c_str();
    ImGui::SameLine();
    ImGui::Text("Camera:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("##Cameras", &current_camera, tmpCameras, IM_ARRAYSIZE(tmpCameras))) {
        uiSignal("view_switch_camera", {{"cam",cameras[current_camera]}});
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::BeginCombo("##Navigations", "Navigations", 0)) {
        for (auto& n : navigations) {
            if (ImGui::Checkbox(n.first.c_str(), &n.second)) uiSignal("view_toggle_navigation", {{"nav",n.first}, {"state",toString(n.second)}});
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::BeginCombo("##Layers", "Layers", 0)) {
        if (ImGui::Checkbox("Cameras", &showCams)) uiSignal("view_toggle_layer", {{"layer","Cameras"},{"state",toString(showCams)}});
        if (ImGui::Checkbox("Lights", &showLights)) uiSignal("view_toggle_layer", {{"layer","Lights"},{"state",toString(showLights)}});
        if (ImGui::Checkbox("Pause Window", &pauseRendering)) uiSignal("view_toggle_layer", {{"layer","Pause rendering"},{"state",toString(pauseRendering)}});
        if (ImGui::Checkbox("Physics", &showPhysics)) uiSignal("view_toggle_layer", {{"layer","Physics"},{"state",toString(showPhysics)}});
        if (ImGui::Checkbox("Objects", &showCoordinates)) uiSignal("view_toggle_layer", {{"layer","Referentials"},{"state",toString(showCoordinates)}});
        if (ImGui::Checkbox("Setup", &showSetup)) uiSignal("view_toggle_layer", {{"layer","Setup"},{"state",toString(showSetup)}});
        if (ImGui::Checkbox("Statistics", &showStats)) uiSignal("view_toggle_layer", {{"layer","Statistics"},{"state",toString(showStats)}});
        if (ImGui::Checkbox("Stencil", &showStencil)) uiSignal("view_toggle_layer", {{"layer","Stencil"},{"state",toString(showStencil)}});
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Fullscreen")) uiSignal("toolbar_fullscreen");
}

void ImConsoles::begin() {
    viewControls.render();
    ImGui::Separator();
    if (ImGui::BeginTabBar("ConsolesTabBar", ImGuiTabBarFlags_None)) {
        for (auto& c : consolesOrder) consoles[c].render();
        ImGui::EndTabBar();
    }
}
