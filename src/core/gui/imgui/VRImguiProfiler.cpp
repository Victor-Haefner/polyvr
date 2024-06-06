#include "VRImguiProfiler.h"

#ifdef _WIN32
#include <imgui_internal.h>
#else
#include <imgui/imgui_internal.h>
#endif

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

ImProfiler::ImProfiler() : ImWidget("Profiler") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("set_profiler_system", [&](OSG::VRGuiSignals::Options o){ updateSystem(o); return true; }, true );
    mgr->addCallback("set_profiler_scene", [&](OSG::VRGuiSignals::Options o){ updateScene(o); return true; }, true );
    mgr->addCallback("set_profiler_performance", [&](OSG::VRGuiSignals::Options o){ updatePerformance(o); return true; }, true );
    mgr->addCallback("set_profiler_frame", [&](OSG::VRGuiSignals::Options o){ updateFrame(o); return true; }, true );
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

void ImProfiler::updatePerformance(OSG::VRGuiSignals::Options& o) {
    Nframes = toInt(o["Nframes"]);
}

void ImProfiler::updateFrame(OSG::VRGuiSignals::Options& o) {
    frameID = o["ID"];
    toValue(o["duration"], frameT);
    frameNChanges = toInt(o["Nchanges"]);
    frameNCreated = toInt(o["Ncreated"]);
    frameNThreads = toInt(o["Nthreads"]);
    frameT0 = toInt(o["t0"]);
    frameT1 = toInt(o["t1"]);
    toValue(o["calls"], frameCalls);
}

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
    if (ImGui::Button("update##profSys")) uiSignal("profiler_update_system");

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
    if (ImGui::Button("update##profScn")) uiSignal("profiler_update_scene");

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
    if (ImGui::Button("update##profPrf")) uiSignal("profiler_update_performance");

    int w = 10;
    int h = 2.5*ImGui::GetTextLineHeightWithSpacing();

    ImGui::BeginChild("ProfSystem", ImVec2(ImGui::GetContentRegionAvail().x, h), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        for (int i=0; i<Nframes; i++) {
            string si = toString(i);
            string bID = si+"##frame"+si;
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(bID.c_str())) uiSignal("profiler_update_frame", {{"frame", si}});
        }
    ImGui::EndChild();

    if (frameID == "") return;

    double T = frameT;

    ImGui::Text(("frame: "+frameID).c_str());
    ImGui::SameLine(0,20*io.FontGlobalScale);
    ImGui::Text(("duration: "+toString(T*0.001) + " ms").c_str());

    ImGui::Text(("N fields changes: "+toString(frameNChanges)).c_str());
    ImGui::SameLine(0,20*io.FontGlobalScale);
    ImGui::Text(("N fields created: "+toString(frameNCreated)).c_str());


    ImGui::BeginChild("ProfFrame", ImGui::GetContentRegionAvail(), true, ImGuiWindowFlags_None);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();

        int L = 80;
        int O = 5;
        int W = ImGui::GetContentRegionAvail().x;

        for (auto& f : frameCalls) {
            string callName = f.first;
            for (auto& c : f.second) {
                int i0 = c[0] - frameT0;
                int i1 = c[1] - frameT0;
                int t0 = double(i0)/T * double(W);
                int t1 = double(i1)/T * double(W);
                int tID = c[2];

                int o = L*0.25*(1.0 - double(i1-i0)/T);

                const ImU32 col32 = ImColor(1.0f, 1.0f, 1.0f, 1.0f);


                ImVec2 p1 = p0;
                p1.x += t0;
                p1.y += (L+O)*tID+o;
                ImVec2 p2 = p0;
                p2.x += t1;
                p2.y += (L+O)*tID+L-o;
                draw_list->AddRect(p1, p2, col32, 0.0f, ImDrawCornerFlags_All, 1);

                ImRect rect(p1, p2);
                ImGuiID id = ImGui::GetCurrentWindow()->GetID(string("frame"+callName).c_str());
                ImGui::ItemAdd(rect, id);
                if (ImGui::IsItemHovered())	ImGui::SetTooltip(callName.c_str());
            }
        }
    ImGui::EndChild();
}


