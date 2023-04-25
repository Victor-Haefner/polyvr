#ifndef VRDEMOS_H_INCLUDED
#define VRDEMOS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/utils/VRName.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRAppPanel);
ptrFwd(VRAppLauncher);
ptrFwd(VRAppManager);

class VRAppManager {
    private:
        string active;
        VRScene* demo = 0;
        VRSignalPtr on_scene_loaded = 0;
        VRSignalPtr on_scene_closing = 0;
        VRAppLauncherPtr current_demo = 0;
        map<string, VRAppPanelPtr> sections;
        VRDeviceCbPtr updateCb;
        bool noLauncherScene = false;

        void clearTable(string t);
        void updateTable(string t);

        void setGuiState(VRAppLauncherPtr e);
        void setCurrentGuiState(bool b);
        VRAppLauncherPtr addEntry(string path, string table, bool running, string timestamp = "", bool recent = false);
        VRAppLauncherPtr getEntry(string path);

        //void updatePixmap(VRAppLauncherPtr e, _GtkImage* img_pxb, int w, int h);
        bool update();

        void writeGitignore(string path);
        void normFileName(string& f);

        void on_launcher_delete();
        void on_launcher_unpin();
        void on_advanced_start();

        void on_diag_new_clicked(string path);
        void on_diag_save_clicked(string path);
        void on_diag_load_clicked(string path);
        void on_stop_clicked();

        void on_toggle_encryption(bool b);
        void on_search();

    public:
        VRAppManager();
        ~VRAppManager();
        static VRAppManagerPtr create();

        VRAppPanelPtr addSection(string name);
        void toggleDemo(VRAppLauncherPtr e);
        void on_lock_toggle(VRAppLauncherPtr e);
        void setCurrentApp(VRAppLauncherPtr e);
};

OSG_END_NAMESPACE;

#endif // VRDEMOS_H_INCLUDED
