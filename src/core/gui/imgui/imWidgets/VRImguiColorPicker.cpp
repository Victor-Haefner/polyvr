#include "VRImguiColorPicker.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

#include <iostream>
using namespace std;

ImColorPicker::ImColorPicker(string ID, string label) : ID("##"+ID), label(label) {}

bool ImColorPicker::render() {
    if (label != "") {
        ImGui::Text(label.c_str());
        ImGui::SameLine();
    }

    return ImGui::ColorEdit4("##bgpicker", (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoSidePreview);
}

void ImColorPicker::signal(string s) {
    uiSignal(s, {{"color",get()}});
}

void ImColorPicker::set(string s) {
    auto parts = splitString(s);
    color.x = toFloat(parts[0]);
    color.y = toFloat(parts[0]);
    color.z = toFloat(parts[0]);
    color.w = toFloat(parts[0]);
}

string ImColorPicker::get() {
    return toString(color.x)+"|"+toString(color.y)+"|"+toString(color.z)+"|"+toString(color.w);
}
