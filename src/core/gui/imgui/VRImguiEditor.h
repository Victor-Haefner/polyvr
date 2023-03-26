#ifndef VRIMGUIEDITOR_H_INCLUDED
#define VRIMGUIEDITOR_H_INCLUDED

#include <functional>
#include <string>
#include <map>
#include <vector>

//#include "imEditor/TextEditor.h"
#include <imgui.h>

using namespace std;

struct Rectangle {
    float left = 0;
    float right = 1;
    float bottom = 0;
    float top = 1;
};

struct Surface {
    int x = 0;
    int y = 0;
    int width = 10;
    int height = 10;

    void compute(const Surface& parent, const Rectangle& area);
};

class Widget {
    public:
        string name;
        Rectangle layout;
        Surface surface;
        Surface parentSurface;

        Widget(string n, Rectangle r) : name(n), layout(r) {}

        void updateLayout(const Surface& newSize);
};

struct ResizeEvent {
    ImVec2 size;
    ImVec2 pos;

    vector<char> changed() {
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
};

typedef function<void(string, map<string, string>)> Signal;
typedef function<void(string, Surface)> ResizeSignal;

class ImWidget : public Widget {
    public:
        Signal signal;
        ResizeEvent resizer;
        ImGuiWindowFlags flags = 0;

        ImWidget(string n, Rectangle r) : Widget(n,r) {
            resize({0,0,800,800});

            flags |= ImGuiWindowFlags_NoTitleBar;
            flags |= ImGuiWindowFlags_NoScrollbar;
            //flags |= ImGuiWindowFlags_MenuBar;
            flags |= ImGuiWindowFlags_NoMove;
            //flags |= ImGuiWindowFlags_NoResize;
            flags |= ImGuiWindowFlags_NoCollapse;
            flags |= ImGuiWindowFlags_NoNav;
            //flags |= ImGuiWindowFlags_NoBackground;
            //flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
            //flags |= ImGuiWindowFlags_UnsavedDocument;
        }

        void begin() {
            //cout << " place widget " << surface << endl;
            ImGui::SetNextWindowPos(ImVec2(surface.x, surface.y)); // ImGuiCond_FirstUseEver
            ImGui::SetNextWindowSize(ImVec2(surface.width, surface.height));
            ImGui::Begin(name.c_str(), NULL, flags);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

            ImGuiIO& io = ImGui::GetIO();
            io.ConfigWindowsResizeFromEdges = true;
            io.MouseDragThreshold = 30;

            for (char edge: resizer.changed()) {
                string sedge = string( 1, edge );
                signal("widgetResize", {{"name",name},{"edge",sedge}} );
            }
        }

        void end() {
            ImGui::End();
        }

        void resize(const Surface& parent) {
            parentSurface = parent;
            surface.compute(parent, layout);
            resizer.pos = ImVec2(surface.x, surface.y);
            resizer.size = ImVec2(surface.width, surface.height);
        }
};

class Imgui {
    private:
        Signal signal;
        ResizeSignal resizeSignal;

        ImWidget toolbar = ImWidget("Toolbar", {0,1,0.95,1});
        ImWidget sidePannel = ImWidget("SidePannel", {0,0.3,0,0.95});
        ImWidget consoles = ImWidget("Consoles", {0.3,1.0,0,0.3});
        ImWidget glArea = ImWidget("glArea", {0.3,1,0.3,0.95});
        //TextEditor editor;

        void resolveResize(const string& name, const ResizeEvent& resizer);

        void renderSidePannel();
        void renderToolbar();
        void renderConsoles();

    public:
        void init(Signal signal, ResizeSignal resizeSignal);
        void close();

        void render();
        void renderGLArea();
        void resizeUI(const Surface& parent);
        void resize(const Surface& parent);
        void onWidgetResize(map<string,string> options);
};

#endif // VRIMGUIEDITOR_H_INCLUDED
