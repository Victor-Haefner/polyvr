#include "VRImguiSceneScripting.h"
#include "core/gui/VRGuiManager.h"

#include <iostream>

using namespace std;

ImScriptGroup::ImScriptGroup(string name) : name(name) {}

ImScriptList::ImScriptList() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("scripts_list_clear", [&](OSG::VRGuiSignals::Options o){ clear(); return true; } );
    mgr->addCallback("scripts_list_add_group", [&](OSG::VRGuiSignals::Options o){ addGroup(o["name"], o["ID"]); return true; } );
    mgr->addCallback("scripts_list_add_script", [&](OSG::VRGuiSignals::Options o){ addScript(o["name"], o["group"]); return true; } );
}

void ImScriptList::clear() {
    groups.clear();
    groupsList.clear();
    addGroup("__default__", "__default__");
}

void ImScriptList::addGroup(string name, string ID) {
    if (groups.count(ID)) return;
    groups[ID] = ImScriptGroup(name);
    groupsList.push_back(ID);
}

void ImScriptList::addScript(string name, string groupID) {
    if (groupID == "") groupID = "__default__";
    groups[groupID].scripts.push_back(name);
}

void ImScriptList::render() {
    ImVec4 colorGroup(0.3f, 0.3f, 0.3f, 1.0f);
    ImVec4 colorScript(0.5f, 0.5f, 0.5f, 1.0f);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
    ImGui::PushStyleColor(ImGuiCol_Header, colorGroup);
    ImGui::PushStyleColor(ImGuiCol_Button, colorScript);

    for (auto groupID : groupsList) {
        if (groupID == "__default__") continue;
        string group = groups[groupID].name;

        if (ImGui::CollapsingHeader((group+"##"+groupID).c_str(), flags)) {

            ImGui::Indent();
            for (auto script : groups[groupID].scripts) {
                if (ImGui::Button(script.c_str())) uiSignal("select_script", {{"script",script}});
            }
            ImGui::Unindent();

        }
    }

    for (auto script : groups["__default__"].scripts) {
        if (ImGui::Button(script.c_str())) uiSignal("select_script", {{"script",script}});
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

ImScriptEditor::ImScriptEditor() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("script_editor_set_buffer", [&](OSG::VRGuiSignals::Options o){ setBuffer(o["data"]); return true; } );
    imEditor.SetShowWhitespaces(false); // TODO: add as feature!
}

void ImScriptEditor::setBuffer(string data) {
    imEditor.SetText(data);
}

void ImScriptEditor::render() {
    imEditor.Render("Editor");
}



ImScripting::ImScripting() {}

void ImScripting::render() {
    // toolbar
    ImGui::Spacing();
    ImGui::Indent(5);  if (ImGui::Button("New")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Template")) uiSignal("scripts_toolbar_template");
    ImGui::SameLine(); if (ImGui::Button("Group")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Import")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Delete")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Checkbox("Pause Scripts", &pause)) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("CPP")) uiSignal("scripts_toolbar_new");

                       if (ImGui::Button("Save")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Execute")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Search")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Documentation")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Checkbox("Performance", &perf)) uiSignal("scripts_toolbar_new");
    ImGui::Unindent();

    ImGui::Spacing();
    //ImGui::Separator();

    // script list
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetContentRegionAvail().y;

    ImGui::BeginGroup();
    ImGui::Spacing();
    ImGui::BeginChild("ScriptListPanel", ImVec2(w*0.3, h), true, flags);
    scriptlist.render();
    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::SameLine();

    // script editor
    ImGui::BeginGroup();
    ImGui::Spacing();
    ImGui::BeginChild("ScriptEditorPanel", ImVec2(w*0.7, h), false, flags);
    editor.render();
    ImGui::EndChild();
    ImGui::EndGroup();
}


