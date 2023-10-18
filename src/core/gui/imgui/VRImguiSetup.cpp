#include "VRImguiSetup.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/imgui/imWidgets/VRImguiInput.h"

ImSetupManager::ImSetupManager() : ImWidget("SetupManager") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("updateSetupsList", [&](OSG::VRGuiSignals::Options o){ updateSetupsList(o["setups"]); return true; } );
    mgr->addCallback("setCurrentSetup", [&](OSG::VRGuiSignals::Options o){ current_setup = toInt(o["setup"]); return true; } );
}

void ImSetupManager::updateSetupsList(string s) {
    toValue(s, setups);
}

void ImSetupManager::begin() {
    vector<const char*> tmpSetups(setups.size(), 0);
    for (int i=0; i<setups.size(); i++) tmpSetups[i] = setups[i].c_str();
    ImGui::Text("Setup:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("##Setups", &current_setup, &tmpSetups[0], tmpSetups.size())) {
        uiSignal("setup_switch_setup", {{"setup",setups[current_setup]}});
    }

    ImGui::SameLine(); if (ImGui::Button("New")) uiSignal("setup_new");
    ImGui::SameLine(); if (ImGui::Button("Delete")) uiSignal("setup_delete");
    ImGui::SameLine(); if (ImGui::Button("Save")) uiSignal("setup_save");
    ImGui::SameLine(); if (ImGui::Button("Save as..")) uiSignal("setup_saveas");
}
