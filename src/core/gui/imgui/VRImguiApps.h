#ifndef VRIMGUIAPPS_H_INCLUDED
#define VRIMGUIAPPS_H_INCLUDED

#include "VRImguiUtils.h"

using namespace std;

class ImAppLauncher {
    public:
        string ID;
        string name;
        string panel;
        string timestamp;
        bool running = false;
        bool sensitive = true;

        ImAppLauncher() {}
        ImAppLauncher(string ID, string panel, string timestamp);
        void render(string filter);
};

class ImAppPanel {
    public:
        string label;
        vector<string> launchers;

        ImAppPanel(string label);
        void render(string filter, map<string, ImAppLauncher>& launcherPool);
};

class ImAppManager : public ImWidget {
    public:
        string filter = "";

        map<string, ImAppLauncher> launchers;

        vector<ImAppPanel> projects;
        ImAppPanel examples;

        void updatePannels();
        void newAppLauncher(string pannel, string ID, string timestamp);
        void setupAppLauncher(string ID, string name);
        void setAppLauncherState(string ID, bool running, bool sensitive);

        void renderLauncher(string name);
        ImAppManager();
        void begin() override;
};

#endif // VRIMGUIAPPS_H_INCLUDED
