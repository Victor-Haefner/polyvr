#include "VRImguiApps.h"
#ifdef _WIN32
#include <imgui_internal.h>
#else
#include <imgui/imgui_internal.h>
#endif

#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"
#include "core/gui/VRGuiManager.h"

#include <algorithm>

ImAppLauncher::ImAppLauncher(string ID, string pnl, string ts) : ID(ID), name(ID), panel(pnl), timestamp(ts) {}

ImAppLauncher::~ImAppLauncher() {}

void ImAppLauncher::render(string filter, ImImage& preview, int fullWidth, int colWidth1, int pathLabelN) {
    if (filter != "") {
        if (!contains(name, filter, false)) return;
    }

    auto ellipsize = [&](const string& s) {
        int N = pathLabelN;
        string lbl = s;
        if (s.length() > N) lbl = ".." + subString(lbl, lbl.length()-(N-2), (N-2));
        return lbl;
    };

    float availWidth = 0;
    bool doHover = false;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;

    if (!sensitive) ImGui::BeginDisabled();

    int col0 = colWidth1;
    int col2 = colWidth1*0.25;
    int col1 = fullWidth - col0-col2;

    ImVec2 cursorBeg = ImGui::GetCursorScreenPos();
    ImGui::BeginGroup();
        ImGui::Spacing();
        ImGui::Indent(5.0);
        ImGui::Columns(3);
            if (previewOK) {
                miniPreview.read(previewPath);
                miniPreview.render(60, 45);
                ImGui::SameLine();
            }

            ImGui::SetColumnWidth(-1, col0);
            if (ImGui::CollapsingHeader(("advanced##"+ID).c_str(), flags) && sensitive) {
                if (ImGui::Button(("Run without scripts##"+ID).c_str())) uiSignal("on_toggle_app_no_scripts", {{"ID",ID}});
            }

            ImGui::NextColumn();
            ImGui::SetColumnWidth(-1, col1);
            string label = ellipsize(name);
            ImGui::Text(label.c_str());

            ImGui::NextColumn();
            ImGui::SetColumnWidth(-1, col2);
            if (!running) {
                if (ImGui::Button(("Run##"+ID).c_str())) uiSignal("on_toggle_app", {{"ID",ID}});
            } else {
                if (ImGui::Button(("Stop##"+ID).c_str())) uiSignal("on_toggle_app", {{"ID",ID}});
            }
        ImGui::Columns(1);
        ImGui::Spacing();
    ImGui::EndGroup();
    ImVec2 cursorEnd = ImGui::GetCursorScreenPos();


    ImVec2 min_pos = ImVec2(ImMin(cursorBeg.x, cursorEnd.x), ImMin(cursorBeg.y, cursorEnd.y));
    ImVec2 max_pos = ImVec2(ImMax(cursorBeg.x+fullWidth, cursorEnd.x+fullWidth), ImMax(cursorBeg.y, cursorEnd.y));
    ImRect rect(min_pos, max_pos);

    ImGuiID id = ImGui::GetCurrentWindow()->GetID(string("appLauncher"+ID).c_str());
    ImGui::ItemAdd(rect, id);
    doHover = ImGui::IsItemHovered();

    ImVec2 p1 = ImGui::GetItemRectMin();
    ImVec2 p2 = ImGui::GetItemRectMax();
    p2.x = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddRect(p1, p2, IM_COL32(255, 255, 255, 255));

    if (!sensitive) ImGui::EndDisabled();

    if (doHover && previewOK) {
        //int h = fullWidth * 9.0/16.0;
        int h = fullWidth * 3.0/4.0;
        auto p = ImGui::GetCursorScreenPos();
        ImGui::SetNextWindowPos(ImVec2(p.x, p.y+5));
        ImGui::SetNextWindowSize(ImVec2(fullWidth,h));
        ImGui::Begin("OpenGL Texture Text", 0, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
        preview.read(previewPath);
        preview.render(fullWidth, h);
        ImGui::End();
    }
}

ImAppPanel::ImAppPanel(string lbl) : label(lbl) {}
ImAppPanel::~ImAppPanel() {}

void ImAppPanel::render(string filter, map<string, ImAppLauncher>& launcherPool, ImImage& preview) {
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
        ImGui::Spacing();
        ImGui::Text(label.c_str());
        //ImGui::SameLine();
        ImGui::Separator();
        ImGui::Spacing();
    }

    float scale = uiStrScale();
    float colWidth1 = ImGui::CalcTextSize("Run without scripts").x + 40.0f; // Add padding
    float fullWidth = ImGui::GetContentRegionAvail().x - 20.0*scale;
    float labelWidth = fullWidth - colWidth1 - 40*scale;
    float charWidth = 7.1*scale;
    int N = labelWidth / charWidth;

    for (auto& lID : launchers) {
        auto& l = launcherPool[lID];
        l.render(filter, preview, fullWidth, colWidth1, N);
    }
}

ImAppManager::ImAppManager() : ImWidget("AppManager"), examples("") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("newAppLauncher", [&](OSG::VRGuiSignals::Options o){ newAppLauncher(o["panel"], o["ID"], o["timestamp"]); return true; } );
    mgr->addCallback("setupAppLauncher", [&](OSG::VRGuiSignals::Options o){ setupAppLauncher(o["ID"], o["name"], o["previewPath"]); return true; } );
    mgr->addCallback("updateAppLauncherPixmap", [&](OSG::VRGuiSignals::Options o){ updateAppLauncherPixmap(o["ID"], o["path"]); return true; } );
    mgr->addCallback("setAppLauncherState", [&](OSG::VRGuiSignals::Options o){ setAppLauncherState(o["ID"], toBool(o["running"]), toBool(o["sensitive"])); return true; } );
}

ImAppManager::~ImAppManager() {}

void getDurationString(size_t d, string& dLabel, size_t& dLabelI) {
    if (d == 0) { dLabel = "just now"; dLabelI = 0; return; }
    if (d < 3600) { dLabel = "last hour"; dLabelI = 3600; return; }
    if (d < 3600*24) { dLabel = "today"; dLabelI = 3600*24; return; }
    if (d < 3600*24*2) { dLabel = "yesterday"; dLabelI = 3600*24*2; return; }
    if (d < 3600*24*7) { dLabel = "this week"; dLabelI = 3600*24*7; return; }
    if (d < 3600*24*30) { dLabel = "this month"; dLabelI = 3600*24*30; return; }
    if (d < 3600*24*356) { dLabel = "this year"; dLabelI = 3600*24*356; return; }
    { dLabel = "long ago"; dLabelI = 3600*24*356*2; return; }
}

void ImAppManager::updatePannels() {
    projects.clear();
    map<string, vector<string>> panelToLaunchers;
    map<size_t, string> panels;

    time_t tnow = time(0);
    auto now = localtime(&tnow);

    for (auto& l : launchers) {
        if (l.second.panel == "examples") continue;

        time_t tl = toValue<size_t>(l.second.timestamp);
        time_t delta = 0;
        delta = tnow-tl;
        string dLabel;
        size_t dLabelI;
        getDurationString(delta, dLabel, dLabelI);

        if (!panelToLaunchers.count(dLabel)) {
            panels[dLabelI] = dLabel;
            panelToLaunchers[dLabel] = vector<string>();
        }
        panelToLaunchers[dLabel].push_back(l.first);
    }

    auto sortByDate = [&](const string& a, const string& b) {
        auto& t1 = launchers[a].timestamp;
        auto& t2 = launchers[b].timestamp;
        return t1 > t2;
    };

    for (auto& pl : panelToLaunchers) {
        std::sort(pl.second.begin(), pl.second.end(), sortByDate);
    }

    for (auto& p : panels) {
        projects.push_back(ImAppPanel(p.second));
        auto& panel = projects[projects.size()-1];
        for (auto& l : panelToLaunchers[p.second]) {
            panel.launchers.push_back(l);
        }
    }
}

void ImAppManager::newAppLauncher(string panel, string ID, string timestamp) {
    launchers[ID] = ImAppLauncher(ID, panel, timestamp);
    if (panel == "examples") examples.launchers.push_back(ID);
    else updatePannels();
}

void ImAppManager::setAppLauncherState(string ID, bool running, bool sensitive) {
    if (!launchers.count(ID)) return;
    auto& l = launchers[ID];
    l.running = running;
    l.sensitive = sensitive;
}

void ImAppManager::setupAppLauncher(string ID, string name, string previewPath) {
    if (!launchers.count(ID)) return;
    auto& l = launchers[ID];
    l.name = name;
    l.previewPath = previewPath;
    l.previewOK = exists(previewPath);
}

void ImAppManager::updateAppLauncherPixmap(string ID, string path) {
    if (!launchers.count(ID)) return;
    auto& l = launchers[ID];
    l.previewPath = path;
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
            for (auto panel : projects) panel.render(filter, launchers, preview);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Examples")) {
            ImGui::Spacing();
            ImGui::BeginChild("Panel2", ImGui::GetContentRegionAvail(), false, flags);
            examples.render(filter, launchers, preview);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
