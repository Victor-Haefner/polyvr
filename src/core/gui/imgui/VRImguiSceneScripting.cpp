#include "VRImguiSceneScripting.h"
#include "core/gui/VRGuiManager.h"

#include <iostream>
#include "imWidgets/VRImguiInput.h"
#include "core/utils/toString.h"

using namespace std;

void renderInput(string& label, string ID, string signal, string idKey) {
    static char str0[128] = "Script0";
    memcpy(str0, label.c_str(), label.size());
    str0[label.size()] = 0;
    ID = "##"+ID;

    if (ImGui::InputText(ID.c_str(), str0, 128, ImGuiInputTextFlags_EnterReturnsTrue) ) {
        uiSignal(signal, {{"idKey",idKey}, {"inputOld",label}, {"inputNew",string(str0)}});
        label = string(str0);
    }
}

void renderCombo(string& opt, vector<string>& options, string ID, string signal, string idKey) {
    ID = "##"+ID;
    int labelI = 0;
    vector<const char*> optionsCstr(options.size(),0);
    for (int i=0; i<options.size(); i++) {
        optionsCstr[i] = options[i].c_str();
        if (options[i] == opt) labelI = i;
    }

    if (ImGui::Combo(ID.c_str(), &labelI, &optionsCstr[0], optionsCstr.size())) {
        opt = options[labelI];
        uiSignal(signal, {{"idKey",idKey}, {"newValue",opt}});
    }
}

string formatPerformance(float exec_time) {
    string time = " ";
    if (exec_time >= 60*1000) time = toString( exec_time*0.001/60 ) + " min";
    else if (exec_time >= 1000) time = toString( exec_time*0.001 ) + " s";
    else if (exec_time >= 0) time = toString( exec_time ) + " ms";
    return time;
}

ImScriptGroup::ImScriptGroup(string name) : name(name) {}
ImScriptEntry::ImScriptEntry(string name) : name(name) {}

ImScriptList::ImScriptList() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("scripts_list_clear", [&](OSG::VRGuiSignals::Options o){ clear(); return true; } );
    mgr->addCallback("scripts_list_add_group", [&](OSG::VRGuiSignals::Options o){ addGroup(o["name"], o["ID"]); return true; } );
    mgr->addCallback("scripts_list_add_script", [&](OSG::VRGuiSignals::Options o){ addScript(o["name"], o["group"], toFloat(o["perf"])); return true; } );
    mgr->addCallback("scripts_list_set_color", [&](OSG::VRGuiSignals::Options o){ setColor(o["name"], o["fg"], o["bg"]); return true; } );
    mgr->addCallback("openUiScript", [&](OSG::VRGuiSignals::Options o) {
        selected = o["name"];
        uiSignal("select_script", {{"script",selected}});
        return true;
    } );
}

void ImScriptList::clear() {
    groups.clear();
    groupsList.clear();
    addGroup("__default__", "__default__");
    computeMinWidth();
}

void ImScriptList::addGroup(string name, string ID) {
    if (groups.count(ID)) return;
    groups[ID] = ImScriptGroup(name);
    groupsList.push_back(ID);
}

void ImScriptList::addScript(string name, string groupID, float time) {
    if (groupID == "") groupID = "__default__";
    ImScriptEntry se(name);
    se.perf = time;
    groups[groupID].scripts.push_back(se);
    computeMinWidth();
}

void ImScriptList::setColor(string name, string fg, string bg) {
    for (auto& g : groups) {
        for (auto& s : g.second.scripts) {
            if (s.name == name) {
                s.fg = fg;
                s.bg = bg;
                return;
            }
        }
    }
}

void ImScriptList::computeMinWidth() {
    ImGuiStyle& style = ImGui::GetStyle();
    int padding = 20;
    if (groupsList.size() > 1) padding += 8; // for groups
    if (doPerf) padding += 50;
    width = 50;
    width = max(width, float(Rinput+padding));
    for (auto& g : groups) {
        for (auto& s : g.second.scripts) {
            auto R = ImGui::CalcTextSize( s.name.c_str() ).x + style.FramePadding.x * 2.0f;
            width = max(width, R+padding);
        }
    }
}

void ImScriptList::renderScriptEntry(ImScriptEntry& scriptEntry) {
    string& script = scriptEntry.name;
    string bID = script + "##script";
    if (!input) input = new ImInput("##renameScript", "", "Script0", ImGuiInputTextFlags_EnterReturnsTrue);
    //ImVec4 colorSelected(0.3f, 0.5f, 1.0f, 1.0f);
    bool isSelected = bool(selected == bID);
    //if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, colorSelected);
    if (!isSelected) {
		ImGui::PushStyleColor(ImGuiCol_Text, colorFromString(scriptEntry.fg));
		ImGui::PushStyleColor(ImGuiCol_Button, colorFromString(scriptEntry.bg));

        if (ImGui::Button(bID.c_str())) {
            selected = bID;
            uiSignal("select_script", {{"script",script}});
            if (input) input->value = script;
        }

		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
    } else {
		ImGui::PushStyleColor(ImGuiCol_Text, colorFromString(scriptEntry.fg));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, colorFromString(scriptEntry.bg));
		ImGui::PushStyleColor(ImGuiCol_Border, colorFromString("#66AAFF"));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);

        ImGuiStyle& style = ImGui::GetStyle();
        Rinput = ImGui::CalcTextSize( input->value.c_str() ).x + style.FramePadding.x * 2.0f + 5;
        if (input->render(Rinput)) {
            script = input->value;
            selected = bID;
            uiSignal("rename_script", {{"name",script}});
            uiSignal("select_script", {{"script",script}});
        }
        computeMinWidth();

		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
    }

    if (doPerf) {
        string pTime = formatPerformance(scriptEntry.perf);
        ImGui::SameLine();
        ImGui::Text(pTime.c_str());
    }

    //if (isSelected) ImGui::PopStyleColor();
}

void ImScriptList::renderGroupEntry(string& group) {
    string bID = group + "##group";

    bool isSelected = bool(selected == bID);
    if (!isSelected) {
        if (ImGui::Button(bID.c_str())) {
            selected = bID;
            uiSignal("select_group", {{"group",group}});
        }
    } else {
        if (!input) input = new ImInput("##renameGroup", "", "Group0", ImGuiInputTextFlags_EnterReturnsTrue);
        input->value = group;
        if (input->render(-1)) {
            group = input->value;
            selected = bID;
            uiSignal("rename_group", {{"name",group}});
        }
    }
}

void ImScriptList::render() {
    ImVec4 colorGroup(0.3f, 0.3f, 0.3f, 1.0f);
    ImVec4 colorScript(0.5f, 0.5f, 0.5f, 1.0f);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_OpenOnArrow;
    ImGui::PushStyleColor(ImGuiCol_Header, colorGroup);
    ImGui::PushStyleColor(ImGuiCol_Button, colorScript);

    for (auto groupID : groupsList) {
        if (groupID == "__default__") continue;
        string group = groups[groupID].name;
        renderGroupEntry(groups[groupID].name);
        ImGui::SameLine();

        //if (ImGui::CollapsingHeader((group+"##"+groupID).c_str(), flags)) {
        if (ImGui::CollapsingHeader(("##"+groupID).c_str(), flags)) {
            ImGui::Indent();
            for (auto& script : groups[groupID].scripts) renderScriptEntry(script);
            ImGui::Unindent();
        }
    }

    for (auto& script : groups["__default__"].scripts) renderScriptEntry(script);

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

ImScriptEditor::ImScriptEditor() {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("script_editor_set_buffer", [&](OSG::VRGuiSignals::Options o){ setBuffer(o["data"]); return true; } );
    mgr->addCallback("script_editor_set_parameters", [&](OSG::VRGuiSignals::Options o){ setParameters(o["type"], o["group"]); return true; } );
    mgr->addCallback("script_editor_request_buffer", [&](OSG::VRGuiSignals::Options o){ getBuffer(toInt(o["skipLines"])); return true; } );
    mgr->addCallback("scripts_list_clear", [&](OSG::VRGuiSignals::Options o){ clearGroups(); return true; } );
    mgr->addCallback("scripts_list_add_group", [&](OSG::VRGuiSignals::Options o){ addGroup(o["name"], o["ID"]); return true; } );
    mgr->addCallback("script_editor_clear_trigs_and_args", [&](OSG::VRGuiSignals::Options o){ clearTrigsAndArgs(); return true; } );
    mgr->addCallback("script_editor_add_trigger", [&](OSG::VRGuiSignals::Options o){ addTrigger(o["name"], o["trigger"], o["parameter"], o["device"], o["key"], o["state"]); return true; } );
    mgr->addCallback("script_editor_add_argument", [&](OSG::VRGuiSignals::Options o){ addArgument(o["name"], o["type"], o["value"]); return true; } );
    mgr->addCallback("editor_cmd", [&](OSG::VRGuiSignals::Options o){ editorCommand(o["cmd"]); return true; } );
    mgr->addCallback("openUiScript", [&](OSG::VRGuiSignals::Options o){ focusOn(o["line"], o["column"]); return true; } );
    mgr->addCallback("shiftTab", [&](OSG::VRGuiSignals::Options o){ handleShiftTab(toInt(o["state"])); return true; }, true );

    imEditor.SetShowWhitespaces(false); // TODO: add as feature!
    imEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Python());

    typeList = {"Logic (Python)", "Shader (GLSL)", "Web (HTML/JS/CSS)"};
    argumentTypes = {"None", "int", "float", "str", "VRPyObjectType", "VRPyTransformType", "VRPyGeometryType", "VRPyLightType", "VRPyLodType", "VRPyDeviceType", "VRPyMouseType", "VRPyHapticType", "VRPySocketType", "VRPyLeapFrameType"};
    triggerTypes = {"None", "on_scene_load", "on_scene_close", "on_scene_import", "on_timeout", "on_device", "on_socket"};
    device_types = {"None", "mouse", "multitouch", "keyboard", "flystick", "haptic", "server1", "leap", "vrpn_device"};
    trigger_states = {"Pressed", "Released", "Drag", "Drop", "To edge", "From edge"};
}

void ImScriptEditor::handleShiftTab(int state) {
	ImGuiIO& io = ImGui::GetIO();
	io.KeysDown[int('\t')] = state;
}

void ImScriptEditor::focusOn(string line, string column) {
    TextEditor::Coordinates coords(max(0,toInt(line)-1), toInt(column));
    imEditor.SetCursorPosition(coords);
}

void ImScriptEditor::editorCommand(string cmd) {
    if (cmd == "toggleLine") {
        auto p = imEditor.GetCursorPosition();
        if (p.mLine <= 1) return;

        // select current line
        p.mColumn = 0;
        imEditor.SetSelectionStart(p);
        p.mLine += 1;
        imEditor.SetSelectionEnd(p);

        // copy current line and delete
        auto ct = ImGui::GetClipboardText();
        string cts = ct?ct:"";
        imEditor.Copy();
        imEditor.Delete();

        // paste above
        p.mLine -= 2;
        imEditor.SetSelectionStart(p);
        imEditor.SetSelectionEnd(p);
        imEditor.SetCursorPosition(p);
        imEditor.Paste();
        ImGui::SetClipboardText(cts.c_str());
    }

    if (cmd == "duplicateLine") {
        bool duplicateLine = false;
        auto p0 = imEditor.GetCursorPosition();

        if (!imEditor.HasSelection()) {
            duplicateLine = true;
            auto p = imEditor.GetCursorPosition();
            p.mColumn = 0;
            imEditor.SetSelectionStart(p);
            p.mLine += 1;
            imEditor.SetSelectionEnd(p);
        }

        auto ct = ImGui::GetClipboardText();
        string cts = ct?ct:"";
        imEditor.Copy();
        imEditor.Paste(); // instead of two pastes do unselect
        imEditor.Paste();
        ImGui::SetClipboardText(cts.c_str());

        if (duplicateLine) {
            auto p = imEditor.GetCursorPosition();
            p.mLine -= 1;
            p.mColumn = p0.mColumn;
            imEditor.SetCursorPosition(p);
        }
    }

    uiSignal("script_editor_text_changed");
}

void ImScriptEditor::getBuffer(int skipLines) {
    string core = imEditor.GetText();
    int begin = 0;
    for (int i=0; i<skipLines; i++) begin = core.find('\n', begin) + 1;
    core = subString(core, begin);
    uiSignal("script_editor_transmit_core", {{"core",core}});
}

void ImScriptEditor::setBuffer(string data) {
    TextEditor::Coordinates c;
    imEditor.SetSelection(c,c); // deselect
    imEditor.SetText(data);
    sensitive = true;
    if (data == "") sensitive = false;
}

void ImScriptEditor::setParameters(string type, string group) {
    current_group = 0;
    for (int i=0; i<groupList.size(); i++) if (groupList[i] == group) current_group = i;

    current_type = 0; // Python
    if (type == "GLSL") current_type = 1;
    if (type == "HTML") current_type = 2;
}

void ImScriptEditor::addTrigger(string name, string trigger, string parameter, string device, string key, string state) {
    triggers.push_back({name, trigger, parameter, device, key, state});
}

void ImScriptEditor::addArgument(string name, string type, string value) {
    arguments.push_back({name, type, value});
}

void ImScriptEditor::clearTrigsAndArgs() {
    triggers.clear();
    arguments.clear();
}

void ImScriptEditor::clearGroups() {
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
    if (!sensitive) ImGui::BeginDisabled();
    ImGuiIO& io = ImGui::GetIO();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
    if (ImGui::CollapsingHeader("Options", flags)) {
        ImGui::Text("Type: ");
        ImGui::SameLine();
        vector<const char*> types(typeList.size(),0);
        for (int i=0; i<typeList.size(); i++) types[i] = typeList[i].c_str();
        if (ImGui::Combo("##scriptTypesCombo", &current_type, &types[0], types.size())) {
            string type = "Python";
            if (current_type == 1) type = "GLSL";
            if (current_type == 2) type = "HTML";
            uiSignal("script_editor_change_type", {{"type",type}});
        }

        vector<const char*> groupsCstr(groupList.size(),0);
        for (int i=0; i<groupList.size(); i++) groupsCstr[i] = groupList[i].c_str();
        ImGui::Text("Group:");
        ImGui::SameLine();
        if (ImGui::Combo("##groupsCombo", &current_group, &groupsCstr[0], groupsCstr.size())) {
            string group = groups[groupList[current_group]];
            uiSignal("script_editor_change_group", {{"group",group}});
        }
    }

    if (ImGui::CollapsingHeader("Triggers", flags)) {
        ImGui::Indent();
        if (ImGui::Button("+##newTrig")) { uiSignal("script_editor_new_trigger"); };
        for (auto& t : triggers) {
            if (ImGui::Button(("-##remTrig-"+t.name).c_str())) { uiSignal("script_editor_rem_trigger", {{"trigger", t.name}}); }; ImGui::SameLine();
            float w = ImGui::GetContentRegionAvail().x;
            ImGui::PushItemWidth(w*0.26 - 16);
            renderCombo(t.trigger, triggerTypes, "trigType-"+t.name, "script_editor_change_trigger_type", t.name); ImGui::SameLine();
            renderInput(t.parameter, "trigParam-"+t.name, "script_editor_change_trigger_param", t.name); ImGui::SameLine();
            renderCombo(t.device, device_types, "trigDevice-"+t.name, "script_editor_change_trigger_device", t.name); ImGui::SameLine();
            ImGui::PushItemWidth(16);
            renderInput(t.key, "trigKey-"+t.name, "script_editor_change_trigger_key", t.name); ImGui::SameLine();
            ImGui::PopItemWidth();
            renderCombo(t.state, trigger_states, "trigState-"+t.name, "script_editor_change_trigger_state", t.name);
            ImGui::PopItemWidth();
        }
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Arguments", flags)) {
        ImGui::Indent();
        if (ImGui::Button("+##newArg")) { uiSignal("script_editor_new_argument"); };
        for (auto& a : arguments) {
            if (ImGui::Button(("-##remArg-"+a.name).c_str())) { uiSignal("script_editor_rem_argument", {{"argument", a.name}}); }; ImGui::SameLine();
            float w = ImGui::GetContentRegionAvail().x;
            ImGui::PushItemWidth(w*0.33);
            renderInput(a.name, "argName-"+a.name, "script_editor_rename_argument", a.name); ImGui::SameLine();
            renderCombo(a.type, argumentTypes, "argType-"+a.name, "script_editor_change_argument_type", a.name); ImGui::SameLine();
            renderInput(a.value, "argValue-"+a.name, "script_editor_change_argument", a.name);
            ImGui::PopItemWidth();
        }
        ImGui::Unindent();
    }

    if (sensitive) {
        static TextEditor::Coordinates lastCoords = imEditor.GetCursorPosition();
        if (!io.KeyShift && !imEditor.HasSelection()) lastCoords = imEditor.GetCursorPosition();

        imEditor.Render("Editor");
        if (imEditor.IsTextChanged()) uiSignal("script_editor_text_changed");

        if (ImGui::IsItemHovered()) { // shift selection
            if( ImGui::IsMouseReleased(0) ) {
                auto coords = imEditor.GetCursorPosition();
                if (io.KeyShift) imEditor.SetSelection(lastCoords, coords, TextEditor::SelectionMode::Normal);
            }
        }
    }

    if (!sensitive) ImGui::EndDisabled();
}



ImScripting::ImScripting() {}

void ImScripting::render() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && io.KeysDown['s']) { io.KeysDown['s'] = false; uiSignal("scripts_toolbar_save"); }
    if (io.KeyCtrl && io.KeysDown['e']) { io.KeysDown['e'] = false; uiSignal("scripts_toolbar_execute"); }
    if (io.KeyCtrl && io.KeysDown['w']) { io.KeysDown['w'] = false; uiSignal("clearConsoles"); }

    // toolbar
    ImGui::Spacing();
    ImGui::Indent(5);  if (ImGui::Button("New")) uiSignal("scripts_toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Template")) uiSignal("ui_toggle_popup", {{"name","template"}, {"width","800"}, {"height","600"}});
    ImGui::SameLine(); if (ImGui::Button("Group")) uiSignal("scripts_toolbar_group");
    ImGui::SameLine(); if (ImGui::Button("Import")) uiSignal("ui_toggle_popup", {{"name","import"}, {"width","400"}, {"height","300"}});
    ImGui::SameLine(); if (ImGui::Button("Delete")) uiSignal("askUser", {{"msg1","This will remove the selected script!"}, {"msg2","Are you sure?"}, {"sig","scripts_toolbar_delete"}});
    ImGui::SameLine(); if (ImGui::Checkbox("Pause Scripts", &pause)) uiSignal("scripts_toolbar_pause", {{"state",toString(pause)}});
    ImGui::SameLine(); if (ImGui::Button("CPP")) uiSignal("scripts_toolbar_cpp");

                       if (ImGui::Button("Save")) uiSignal("scripts_toolbar_save");
    ImGui::SameLine(); if (ImGui::Button("Execute")) uiSignal("scripts_toolbar_execute");
    ImGui::SameLine(); if (ImGui::Button("Search")) uiSignal("ui_toggle_popup", {{"name","search"}, {"width","400"}, {"height","300"}}); // TODO: pass current editor selection
    ImGui::SameLine(); if (ImGui::Button("Documentation")) uiSignal("ui_toggle_popup", {{"name","documentation"}, {"width","800"}, {"height","600"}});
    ImGui::SameLine(); if (ImGui::Checkbox("Performance", &perf)) { scriptlist.doPerf = perf; uiSignal("scripts_toolbar_performance", {{"state",toString(perf)}}); }
    ImGui::Unindent();

    ImGui::Spacing();
    //ImGui::Separator();

    // script list
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetContentRegionAvail().y;

    float w1 = min(float(w*0.5), scriptlist.width);
    float w2 = w - w1;

    ImGui::BeginGroup();
    ImGui::Spacing();
    ImGui::BeginChild("ScriptListPanel", ImVec2(w1, h), true, flags);
    scriptlist.render();
    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::SameLine();

    // script editor
    ImGui::BeginGroup();
    ImGui::Spacing();
    ImGui::BeginChild("ScriptEditorPanel", ImVec2(w2, h), false, flags);
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
        if (io.KeyCtrl && io.KeysDown['t']) { io.KeysDown['t'] = false; uiSignal("editor_cmd", {{"cmd","toggleLine"}}); }
        if (io.KeyCtrl && io.KeysDown['d']) { io.KeysDown['d'] = false; uiSignal("editor_cmd", {{"cmd","duplicateLine"}}); }
    }

    editor.render();

    ImGui::EndChild();
    ImGui::EndGroup();
}


