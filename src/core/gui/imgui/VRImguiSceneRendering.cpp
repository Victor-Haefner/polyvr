#include "VRImguiSceneRendering.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

ImRendering::ImRendering() :
        pathInput("bgPath", "Path:", ""),
        pathSplash("splashPath", "Path:", ""),
        extInput("bgExtension", "Extension:", ""),
        bgColor("bgColor", "Color:") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("set_bg_type", [&](OSG::VRGuiSignals::Options o){ setBGType(o["type"]); return true; } );
    mgr->addCallback("set_bg_solid_color", [&](OSG::VRGuiSignals::Options o){ bgColor.set(o["color"]); return true; } );
    mgr->addCallback("set_bg_path", [&](OSG::VRGuiSignals::Options o){ setBGPath(o["path"]); return true; } );
    mgr->addCallback("set_bg_file_ext", [&](OSG::VRGuiSignals::Options o){ setBGExt(o["ext"]); return true; } );
    mgr->addCallback("set_enable_splash", [&](OSG::VRGuiSignals::Options o){ setShowSplash(toBool(o["show"])); return true; } );
    mgr->addCallback("set_splash_path", [&](OSG::VRGuiSignals::Options o){ setSplashPath(o["path"]); return true; } );
}

void ImRendering::setBGType(string data) {
    if (data == "solid") bgType = 0;
    if (data == "image") bgType = 1;
    if (data == "skybox") bgType = 2;
    if (data == "sky") bgType = 3;
}

void ImRendering::setBGPath(string data) {
    pathInput.value = data;
}

void ImRendering::setBGExt(string data) {
    extInput.value = data;
}

void ImRendering::setSplashPath(string data) {
    pathSplash.value = data;
}

void ImRendering::setShowSplash(bool b) {
    splash = b;
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
            if (bgColor.render()) bgColor.signal("on_change_bg_color");
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

        if (ImGui::Checkbox("Splash Image", &splash)) uiSignal("on_enable_splash", {{"state",toString(splash)}});
        if (splash) {
            if (pathSplash.render(-1)) uiSignal("on_change_splash_path", {{"path",pathSplash.value}});
        }
    }
}
