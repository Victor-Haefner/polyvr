#ifndef VRIMGUIAPPS_H_INCLUDED
#define VRIMGUIAPPS_H_INCLUDED

#include "VRImguiUtils.h"

using namespace std;

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

class ImAppPanel {
    public:
        map<string,ImAppLauncher> launchers;

        ImAppPanel() {}
        void render();
};

class ImAppManager : public ImWidget {
    public:
        ImAppPanel recents;
        ImAppPanel projects;
        ImAppPanel examples;

        void newAppLauncher(string pannel, string ID);
        void setupAppLauncher(string ID, string name);
        void setAppLauncherState(string ID, bool running, bool sensitive);

        void renderLauncher(string name);
        ImAppManager();
        void begin() override;
};

#endif // VRIMGUIAPPS_H_INCLUDED
