#ifndef VRDEMOS_H_INCLUDED
#define VRDEMOS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/utils/VRName.h"

class VRGuiContextMenu;
struct _GtkImage;
struct _GtkWidget;
struct _GtkCheckButton;
union _GdkEvent;

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
        map<string, _GtkWidget*> tables;
        VRGuiContextMenu* menu;
        VRDeviceCbPtr updateCb;
        bool noLauncherScene = false;

        void clearTable(string t);
        void updateTable(string t);

        void setGuiState(VRAppLauncherPtr e);
        VRAppLauncherPtr addEntry(string path, string table, bool running, string timestamp = "", bool recent = false);

        void updatePixmap(VRAppLauncherPtr e, _GtkImage* img_pxb, int w, int h);
        void update();

        void writeGitignore(string path);
        void normFileName(string& f);

        void initMenu();
        void on_menu_delete();
        void on_menu_unpin();

        void on_advanced_cancel();
        void on_advanced_start();

        void on_diag_new_clicked();
        void on_diag_save_clicked();
        void on_diag_load_clicked();
        void on_new_clicked();
        void on_saveas_clicked();
        void on_stop_clicked();
        void on_load_clicked();

        void on_toggle_encryption(_GtkCheckButton* b);

        void on_search();

    public:
        VRAppManager();
        ~VRAppManager();
        static VRAppManagerPtr create();

        VRAppPanelPtr addSection(string name, string t);
        void toggleDemo(VRAppLauncherPtr e);
        void on_lock_toggle(VRAppLauncherPtr e);
        void on_menu_advanced(VRAppLauncherPtr e);
        bool on_any_event(_GdkEvent* event, VRAppLauncherPtr entry);
};

OSG_END_NAMESPACE;

#endif // VRDEMOS_H_INCLUDED
