#ifndef VRDEMOS_H_INCLUDED
#define VRDEMOS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

#include <gdkmm/event.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/utils/VRName.h"

namespace Gtk {
    class Button;
    class Image;
    class Label;
    class Frame;
    class Table;
}

class VRGuiContextMenu;

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRAppLauncher);
ptrFwd(VRAppSection);
ptrFwd(VRAppManager);

class VRAppLauncher {
    public:
        string path;
        string pxm_path;
        string table;
        Gtk::Frame* widget = 0;
        Gtk::Button* butPlay = 0;
        Gtk::Button* butOpts = 0;
        Gtk::Button* butLock = 0;
        Gtk::Label* label = 0;
        Gtk::Image* imgScene = 0;
        Gtk::Image* imgPlay = 0;
        Gtk::Image* imgLock = 0;
        Gtk::Image* imgUnlock = 0;
        Gtk::Image* imgOpts = 0;
        bool running = false;
        bool pixmap = false;
        bool favorite = true;
        bool write_protected = false;
        VRDeviceCbPtr uPixmap;
        VRAppSectionWeakPtr section;

    public:
        VRAppLauncher(VRAppSectionPtr s);
        ~VRAppLauncher();
        static VRAppLauncherPtr create(VRAppSectionPtr s);

        void updatePixmap();
};

class VRAppSection : public std::enable_shared_from_this<VRAppSection>, public VRName {
    private:
        map<string, VRAppLauncherPtr> apps;

        void setButton(VRAppLauncherPtr e, VRGuiContextMenu* menu, VRAppManager* mgr);

    public:
        VRAppSection(string name);
        ~VRAppSection();
        static VRAppSectionPtr create(string name);
        VRAppSectionPtr ptr();

        VRAppLauncherPtr addLauncher(string path, VRGuiContextMenu* menu, VRAppManager* mgr);
        void remLauncher(string path);
        VRAppLauncherPtr getLauncher(string path);
        int getSize();

        void fillTable(string t, Gtk::Table* tab, int& i);
        void clearTable(string t, Gtk::Table* tab);
        void setGuiState(VRAppLauncherPtr e, bool running);
};

class VRAppManager {
    private:
        string active;
        VRScene* demo = 0;
        VRSignalPtr on_scene_loaded = 0;
        VRSignalPtr on_scene_closing = 0;
        VRAppLauncherPtr current_demo = 0;
        VRAppSectionPtr examplesSection;
        VRAppSectionPtr favoritesSection;
        VRAppSectionPtr recentsSection;
        VRGuiContextMenu* menu;
        VRDeviceCbPtr updateCb;

        void clearTable(string t);
        void updateTable(string t);

        void setGuiState(VRAppLauncherPtr e);
        VRAppLauncherPtr addEntry(string path, string table, bool running);

        void updatePixmap(VRAppLauncherPtr e, Gtk::Image* img_pxb, int w, int h);
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
        void on_load_clicked();

    public:
        VRAppManager();
        ~VRAppManager();
        static VRAppManagerPtr create();

        void toggleDemo(VRAppLauncherPtr e);
        void on_lock_toggle(VRAppLauncherPtr e);
        void on_menu_advanced(VRAppLauncherPtr e);
        bool on_any_event(GdkEvent* event, VRAppLauncherPtr entry);
};

OSG_END_NAMESPACE;

#endif // VRDEMOS_H_INCLUDED
