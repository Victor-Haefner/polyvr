#include "VRImguiSetup.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/imgui/imWidgets/VRImguiInput.h"

ImSetupManager::ImSetupManager() : ImWidget("SetupManager") {
    auto mgr = OSG::VRGuiSignals::get();
    //mgr->addCallback("newAppLauncher", [&](OSG::VRGuiSignals::Options o){ newAppLauncher(o["panel"], o["ID"]); return true; } );
}

void ImSetupManager::begin() {

}
