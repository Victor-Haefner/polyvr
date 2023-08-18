#include "VRImguiVector.h"
#include "core/utils/toString.h"

#include <iostream>
using namespace std;

int ivFlags = 0;

Im_Vector::Im_Vector(string ID, string label, int flags) :  ID("##"+ID), label(label), flags(flags),
                                                            iX(ID+"X","","0",ivFlags),
                                                            iY(ID+"Y","","0",ivFlags),
                                                            iZ(ID+"Z","","0",ivFlags) {}

void Im_Vector::set2(double x, double y) { vX = x; vY = y; iX.value = toString(x); iY.value = toString(y); Nfields = 2; }
void Im_Vector::set3(double x, double y, double z) { vX = x; vY = y; vZ = z; iX.value = toString(x); iY.value = toString(y); iZ.value = toString(z); Nfields = 3; }

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

    if (ret) {
        vX = toFloat(iX.value);
        vY = toFloat(iY.value);
        vZ = toFloat(iZ.value);
    }

    return ret;
}
