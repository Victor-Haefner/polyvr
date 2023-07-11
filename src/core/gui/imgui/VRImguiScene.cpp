#include "VRImguiScene.h"


#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

ImSceneEditor::ImSceneEditor() : ImWidget("SceneEditor") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("openUiTabs", [&](OSG::VRGuiSignals::Options o){ selected = o["tab2"]; return true; } );
}

void ImSceneEditor::begin() {
    auto setCurrentTab = [&](string t) {
        if (t != currentTab) {
            currentTab = t;
            uiSignal("ui_change_scene_tab", {{"tab",currentTab}});
            //cout << "  setCurrentTab " << currentTab << " -> " << t << endl;
        }
    };

    ImGuiTabItemFlags flags1 = (selected == "Rendering") ? ImGuiTabItemFlags_SetSelected : 0;
    ImGuiTabItemFlags flags2 = (selected == "Scenegraph") ? ImGuiTabItemFlags_SetSelected : 0;
    ImGuiTabItemFlags flags3 = (selected == "Scripting") ? ImGuiTabItemFlags_SetSelected : 0;
    ImGuiTabItemFlags flags4 = (selected == "Navigation") ? ImGuiTabItemFlags_SetSelected : 0;
    ImGuiTabItemFlags flags5 = (selected == "Semantics") ? ImGuiTabItemFlags_SetSelected : 0;
    ImGuiTabItemFlags flags6 = (selected == "Network") ? ImGuiTabItemFlags_SetSelected : 0;
    selected = "";

    if (ImGui::BeginTabBar("AppPanelsTabBar", ImGuiTabBarFlags_None)) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        if (ImGui::BeginTabItem("Rendering", NULL, flags1)) {
            ImGui::Spacing();
            ImGui::BeginChild("RenderingPanel", ImGui::GetContentRegionAvail(), false, flags);
            rendering.render();
            ImGui::EndChild();
            ImGui::EndTabItem();
            setCurrentTab("Rendering");
        }

        if (ImGui::BeginTabItem("Scenegraph", NULL, flags2)) {
            ImGui::Spacing();
            ImGui::BeginChild("ScenegraphPanel", ImGui::GetContentRegionAvail(), false, flags);
            scenegraph.render();
            ImGui::EndChild();
            ImGui::EndTabItem();
            setCurrentTab("Scenegraph");
        }

        if (ImGui::BeginTabItem("Scripting", NULL, flags3)) {
            scripting.render();
            ImGui::EndTabItem();
            setCurrentTab("Scripting");
        }

        if (ImGui::BeginTabItem("Navigation", NULL, flags4)) {
            ImGui::Spacing();
            ImGui::BeginChild("NavigationPanel", ImGui::GetContentRegionAvail(), false, flags);
            navigation.render();
            ImGui::EndChild();
            ImGui::EndTabItem();
            setCurrentTab("Navigation");
        }

        if (ImGui::BeginTabItem("Semantics", NULL, flags5)) {
            ImGui::Spacing();
            ImGui::BeginChild("SemanticsPanel", ImGui::GetContentRegionAvail(), false, flags);
            semantics.render();
            ImGui::EndChild();
            ImGui::EndTabItem();
            setCurrentTab("Semantics");
        }

        if (ImGui::BeginTabItem("Network", NULL, flags6)) {
            ImGui::Spacing();
            ImGui::BeginChild("NetworkPanel", ImGui::GetContentRegionAvail(), false, flags);
            network.render();
            ImGui::EndChild();
            ImGui::EndTabItem();
            setCurrentTab("Network");
        }

        ImGui::EndTabBar();
    }
}
