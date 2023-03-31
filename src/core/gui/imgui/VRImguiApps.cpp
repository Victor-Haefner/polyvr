#include "VRImguiApps.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

ImAppLauncher::ImAppLauncher(string ID) : ID(ID), name(ID) {}

void ImAppLauncher::render() {
    if (!sensitive) ImGui::BeginDisabled();

    ImGui::BeginGroup();
    ImGui::Spacing();
    ImGui::Indent(5.0);
    if (!running) {
        if (ImGui::Button(("Run##"+ID).c_str())) uiSignal("on_toggle_app", {{"ID",ID}});
    } else {
        if (ImGui::Button(("Stop##"+ID).c_str())) uiSignal("on_toggle_app", {{"ID",ID}});
    }
    ImGui::SameLine();
    string label = name;
    if (label.length() > 25) label = ".." + subString(label, label.length()-23, 23);
    ImGui::Text(label.c_str());
    ImGui::SameLine();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
    if (ImGui::CollapsingHeader(("advanced##"+ID).c_str(), flags) && sensitive) {
        if (ImGui::Button(("Run without scripts##"+ID).c_str())) uiSignal("on_toggle_app_no_scripts", {{"ID",ID}});
    }
    ImGui::Spacing();
    ImGui::EndGroup();

    ImVec2 p1 = ImGui::GetItemRectMin();
    ImVec2 p2 = ImGui::GetItemRectMax();
    p2.x = ImGui::GetContentRegionAvail().x+8;
    ImGui::GetWindowDrawList()->AddRect(p1, p2, IM_COL32(255, 255, 255, 255));

    if (!sensitive) ImGui::EndDisabled();
}

void ImAppPanel::render() {
    for (auto& l : launchers) l.second.render();
}

ImAppManager::ImAppManager() : ImWidget("AppManager") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("newAppLauncher", [&](OSG::VRGuiSignals::Options o){ newAppLauncher(o["panel"], o["ID"]); return true; } );
    mgr->addCallback("setupAppLauncher", [&](OSG::VRGuiSignals::Options o){ setupAppLauncher(o["ID"], o["name"]); return true; } );
    mgr->addCallback("setAppLauncherState", [&](OSG::VRGuiSignals::Options o){ setAppLauncherState(o["ID"], toBool(o["running"]), toBool(o["sensitive"])); return true; } );
}

void ImAppManager::newAppLauncher(string panel, string ID) {
    if (panel == "examples") examples.launchers[ID] = ImAppLauncher(ID);
    if (panel == "favorites") projects.launchers[ID] = ImAppLauncher(ID);
    if (panel == "recents") recents.launchers[ID] = ImAppLauncher(ID);
}

void ImAppManager::setAppLauncherState(string ID, bool running, bool sensitive) {
    for (auto& p : {&examples, &projects, &recents}) {
        if (!p->launchers.count(ID)) continue;
        p->launchers[ID].running = running;
        p->launchers[ID].sensitive = sensitive;
    }
}

void ImAppManager::setupAppLauncher(string ID, string name) {
    for (auto& p : {&examples, &projects, &recents}) {
        if (!p->launchers.count(ID)) continue;
        p->launchers[ID].name = name;
    }
}

void ImAppManager::begin() {
    if (ImGui::BeginTabBar("AppPanelsTabBar", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Projects")) {
            ImGui::Spacing();
            ImGuiWindowFlags flags = ImGuiWindowFlags_None;
            ImGui::BeginChild("Panel1", ImGui::GetContentRegionAvail(), false, flags);
            recents.render();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            projects.render();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Examples")) {
            ImGui::Spacing();
            ImGuiWindowFlags flags = ImGuiWindowFlags_None;
            ImGui::BeginChild("Panel2", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false, flags);
            examples.render();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
