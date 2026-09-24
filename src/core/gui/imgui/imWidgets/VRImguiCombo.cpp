#include "VRImguiCombo.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

#include <iostream>
using namespace std;

ImCombo::ImCombo(string ID, string label, int flags) : ID("##"+ID), label(label), flags(flags) {}

void ImCombo::setList(vector<string> v) {
    strings = v;
    cstrings = vector<const char*>(v.size(), 0);
    for (size_t i=0; i<strings.size(); i++) cstrings[i] = strings[i].c_str();
}

void ImCombo::setList(string v) {
    toValue(v, strings);
    setList(strings);
}

void ImCombo::appendList(string s) {
    strings.push_back(s);
    cstrings.push_back(strings[strings.size()-1].c_str());
}

void ImCombo::clearList() {
    strings.clear();
    cstrings.clear();
}

bool ImCombo::render(int width) {
    if (label != "") {
        ImGui::TextUnformatted(label.c_str());
        ImGui::SameLine();
    }

    bool ret = false;
    ImGui::SetNextItemWidth(width);
    if (ImGui::Combo(ID.c_str(), &current, &cstrings[0], cstrings.size())) {
        ret = true;
    }
    return ret;
}

void ImCombo::signal(string s) {
    uiSignal(s, {{"selection", strings[current]}, {"index", toString(current)}});
}

void ImCombo::set(string s) {
    current = 0;
    for (int i=0; i<strings.size(); i++) if (strings[i] == s) current = i;
}

string ImCombo::get() { return strings[current]; }
