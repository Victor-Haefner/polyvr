#ifndef VRIMGUICONSOLES_H_INCLUDED
#define VRIMGUICONSOLES_H_INCLUDED

#include "VRImguiUtils.h"
#include "imEditor/TextEditor.h"
#include "imWidgets/VRImguiCombo.h"

using namespace std;

class ImViewControls {
    public:
        ImCombo cameras;

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
        struct Attribute {
            string value;
            int c0 = 0;
            int L = 0;
        };

        struct Attributes {
            vector<Attribute> marks;
            vector<Attribute> styles;
        };

        TextEditor console;

        bool sensitive = true;
        bool tabOpen = false;
        bool paused = false;
        int changed = 0;
        string ID;
        string name;
        vector<string> lines;
        map<size_t, Attributes> attributes;
        unsigned int color = 0;

        ImConsole() {}
        ImConsole(string ID);
        void push(string data, string style, string mark);
        void render();
        void clear();
        void pause(bool b);
        void setTheme(string theme);
};

class ImConsoles : public ImWidget {
    private:
        ImViewControls viewControls;
        vector<string> consolesOrder;
        map<string,ImConsole> consoles;

        string theme = "dark";
        bool paused = false;

        void setConsolesPalette(string theme);

    public:

        void newConsole(string ID, string color);
        void clearConsole(string ID);
        void setupConsole(string ID, string name);
        void pushConsole(string ID, string data, string style, string mark);
        void setConsoleLabelColor(string ID, string color);

        ImConsoles();
        void begin() override;
};

#endif // VRIMGUICONSOLES_H_INCLUDED
