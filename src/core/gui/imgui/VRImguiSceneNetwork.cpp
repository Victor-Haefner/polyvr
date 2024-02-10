#include "VRImguiSceneNetwork.h"
#include "core/gui/VRGuiManager.h"

using namespace std;

ImNetwork::ImNetwork() {}

void ImNetwork::render() {
    // toolbar
    ImGui::Spacing();
    ImGui::Indent(5);
        if (ImGui::Button("Update")) uiSignal("ui_network_update");
    ImGui::Unindent();
}
