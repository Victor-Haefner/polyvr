#include "VRImguiSetup.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/imgui/imWidgets/VRImguiInput.h"

ImSetupManager::ImSetupManager() : ImWidget("SetupManager") {
    auto mgr = OSG::VRGuiSignals::get();
    //mgr->addCallback("newAppLauncher", [&](OSG::VRGuiSignals::Options o){ newAppLauncher(o["panel"], o["ID"]); return true; } );
}

void ImSetupManager::begin() {
    if (ImGui::Checkbox("Fotomode", &fotomode)) uiSignal("ui_toggle_fotomode", {{"active",toString(fotomode)}});
    if (ImGui::Checkbox("V-Sync", &vsync)) uiSignal("ui_toggle_vsync", {{"active",toString(vsync)}});
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
}
