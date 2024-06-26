#include "VRImguiInput.h"

#include <iostream>
using namespace std;

ImInput::ImInput(string ID, string label, string value, int flags) : ID("##"+ID), label(label), value(value), flags(flags) {}

int imguiInputCb(ImGuiInputTextCallbackData* data) { // the callback reacts on any edit to make sure the value is always set, even if InputText only reacts on ENTER
    ImInput& input = *((ImInput*)data->UserData);
    input.value = string(data->Buf, data->BufTextLen);
    //cout << "imguiInputCb " << data->BufTextLen << ", " << input.value << endl;
    return 0;
}

bool ImInput::render(int width) {
    if (label != "") {
        ImGui::Text(label.c_str());
        ImGui::SameLine();
    }

    const int Nb = 512;
    static char str0[Nb] = "";
    int N = min(Nb, int(value.size()));
    memcpy(str0, value.c_str(), N);
    str0[N] = 0;

    bool ret = false;
    ImGui::PushItemWidth(width);
    if (ImGui::InputText(ID.c_str(), str0, Nb, flags | ImGuiInputTextFlags_CallbackEdit, imguiInputCb, this) ) {
        value = string(str0);
        ret = true;
    }
    ImGui::PopItemWidth();

    return ret;
}
