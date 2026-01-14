#include "VRImguiColorPicker.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

#include <iostream>
#include <OpenSG/OSGColor.h>

using namespace std;

ImColorPicker::ImColorPicker(string ID, string label) : ID("##"+ID), label(label) {}

bool ImColorPicker::render() {
    if (label != "") {
        ImGui::TextUnformatted(label.c_str());
        ImGui::SameLine();
    }

    return ImGui::ColorEdit4(ID.c_str(), (float*)&color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoSidePreview);
}

void ImColorPicker::signal(string s) {
    uiSignal(s, {{"color",get()}});
}

void ImColorPicker::set(string s) {
    OSG::Color4f c;
    toValue(s, c);
    color.x = c[0];
    color.y = c[1];
    color.z = c[2];
    color.w = c[3];
}

string ImColorPicker::get() {
    OSG::Color4f c(color.x, color.y, color.z, color.w);
    return toString(c);
}
