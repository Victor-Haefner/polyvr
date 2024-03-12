
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

#include "core/utils/xml.h"

map<string, string> uiParameterStore;
string uiParameterFile;

void uiInitStore() {
    uiParameterFile = absolute(".uiParameter.ini");
    OSG::XML xml;
    xml.read(uiParameterFile, false);
    OSG::XMLElementPtr root = xml.getRoot();

    auto params = root->getChild("params");
    if (!params) return;
    for (auto el : params->getChildren()) {
        string name = el->getName();
        uiParameterStore[name] = el->getText();
    }
}

void uiStoreParameter(string name, string value) {
    uiParameterStore[name] = value;

    OSG::XML xml;
    OSG::XMLElementPtr root = xml.newRoot("Project", "", ""); // name, ns_uri, ns_prefix
    OSG::XMLElementPtr params = root->addChild("params");
    for (auto t : uiParameterStore) {
        auto e2 = params->addChild(t.first);
        e2->setText(t.second);
    }
    xml.write(uiParameterFile);
}

void uiCloseStore() {
    uiParameterStore.clear();
}

string uiGetParameter(string name, string def) {
    if (uiParameterStore.count(name)) return uiParameterStore[name];
    else return def;
}

float strWidth(const string& s) { // TODO
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();

    float p = style.FramePadding.x * 2.0f;
    //return ImGui::CalcTextSize(s.c_str()).x + p; // CalcTextSize may crash when starting maximized
    return s.size()*6.5*io.FontGlobalScale + p;
}

static bool borderGlowActive = false;
static map<int, int> borderGlowTickers;

void pushGlowBorderStyle(int ID) {
    if (borderGlowActive) return;

    const int N = 120;
    if (!borderGlowTickers.count(ID)) borderGlowTickers[ID] = 0;
    borderGlowTickers[ID] += 1;
    borderGlowTickers[ID] %= N;

    float t = 0.5 + abs(0.5 - borderGlowTickers[ID]/float(N)); // 0.5 -> 1 -> 0.5
    ImVec4 col(1.0*t, 0.7*t, 0.2*t, 1.0);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, col);
    ImGui::PushStyleColor(ImGuiCol_Border, col);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0);
    borderGlowActive = true;
}

void popGlowBorderStyle() {
    if (!borderGlowActive) return;
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(2);
    borderGlowActive = false;
}
