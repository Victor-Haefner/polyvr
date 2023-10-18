#include "VRImguiSceneRendering.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

ImRendering::ImRendering() : pathInput("bgPath", "Path:", ""), extInput("bgExtension", "Extension:", "") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("set_bg_type", [&](OSG::VRGuiSignals::Options o){ setBGType(o["type"]); return true; } );
    mgr->addCallback("set_bg_solid_color", [&](OSG::VRGuiSignals::Options o){ setBGColor(o["color"]); return true; } );
    mgr->addCallback("set_bg_path", [&](OSG::VRGuiSignals::Options o){ setBGPath(o["color"]); return true; } );
    mgr->addCallback("set_bg_file_ext", [&](OSG::VRGuiSignals::Options o){ setBGExt(o["color"]); return true; } );
}

void ImRendering::setBGType(string data) {
    if (data == "solid") bgType = 0;
    if (data == "image") bgType = 1;
    if (data == "skybox") bgType = 2;
    if (data == "sky") bgType = 3;
}

void ImRendering::setBGColor(string data) {
    auto parts = splitString(data);
    color.x = toFloat(parts[0]);
    color.y = toFloat(parts[0]);
    color.z = toFloat(parts[0]);
    color.w = toFloat(parts[0]);
}

void ImRendering::setBGPath(string data) {
    pathInput.value = data;
}

void ImRendering::setBGExt(string data) {
    extInput.value = data;
}

void ImRendering::render() {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;

    if (ImGui::Checkbox("V-Sync", &vsync)) uiSignal("ui_toggle_vsync", {{"active",toString(vsync)}});
    ImGui::SameLine();
    if (ImGui::Checkbox("Framesleep", &framesleep)) {
        int fps = framesleep ? targetFPS : 0;
        uiSignal("ui_set_framesleep", { {"fps",toString(fps)} });
    }
    if (framesleep) {
        ImGui::SameLine();
        static ImInput fpsInput("fpsInput", "Target FPS:", toString(targetFPS));
        if (fpsInput.render(50)) {
            targetFPS = toInt(fpsInput.value);
            uiSignal("ui_set_framesleep", { {"fps",toString(targetFPS)} });
        }
    }

    if (ImGui::CollapsingHeader("Background##bg", flags)) {
        if (ImGui::RadioButton("Solid", &bgType, 0)) uiSignal("on_toggle_bg", {{"type","solid"}});
        ImGui::SameLine();
        if (ImGui::RadioButton("Image", &bgType, 1)) uiSignal("on_toggle_bg", {{"type","image"}});
        ImGui::SameLine();
        if (ImGui::RadioButton("Skybox", &bgType, 2)) uiSignal("on_toggle_bg", {{"type","skybox"}});
        ImGui::SameLine();
        if (ImGui::RadioButton("Sky", &bgType, 3)) uiSignal("on_toggle_bg", {{"type","sky"}});

        if (bgType == 0) { // solid
            ImGui::Text("Color:");
            ImGui::SameLine();
            if (ImGui::ColorEdit4("##bgpicker", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoSidePreview)) {
                uiSignal("on_change_bg_color", {{"color",toString(color.x)+"|"+toString(color.y)+"|"+toString(color.z)+"|"+toString(color.w)}});
            }
        }

        if (bgType == 1) {
            if (pathInput.render(-1)) uiSignal("on_change_bg_path", {{"path",pathInput.value}});
        }

        if (bgType == 2) {
            if (pathInput.render(-1)) uiSignal("on_change_bg_path", {{"path",pathInput.value}});
            if (extInput.render(-1)) uiSignal("on_change_bg_ext", {{"ext",pathInput.value}});
        }

        if (bgType == 3) {
            ImGui::Text("Sky params (TODO)"); // TODO: set some of the params like speed, overcast etc..
        }
    }
}
