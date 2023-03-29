#ifndef VRIMGUIEDITOR_H_INCLUDED
#define VRIMGUIEDITOR_H_INCLUDED

#include <functional>
#include <string>
#include <map>
#include <vector>

//#include "imEditor/TextEditor.h"
#include <imgui.h>
#include <core/utils/VRFwdDeclTemplate.h>

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

struct ResizeEvent {
    ImVec2 size;
    ImVec2 pos;

    vector<char> changed();
};

typedef function<void(string, map<string, string>)> Signal;
typedef function<void(string, Surface)> ResizeSignal;

ptrFwd(ImWidget);

class ImWidget {
    public:
        string name;
        Signal signal;

        ImWidget(string n);
        virtual ~ImWidget();

        void render();
        virtual void begin() = 0;
        virtual void end();
};

class ImSection : public ImWidget {
    public:
        Rectangle layout;
        Surface surface;
        Surface parentSurface;
        ResizeEvent resizer;
        ImGuiWindowFlags flags = 0;

        ImSection(string n, Rectangle r);
        void resize(const Surface& parent);
        void updateLayout(const Surface& newSize);

        void begin() override;
        void end() override;
};

class ImAppLauncher {
    public:
        string ID;
        string name;
        bool running = false;
        bool sensitive = true;

        ImAppLauncher() {}
        ImAppLauncher(string ID);
        void render();
};

class ImAppManager : public ImWidget {
    public:
        map<string,ImAppLauncher> launchers;

        void newAppLauncher(string ID);
        void setupAppLauncher(string ID, string name);
        void setAppLauncherState(string ID, bool running, bool sensitive);

        void renderLauncher(string name);
        ImAppManager();
        void begin() override;
};

class ImToolbar : public ImSection {
    public:
        ImToolbar(Rectangle r);
        void begin() override;
};

class ImSidePanel : public ImSection {
    public:
        ImWidgetPtr appManager;

        ImSidePanel(Rectangle r);
        void begin() override;
};

class ImConsoles : public ImSection {
    public:
        ImConsoles(Rectangle r);
        void begin() override;
};

class Imgui {
    private:
        Signal signal;
        ResizeSignal resizeSignal;

        ImToolbar toolbar = ImToolbar({0,1,0.95,1});
        ImSidePanel sidePanel = ImSidePanel({0,0.3,0,0.95});
        ImConsoles consoles = ImConsoles({0.3,1.0,0,0.3});
        ImSection glArea = ImSection("glArea", {0.3,1,0.3,0.95});

        void resolveResize(const string& name, const ResizeEvent& resizer);

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
