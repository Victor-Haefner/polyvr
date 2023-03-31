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

void ImConsoles::begin() {
    if (ImGui::BeginTabBar("ConsolesTabBar", ImGuiTabBarFlags_None)) {
        for (auto& c : consolesOrder) consoles[c].render();
        ImGui::EndTabBar();
    }
}
