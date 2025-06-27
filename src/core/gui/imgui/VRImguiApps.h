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
        string previewPath;
        ImImage miniPreview;
        bool running = false;
        bool sensitive = true;
        bool previewOK = false;

        ImAppLauncher() {}
        ImAppLauncher(string ID, string panel, string timestamp);
        ~ImAppLauncher();
        void render(string filter, ImImage& preview, int fullWidth, int colWidth1, int pathLabelN);
};

class ImAppPanel {
    public:
        string label;
        vector<string> launchers;

        ImAppPanel(string label);
        ~ImAppPanel();
        void render(string filter, map<string, ImAppLauncher>& launcherPool, ImImage& preview);
};

class ImAppManager : public ImWidget {
    public:
        string filter = "";
        ImImage preview;

        map<string, ImAppLauncher> launchers;

        vector<ImAppPanel> projects;
        ImAppPanel examples;

        void updatePannels();
        void newAppLauncher(string pannel, string ID, string timestamp);
        void setupAppLauncher(string ID, string name, string previewPath);
        void setAppLauncherState(string ID, bool running, bool sensitive);
        void updateAppLauncherPixmap(string ID, string path);

        void renderLauncher(string name);
        ImAppManager();
        ~ImAppManager();
        void begin() override;
};

#endif // VRIMGUIAPPS_H_INCLUDED
