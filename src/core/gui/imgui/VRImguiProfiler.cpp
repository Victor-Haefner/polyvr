#include "VRImguiProfiler.h"


#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

ImProfiler::ImProfiler() : ImWidget("Profiler") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("set_profiler_system", [&](OSG::VRGuiSignals::Options o){ updateSystem(o); return true; }, true );
    mgr->addCallback("set_profiler_scene", [&](OSG::VRGuiSignals::Options o){ updateScene(o); return true; }, true );
    mgr->addCallback("set_profiler_performance", [&](OSG::VRGuiSignals::Options o){ updatePerformance(o); return true; }, true );
}

void ImProfiler::updateSystem(OSG::VRGuiSignals::Options& o) {
    vendor = o["vendor"];
    version = o["version"];
    glsl = o["glsl"];
    hasGeomShader = o["hasGeomShader"];
    hasTessShader = o["hasTessShader"];
}

void ImProfiler::updateScene(OSG::VRGuiSignals::Options& o) {
    Nnodes = o["Nobjects"];
    Ntransformations = o["Ntransforms"];
    Ngeometries = o["Ngeometries"];
}

void ImProfiler::updatePerformance(OSG::VRGuiSignals::Options& o) {}

void ImProfiler::begin() {
    auto setCurrentTab = [&](string t) {
        if (t != currentTab) {
            currentTab = t;
            uiSignal("ui_change_profiler_tab", {{"tab",currentTab}});
        }
    };

    ImGuiTabItemFlags flags1 = (selected == "System") ? ImGuiTabItemFlags_SetSelected : 0;
    ImGuiTabItemFlags flags2 = (selected == "Scene") ? ImGuiTabItemFlags_SetSelected : 0;
    ImGuiTabItemFlags flags3 = (selected == "Performance") ? ImGuiTabItemFlags_SetSelected : 0;
    selected = "";

    if (ImGui::BeginTabBar("ProfilerTabBar", ImGuiTabBarFlags_None)) {
        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        if (ImGui::BeginTabItem("System", NULL, flags1)) {
            ImGui::Spacing();
            ImGui::BeginChild("ProfSystem", ImGui::GetContentRegionAvail(), false, flags);
            renderTabSystem();
            ImGui::EndChild();
            ImGui::EndTabItem();
            setCurrentTab("Rendering");
        }

        if (ImGui::BeginTabItem("Scene", NULL, flags2)) {
            ImGui::Spacing();
            ImGui::BeginChild("ProfScene", ImGui::GetContentRegionAvail(), false, flags);
            renderTabScene();
            ImGui::EndChild();
            ImGui::EndTabItem();
            setCurrentTab("Rendering");
        }

        if (ImGui::BeginTabItem("Performance", NULL, flags3)) {
            ImGui::Spacing();
            ImGui::BeginChild("ProfPerformance", ImGui::GetContentRegionAvail(), false, flags);
            renderTabPerformance();
            ImGui::EndChild();
            ImGui::EndTabItem();
            setCurrentTab("Scenegraph");
        }

        ImGui::EndTabBar();
    }
}

void ImProfiler::renderTabSystem() {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Text("System:");
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 50*io.FontGlobalScale);
    if (ImGui::Button("update")) uiSignal("profiler_update_system");

    ImGui::Indent(10);
        ImGui::Text(("Vendor: " + vendor).c_str());
        ImGui::Text(("Version: " + version).c_str());
        ImGui::Text(("GLSL: " + glsl).c_str());
        ImGui::Text(("Has geom. shader: " + hasGeomShader).c_str());
        ImGui::Text(("Has tess. shader: " + hasTessShader).c_str());
    ImGui::Unindent(10);
}

void ImProfiler::renderTabScene() {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Text("Nodes:");
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 50*io.FontGlobalScale);
    if (ImGui::Button("update")) uiSignal("profiler_update_scene");

    ImGui::Indent(10);
        ImGui::Text(("Total nodes: " + Nnodes).c_str());
        ImGui::Text(("Total transformations: " + Ntransformations).c_str());
        ImGui::Text(("Total geometries: " + Ngeometries).c_str());
    ImGui::Unindent(10);
}

void ImProfiler::renderTabPerformance() {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Text("Frames:");
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 50*io.FontGlobalScale);
    if (ImGui::Button("update")) uiSignal("profiler_update_performance");
}


