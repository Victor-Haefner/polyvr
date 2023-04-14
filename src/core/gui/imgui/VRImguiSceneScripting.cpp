#include "VRImguiSceneScripting.h"
#include "core/gui/VRGuiManager.h"

#include <iostream>
#include "core/utils/toString.h"

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
    cout << " ----- addGroup " << name << ", " << ID << endl;
    if (groups.count(ID)) return;
    groups[ID] = ImScriptGroup(name);
    groupsList.push_back(ID);
}

void ImScriptList::addScript(string name, string groupID) {
    if (groupID == "") groupID = "__default__";
    cout << " ----- addScript " << name << ", " << groupID << endl;
    groups[groupID].scripts.push_back(name);
}

void ImScriptList::renderListEntry(string& script) {
    ImVec4 colorSelected(0.3f, 0.5f, 1.0f, 1.0f);
    bool isSelected = bool(selected == script);
    if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, colorSelected);
    if (!isSelected) {
        if (ImGui::Button(script.c_str())) {
            selected = script;
            uiSignal("select_script", {{"script",script}});
        }
    } else {
        static char str0[128] = "Script0";
        memcpy(str0, script.c_str(), script.size());
        str0[script.size()] = 0;
        if (ImGui::InputText("##renameScript", str0, 128, ImGuiInputTextFlags_EnterReturnsTrue) ) {
            script = string(str0);
            selected = script;
            uiSignal("rename_script", {{"name",string(str0)}});
            uiSignal("select_script", {{"script",script}});
        }
    }
    if (isSelected) ImGui::PopStyleColor();
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
            for (auto& script : groups[groupID].scripts) renderListEntry(script);
            ImGui::Unindent();

        }
    }

    for (auto& script : groups["__default__"].scripts) renderListEntry(script);

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

ImScriptEditor::ImScriptEditor() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("script_editor_set_buffer", [&](OSG::VRGuiSignals::Options o){ setBuffer(o["data"]); return true; } );
    mgr->addCallback("script_editor_set_parameters", [&](OSG::VRGuiSignals::Options o){ setParameters(o["type"], o["group"]); return true; } );
    mgr->addCallback("script_editor_request_buffer", [&](OSG::VRGuiSignals::Options o){ getBuffer(toInt(o["skipLines"])); return true; } );
    mgr->addCallback("scripts_list_clear", [&](OSG::VRGuiSignals::Options o){ clear(); return true; } );
    mgr->addCallback("scripts_list_add_group", [&](OSG::VRGuiSignals::Options o){ addGroup(o["name"], o["ID"]); return true; } );
    imEditor.SetShowWhitespaces(false); // TODO: add as feature!

    typeList = {"Logic (Python)", "Shader (GLSL)", "Web (HTML/JS/CSS)"};
}

void ImScriptEditor::getBuffer(int skipLines) {
    string core = imEditor.GetText();
    int begin = 0;
    for (int i=0; i<skipLines; i++) begin = core.find('\n', begin) + 1;
    core = subString(core, begin);
    uiSignal("script_editor_transmit_core", {{"core",core}});
}

void ImScriptEditor::setBuffer(string data) {
    imEditor.SetText(data);
}

void ImScriptEditor::setParameters(string type, string group) {
    current_group = 0;
    current_type = 0;
    for (int i=0; i<groupList.size(); i++) if (groupList[i] == group) current_group = i;
    for (int i=0; i<typeList.size() ; i++) if (typeList[i]  == type)  current_type = i;
}

void ImScriptEditor::clear() {
    groups.clear();
    groups["no group"] = "";
    groupList.clear();
    groupList.push_back("no group");
}

void ImScriptEditor::addGroup(string name, string ID) {
    string nameID = name + "##" + ID;
    groups[nameID] = name;
    groupList.clear();
    for (auto& g : groups) groupList.push_back(g.first);
}


void ImScriptEditor::render() {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
    if (ImGui::CollapsingHeader("Options", flags)) {
        ImGui::Text("Type: ");
        ImGui::SameLine();
        const char* types[typeList.size()];
        for (int i=0; i<typeList.size(); i++) types[i] = typeList[i].c_str();
        if (ImGui::Combo("##scriptTypesCombo", &current_type, types, typeList.size())) {
            string type = typeList[current_type];
            uiSignal("script_editor_change_type", {{"type",type}});
        }

        const char* groupsCstr[groupList.size()];
        for (int i=0; i<groupList.size(); i++) groupsCstr[i] = groupList[i].c_str();
        ImGui::Text("Group:");
        ImGui::SameLine();
        if (ImGui::Combo("##groupsCombo", &current_group, groupsCstr, groupList.size())) {
            string group = groups[groupList[current_group]];
            uiSignal("script_editor_change_group", {{"group",group}});
        }
    }

    if (ImGui::CollapsingHeader("Triggers", flags)) {
    }

    if (ImGui::CollapsingHeader("Arguments", flags)) {
    }

    imEditor.Render("Editor");
    if (imEditor.IsTextChanged()) uiSignal("script_editor_text_changed");
}



ImScripting::ImScripting() {}

void ImScripting::render() {
    // toolbar
    ImGui::Spacing();
    ImGui::Indent(5);  if (ImGui::Button("New")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Template")) uiSignal("scripts_toolbar_template");
    ImGui::SameLine(); if (ImGui::Button("Group")) uiSignal("scripts_toolbar_group");
    ImGui::SameLine(); if (ImGui::Button("Import")) uiSignal("scripts_toolbar_import");
    ImGui::SameLine(); if (ImGui::Button("Delete")) uiSignal("scripts_toolbar_delete");
    ImGui::SameLine(); if (ImGui::Checkbox("Pause Scripts", &pause)) uiSignal("scripts_toolbar_pause", {{"state",toString(pause)}});
    ImGui::SameLine(); if (ImGui::Button("CPP")) uiSignal("scripts_toolbar_cpp");

                       if (ImGui::Button("Save")) uiSignal("scripts_toolbar_save");
    ImGui::SameLine(); if (ImGui::Button("Execute")) uiSignal("scripts_toolbar_execute");
    ImGui::SameLine(); if (ImGui::Button("Search")) uiSignal("scripts_toolbar_search");
    ImGui::SameLine(); if (ImGui::Button("Documentation")) uiSignal("scripts_toolbar_documentation");
    ImGui::SameLine(); if (ImGui::Checkbox("Performance", &perf)) uiSignal("scripts_toolbar_performance", {{"state",toString(perf)}});
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


