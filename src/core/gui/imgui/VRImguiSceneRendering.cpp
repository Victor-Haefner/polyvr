#include "VRImguiSceneRendering.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

ImRendering::ImRendering() : pathInput("bgPath", "Path:", ""), formatInput("bgEnding", "Ending:", "") {}

// TODO: load settings from scene
// TODO: finalize background signal handling

void ImRendering::render() {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;

    if (ImGui::CollapsingHeader("Background##bg", flags)) {
        static int bg = 0;

        if (ImGui::RadioButton("Solid", &bg, 0)) uiSignal("on_toggle_bg", {{"bg","Solid"}});
        ImGui::SameLine();
        if (ImGui::RadioButton("Image", &bg, 1)) uiSignal("on_toggle_bg", {{"bg","Image"}});
        ImGui::SameLine();
        if (ImGui::RadioButton("Skybox", &bg, 2)) uiSignal("on_toggle_bg", {{"bg","Skybox"}});
        ImGui::SameLine();
        if (ImGui::RadioButton("Sky", &bg, 3)) uiSignal("on_toggle_bg", {{"bg","Sky"}});

        if (bg == 0) { // solid
            static ImVec4 color;
            ImGui::Text("Color:");
            ImGui::SameLine();
            if (ImGui::ColorEdit4("##bgpicker", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoSidePreview)) {
                uiSignal("set_bg_solid_color", {{"color",toString(color.x)+"|"+toString(color.y)+"|"+toString(color.z)+"|"+toString(color.w)}});
            }
        }

        if (bg == 1) {
            if (pathInput.render()) uiSignal("set_bg_img_path", {{"path",pathInput.value}});
        }

        if (bg == 2) {
            bool b1 = pathInput.render();
            bool b2 = formatInput.render();
            if (b1 || b2) uiSignal("set_bg_skybox_path", {{"path",pathInput.value},{"format",formatInput.value}});
        }

        if (bg == 3) {
            ImGui::Text("Sky params (TODO)"); // TODO: set some of the params like speed, overcast etc..
        }
    }
}
