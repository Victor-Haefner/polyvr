
#include "VRImguiUtils.h"
#include <iostream>
#include <math.h>

#include "core/utils/system/VRSystem.h"
#include "core/tools/VRProjectManager.h"


ostream& operator<<(ostream& os, const ResizeEvent& s) {
    os << "[" << s.pos.x << ", " << s.pos.y << ", " << s.size.x << ", " << s.size.y << "]";
    return os;
}

ostream& operator<<(ostream& os, const ImVec2& s) {
    os << "[" << s.x << ", " << s.y << "]";
    return os;
}

ostream& operator<<(ostream& os, const ImRectangle& s) {
    os << "[" << s.left << ", " << s.right << ", " << s.top << ", " << s.bottom << "]";
    return os;
}

ostream& operator<<(ostream& os, const Surface& s) {
    os << "[" << s.x << ", " << s.y << ", " << s.width << ", " << s.height << "]";
    return os;
}

void Surface::compute(const Surface& parent, const ImRectangle& area) {
    width  = round( parent.width * (area.right - area.left) );
    height = round( parent.height * (area.top - area.bottom) );
    width = max(width, 10);
    height = max(height, 10);
    x = round( parent.width * area.left );
    y = round( parent.height * (1.0 - area.top) );
    //cout << " compute surface " << width << ", " << height << ", " << x << ", " << y << endl;
}

vector<char> ResizeEvent::changed() {
    vector<char> edges;
    ImVec2 s = ImGui::GetWindowSize();
    ImVec2 p = ImGui::GetWindowPos();
    if (s.x == size.x && s.y == size.y) return edges;

    if (p.x != pos.x && s.x != size.x) edges.push_back('L');
    if (p.x == pos.x && s.x != size.x) edges.push_back('R');
    if (p.y != pos.y && s.y != size.y) edges.push_back('T');
    if (p.y == pos.y && s.y != size.y) edges.push_back('B');

    size = s;
    pos = p;
    return edges;
}


ImWidget::ImWidget(string n) : name(n) {}
ImWidget::~ImWidget() {}
void ImWidget::end() {}

void ImWidget::render() {
    begin();
    end();
}

ImVec4 colorFromString(const string& c) {
    auto conv = [](char c1, char c2) {
        if (c1 >= 'A' && c1 <= 'F') c1 -= ('A'-'a');
        if (c2 >= 'A' && c2 <= 'F') c2 -= ('A'-'a');
        int C1 = c1-'0';
        int C2 = c2-'0';
        if (c1 >= 'a' && c1 <= 'f') C1 = (c1-'a')+10;
        if (c2 >= 'a' && c2 <= 'f') C2 = (c2-'a')+10;
        return (C1*16+C2)/256.0;
    };

    if (c[0] == '#') {
        if (c.size() == 4) return ImVec4(conv(c[1],'f'), conv(c[2],'f'), conv(c[3],'f'), 255);
        if (c.size() == 5) return ImVec4(conv(c[1],'f'), conv(c[2],'f'), conv(c[3],'f'), conv(c[4],'f'));
        if (c.size() == 7) return ImVec4(conv(c[1],c[2]), conv(c[3],c[4]), conv(c[5],c[6]), 255);
        if (c.size() == 9) return ImVec4(conv(c[1],c[2]), conv(c[3],c[4]), conv(c[5],c[6]), conv(c[7],c[8]));
    }
    return ImVec4(255,255,255,255);
}

OSG::VRProjectManagerPtr uiParameterStore;

void uiInitStore() {
    uiParameterStore = OSG::VRProjectManager::create();
    uiParameterStore->newProject(absolute(".uiParameter.ini"), true);
    uiParameterStore->load();
}

void uiStoreParameter(string name, string value) {
    uiParameterStore->setSetting(name, value);
}

string uiGetParameter(string name, string def) {
    return uiParameterStore->getSetting(name, def);
}


