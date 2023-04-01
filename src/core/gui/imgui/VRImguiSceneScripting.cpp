#include "VRImguiSceneScripting.h"
#include "core/gui/VRGuiManager.h"


ImScriptList::ImScriptList() {}

void ImScriptList::render() {
    ;
}

ImScriptEditor::ImScriptEditor() {}

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


