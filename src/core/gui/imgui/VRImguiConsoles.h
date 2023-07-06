#ifndef VRIMGUICONSOLES_H_INCLUDED
#define VRIMGUICONSOLES_H_INCLUDED

#include "VRImguiUtils.h"

using namespace std;

class ImViewControls {
    public:
        vector<string> cameras;
        int current_camera = 0;

        map<string, bool> navigations;

        bool showCams = false;
        bool showLights = false;
        bool pauseRendering = false;
        bool showPhysics = false;
        bool recorder = false;
        bool showCoordinates = false;
        bool showSetup = false;
        bool showStats = false;
        bool showStencil = false;

        ImViewControls();
        void render();
};

class ImConsole {
    public:
        bool sensitive = true;
        int changed = 2;
        string ID;
        string name;
        vector<string> lines;
        unsigned int color = 0;

        ImConsole() {}
        ImConsole(string ID);
        void render();
};

class ImConsoles : public ImWidget {
    public:
        ImViewControls viewControls;
        vector<string> consolesOrder;
        map<string,ImConsole> consoles;

        void newConsole(string ID);
        void clearConsole(string ID);
        void setupConsole(string ID, string name);
        void pushConsole(string ID, string data, string style, string mark);
        void setConsoleLabelColor(string ID, string color);

        ImConsoles();
        void begin() override;
};

#endif // VRIMGUICONSOLES_H_INCLUDED
