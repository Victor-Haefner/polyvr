#include "VRImguiSetup.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

ImSetupManager::ImSetupManager() : ImWidget("SetupManager") {
    auto mgr = OSG::VRGuiSignals::get();
    //mgr->addCallback("newAppLauncher", [&](OSG::VRGuiSignals::Options o){ newAppLauncher(o["panel"], o["ID"]); return true; } );
}

void ImSetupManager::begin() {
    if (ImGui::Checkbox("Fotomode", &fotomode)) uiSignal("ui_toggle_fotomode", {{"active",toString(fotomode)}});
    if (ImGui::Checkbox("V-Sync", &vsync)) uiSignal("ui_toggle_vsync", {{"active",toString(vsync)}});
}
