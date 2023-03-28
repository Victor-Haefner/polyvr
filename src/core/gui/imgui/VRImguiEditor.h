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

    vector<char> changed();
};

typedef function<void(string, map<string, string>)> Signal;
typedef function<void(string, Surface)> ResizeSignal;

class ImWidget : public Widget {
    public:
        Signal signal;
        ResizeEvent resizer;
        ImGuiWindowFlags flags = 0;

        ImWidget(string n, Rectangle r);

        void begin();
        void end();
        void resize(const Surface& parent);
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
        void onSectionResize(map<string,string> options);
};

#endif // VRIMGUIEDITOR_H_INCLUDED
