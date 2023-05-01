#include "VRImguiConsoles.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

ImConsole::ImConsole(string ID) : ID(ID), name(ID) {}

void ImConsole::render() {
    if (!sensitive) ImGui::BeginDisabled();

    if (ImGui::BeginTabItem(name.c_str())) {
        ImGui::Text(data.c_str());
        ImGui::EndTabItem();
    }

    if (!sensitive) ImGui::EndDisabled();
}

ImConsoles::ImConsoles() : ImWidget("Consoles") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("newConsole", [&](OSG::VRGuiSignals::Options o){ newConsole(o["ID"]); return true; } );
    mgr->addCallback("setupConsole", [&](OSG::VRGuiSignals::Options o){ setupConsole(o["ID"], o["name"]); return true; } );
    mgr->addCallback("pushConsole", [&](OSG::VRGuiSignals::Options o){ pushConsole(o["ID"], o["string"]); return true; } );
    mgr->addCallback("clearConsole", [&](OSG::VRGuiSignals::Options o){ clearConsole(o["ID"]); return true; } );
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

void ImConsoles::pushConsole(string ID, string data) {
    cout << " - - - - - - - ImConsoles::pushConsole " << ID << "  " << data << endl;
    if (!consoles.count(ID)) return;
    consoles[ID].data += data;
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
