#include "VRImguiConsoles.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include <imgui_internal.h>

ImConsole::ImConsole(string ID) : ID(ID), name(ID) {
    color = ImGui::GetColorU32(ImVec4(255,255,255,255));
}

void ImConsole::render() {
    if (!sensitive) ImGui::BeginDisabled();

    static int tick = 0; tick = (tick+1)%100;
    bool colorLabel = (!tabOpen && changed && tick < 50);

    if (colorLabel) ImGui::PushStyleColor(ImGuiCol_Text, color);
    tabOpen = ImGui::BeginTabItem(name.c_str());
    if (colorLabel) ImGui::PopStyleColor();

    if (tabOpen) {
        auto r = ImGui::GetContentRegionAvail();
        string wID = "##"+name+"_text";

        if (changed > 0) {
            //size_t N = countLines(data);
            size_t N = lines.size();
            ImGui::SetNextWindowScroll(ImVec2(-1, N * ImGui::GetTextLineHeight()));
            changed -= 1; // for some reason needs two passes
        }

		//ImGui::InputTextMultiline(wID.c_str(), &data[0], data.size(), r, ImGuiInputTextFlags_ReadOnly);
		ImGui::BeginChild(wID.c_str());
		ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0,0,0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
		size_t i = 0;
		for (auto& l : lines) {
            string lID = wID + toString(i);

            bool colorized = false;
            if (attributes.count(i)) {
                auto& a = attributes[i];
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,0,0,255));
                colorized = true;
            }

            // TODO: test with InputTextMultiline
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText(lID.c_str(), &l[0], l.size(), ImGuiInputTextFlags_ReadOnly);
            if (colorized) ImGui::PopStyleColor();

            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased( 0 ) ) {
                if (attributes.count(i)) {
                    float mP = ImGui::GetMousePos().x;
                    float r0 = ImGui::GetItemRectMin().x;
                    ImFont* font = ImGui::GetFont();

                    int cM = 0; // mouse column clicked in editor
                    float pC = r0;
                    for (int j=0; j<l.size(); j++) {
                        auto& c = l[j];
                        pC += font->CalcTextSizeA(font->FontSize, FLT_MAX, 0, &c, &c + 1).x;
                        if (mP < pC) {
                            cM = j;
                            break;
                        }
                    }

                    auto& a = attributes[i];
                    for (auto& mark : a.marks) {
                        if (cM > mark.c0 && cM < mark.c0+mark.L) {
                            uiSignal("clickConsole", {{"mark",mark.value}, {"ID",ID}});
                            break;
                        }
                    }
                }
            }
            i++;
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::EndChild();

        ImGui::EndTabItem();
    }

    if (!sensitive) ImGui::EndDisabled();
}

void ImConsole::clear() {
    lines.clear();
    attributes.clear();
    changed = 0;
}

ImConsoles::ImConsoles() : ImWidget("Consoles") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("newConsole", [&](OSG::VRGuiSignals::Options o){ newConsole(o["ID"], o["color"]); return true; } );
    mgr->addCallback("setupConsole", [&](OSG::VRGuiSignals::Options o){ setupConsole(o["ID"], o["name"]); return true; } );
    mgr->addCallback("pushConsole", [&](OSG::VRGuiSignals::Options o){ pushConsole(o["ID"], o["string"], o["style"], o["mark"]); return true; } );
    mgr->addCallback("clearConsole", [&](OSG::VRGuiSignals::Options o){ consoles[o["ID"]].clear(); return true; } );
    mgr->addCallback("clearConsoles", [&](OSG::VRGuiSignals::Options o){ for (auto& c : consoles) c.second.clear(); return true; } );
    mgr->addCallback("setConsoleLabelColor", [&](OSG::VRGuiSignals::Options o){ setConsoleLabelColor(o["ID"], o["color"]); return true; } );
}

void ImConsoles::setConsoleLabelColor(string ID, string color) {
    auto c = colorFromString(color);
    consoles[ID].color = ImGui::GetColorU32(c);
}

void ImConsoles::newConsole(string ID, string color) {
    consoles[ID] = ImConsole(ID);
    consolesOrder.push_back(ID);
    setConsoleLabelColor(ID, color);
}

void ImConsoles::clearConsole(string ID) {
    if (!consoles.count(ID)) return;
    consoles[ID].clear();
}

void ImConsoles::setupConsole(string ID, string name) {
    if (!consoles.count(ID)) return;
    consoles[ID].name = name;
}

void ImConsoles::pushConsole(string ID, string data, string style, string mark) {
    //cout << " - - - - - - - ImConsoles::pushConsole " << ID << "  '" << data << "'  " << style << "  " << mark << endl;
    if (data.size() == 0) return;
    if (!consoles.count(ID)) return;
    consoles[ID].changed = 2;
    auto& lns = consoles[ID].lines;
    auto& att = consoles[ID].attributes;
    auto dataV = splitString(data, '\n');
    for (int i=0; i<dataV.size(); i++) {
        int c0 = 0;
        if (lns.size() > 0) c0 = lns[lns.size()-1].size();
        int L = dataV[i].size();

        if (i == 0 && lns.size() > 0) lns[lns.size()-1] += dataV[i];
        else lns.push_back(dataV[i]);

        if (mark.size() > 0)  att[lns.size()-1].marks.push_back({mark, c0, L});
        if (style.size() > 0) att[lns.size()-1].styles.push_back({style, c0, L});
    }
    if (data[data.size()-1] == '\n') lns.push_back("");
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
    vector<const char*> tmpCameras(cameras.size(), 0);
    for (int i=0; i<cameras.size(); i++) tmpCameras[i] = cameras[i].c_str();
    ImGui::SameLine();
    ImGui::Text("Camera:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("##Cameras", &current_camera, &tmpCameras[0], tmpCameras.size())) {
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

    ImGui::SameLine(ImGui::GetWindowWidth()-280);
    ImGui::SetNextItemWidth(100);
    if (ImGui::BeginCombo("##UItheme", "Theme", 0)) {
        if (ImGui::RadioButton("Light", &uiTheme, 0)) { ImGui::StyleColorsLight(); uiStoreParameter("uiTheme", "light"); }
        if (ImGui::RadioButton("Dark", &uiTheme, 1)) { ImGui::StyleColorsDark(); uiStoreParameter("uiTheme", "dark"); }
        if (ImGui::RadioButton("Classic", &uiTheme, 2)) { ImGui::StyleColorsClassic(); uiStoreParameter("uiTheme", "classic"); }
        ImGui::EndCombo();
    }
    ImGui::SameLine();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Font size:"); ImGui::SameLine();
    //io.FontAllowUserScaling = false;
    if (ImGui::Button("+##FontSizeP")) { io.FontGlobalScale += 0.1; uiStoreParameter("fontScale", toString(io.FontGlobalScale)); } ImGui::SameLine();
    if (ImGui::Button("-##FontSizeM")) { io.FontGlobalScale -= 0.1; uiStoreParameter("fontScale", toString(io.FontGlobalScale)); } ImGui::SameLine();
    if (ImGui::Button("1##FontSize1")) { io.FontGlobalScale = 1.0;  uiStoreParameter("fontScale", toString(io.FontGlobalScale)); }

}

void ImConsoles::begin() {
    viewControls.render();
    ImGui::Separator();

    if (ImGui::BeginTabBar("ConsolesTabBar", ImGuiTabBarFlags_None)) {
        for (auto& c : consolesOrder) consoles[c].render();
        ImGui::EndTabBar();
    }

    ImGui::SameLine(ImGui::GetWindowWidth()-280);
    if (ImGui::Button("clear")) uiSignal("clearConsoles");
}
