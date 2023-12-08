#include "VRImguiVector.h"
#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

#include <iostream>
using namespace std;

int ivFlags = 0;

Im_Vector::Im_Vector(string ID, string label, int flags) :  ID("##"+ID), label(label), flags(flags),
                                                            iX(ID+"X","","0",ivFlags),
                                                            iY(ID+"Y","","0",ivFlags),
                                                            iZ(ID+"Z","","0",ivFlags),
                                                            iW(ID+"W","","0",ivFlags) {}

void Im_Vector::set2(double x, double y) { vX = x; vY = y; iX.value = toString(x); iY.value = toString(y); Nfields = 2; }
void Im_Vector::set3(double x, double y, double z) { vX = x; vY = y; vZ = z; iX.value = toString(x); iY.value = toString(y); iZ.value = toString(z); Nfields = 3; }
void Im_Vector::set4(double x, double y, double z, double w) { vX = x; vY = y; vZ = z; vW = w; iX.value = toString(x); iY.value = toString(y); iZ.value = toString(z); iW.value = toString(w); Nfields = 4; }

void Im_Vector::set2(string s) { vector<float> v; toValue(s, v); set2(v[0], v[1]); }
void Im_Vector::set3(string s) { vector<float> v; toValue(s, v); set3(v[0], v[1], v[2]); }
void Im_Vector::set4(string s) { vector<float> v; toValue(s, v); set4(v[0], v[1], v[2], v[3]); }

void Im_Vector::signal(string sig) {
    if (Nfields == 2) uiSignal( sig, {{"x",iX.value}, {"y",iY.value}});
    if (Nfields == 3) uiSignal( sig, {{"x",iX.value}, {"y",iY.value}, {"z",iZ.value}});
    if (Nfields == 4) uiSignal( sig, {{"x",iX.value}, {"y",iY.value}, {"z",iZ.value}, {"w",iW.value}});
}

bool Im_Vector::render(int width) {
    int Nparts = Nfields;
    if (label != "") Nparts += 1;
    int wPart = 0.8*width / Nparts;

    if (label != "") {
        string bID = "box" + ID;
        ImGui::BeginChild(bID.c_str(), ImVec2(wPart, ImGui::GetTextLineHeightWithSpacing()));
            ImGui::Text(label.c_str());
        ImGui::EndChild();
        ImGui::SameLine();
    }

    bool ret = false;
    if (iX.render(wPart)) ret = true;

    if (Nfields >= 2) {
        ImGui::SameLine();
        if (iY.render(wPart)) ret = true;
    }

    if (Nfields >= 3) {
        ImGui::SameLine();
        if (iZ.render(wPart)) ret = true;
    }

    if (Nfields >= 4) {
        ImGui::SameLine();
        if (iW.render(wPart)) ret = true;
    }

    if (ret) {
        vX = toFloat(iX.value);
        if (Nfields >= 2) vY = toFloat(iY.value);
        if (Nfields >= 3) vZ = toFloat(iZ.value);
        if (Nfields >= 4) vW = toFloat(iW.value);
    }

    return ret;
}
