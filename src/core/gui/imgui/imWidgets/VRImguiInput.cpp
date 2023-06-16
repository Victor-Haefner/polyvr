#include "VRImguiInput.h"

ImInput::ImInput(string ID, string label, string value, int flags) : ID("##"+ID), label(label), value(value), flags(flags) {}

bool ImInput::render() {
    if (label != "") {
        ImGui::Text(label.c_str());
        ImGui::SameLine();
    }

    static char str0[128] = "";
    int N = min(128, int(value.size()));
    memcpy(str0, value.c_str(), N);
    str0[N] = 0;

    if (ImGui::InputText(ID.c_str(), str0, 128, flags) ) {
        value = string(str0);
        return true;
    }

    return false;
}
