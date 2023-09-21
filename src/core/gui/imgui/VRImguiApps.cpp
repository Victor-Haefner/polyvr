#include "VRImguiApps.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

ImAppLauncher::ImAppLauncher(string ID, string pnl) : ID(ID), name(ID), panel(pnl) {}

void ImAppLauncher::render(string filter) {
    if (filter != "") {
        if (!contains(name, filter, false)) return;
    }

    string label = name;
    if (label.length() > 25) label = ".." + subString(label, label.length()-23, 23);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;

    if (!sensitive) ImGui::BeginDisabled();

    ImGui::BeginGroup();
        ImGui::Spacing();
        ImGui::Indent(5.0);
        ImGui::Columns(2);
            if (ImGui::CollapsingHeader(("advanced##"+ID).c_str(), flags) && sensitive) {
                if (ImGui::Button(("Run without scripts##"+ID).c_str())) uiSignal("on_toggle_app_no_scripts", {{"ID",ID}});
            }
            ImGui::NextColumn();
            ImGui::Text(label.c_str());
            ImGui::SameLine();
            if (!running) {
                if (ImGui::Button(("Run##"+ID).c_str())) uiSignal("on_toggle_app", {{"ID",ID}});
            } else {
                if (ImGui::Button(("Stop##"+ID).c_str())) uiSignal("on_toggle_app", {{"ID",ID}});
            }
        ImGui::Columns(1);
        ImGui::Spacing();
    ImGui::EndGroup();

    ImVec2 p1 = ImGui::GetItemRectMin();
    ImVec2 p2 = ImGui::GetItemRectMax();
    p2.x = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddRect(p1, p2, IM_COL32(255, 255, 255, 255));

    if (!sensitive) ImGui::EndDisabled();
}

ImAppPanel::ImAppPanel(string lbl) : label(lbl) {
    ;
}

void ImAppPanel::render(string filter, map<string, ImAppLauncher>& launcherPool) {
    if (filter != "") {
        bool anyVisible = false;
        for (auto& lID : launchers) {
            auto& l = launcherPool[lID];
            if (contains(l.name, filter, false)) anyVisible = true;
        }
        if (!anyVisible) return;
    }

    if (label.size() > 0) {
        ImGui::Spacing();
        ImGui::Text(label.c_str());
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::Spacing();
    }

    for (auto& lID : launchers) {
        auto& l = launcherPool[lID];
        l.render(filter);
    }
}

ImAppManager::ImAppManager() : ImWidget("AppManager"), examples("") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("newAppLauncher", [&](OSG::VRGuiSignals::Options o){ newAppLauncher(o["panel"], o["ID"]); return true; } );
    mgr->addCallback("setupAppLauncher", [&](OSG::VRGuiSignals::Options o){ setupAppLauncher(o["ID"], o["name"]); return true; } );
    mgr->addCallback("setAppLauncherState", [&](OSG::VRGuiSignals::Options o){ setAppLauncherState(o["ID"], toBool(o["running"]), toBool(o["sensitive"])); return true; } );
}

void ImAppManager::updatePannels() {
    projects.clear();
    projects.push_back(ImAppPanel("recent"));
    projects.push_back(ImAppPanel("older"));

    ImAppPanel& recents = projects[0];
    ImAppPanel& older = projects[1];

    for (auto& l : launchers) {
        if (l.second.panel == "recents") {
            recents.launchers.push_back(l.first);
        } else {
            older.launchers.push_back(l.first);
        }
    }
}

void ImAppManager::newAppLauncher(string panel, string ID) {
    launchers[ID] = ImAppLauncher(ID, panel);
    if (panel == "examples") examples.launchers.push_back(ID);
    else updatePannels();
}

void ImAppManager::setAppLauncherState(string ID, bool running, bool sensitive) {
    if (!launchers.count(ID)) return;
    auto& l = launchers[ID];
    l.running = running;
    l.sensitive = sensitive;
}

void ImAppManager::setupAppLauncher(string ID, string name) {
    if (!launchers.count(ID)) return;
    auto& l = launchers[ID];
    l.name = name;
}

void ImAppManager::begin() {
    static char str0[128] = "";
    memcpy(str0, filter.c_str(), filter.size());
    str0[filter.size()] = 0;

    ImGui::Text("filter:");
    ImGui::SameLine();
    if (ImGui::InputText("##AppFilter", str0, 128) ) {
        filter = string(str0);
        uiSignal("on_change_app_filter", {{"filter",filter}});
    }

    if (ImGui::BeginTabBar("AppPanelsTabBar", ImGuiTabBarFlags_None)) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        if (ImGui::BeginTabItem("Projects")) {
            ImGui::Spacing();
            ImGui::BeginChild("Panel1", ImGui::GetContentRegionAvail(), false, flags);
            for (auto panel : projects) panel.render(filter, launchers);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Examples")) {
            ImGui::Spacing();
            ImGui::BeginChild("Panel2", ImGui::GetContentRegionAvail(), false, flags);
            examples.render(filter, launchers);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
