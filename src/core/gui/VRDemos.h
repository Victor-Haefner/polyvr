#ifndef VRDEMOS_H_INCLUDED
#define VRDEMOS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

#include <gdkmm/event.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"

namespace Gtk {
    class Button;
    class Image;
    class Label;
    class Frame;
}

class VRGuiContextMenu;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;
class VRSignal;

struct demoEntry {
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
};

ptrFwd(demoEntry);

class VRDemos {
    private:
        string active;
        VRScene* demo = 0;
        VRSignalPtr on_scene_loaded = 0;
        VRSignalPtr on_scene_closing = 0;
        demoEntryPtr current_demo = 0;
        map<string, demoEntryPtr> demos;
        VRGuiContextMenu* menu;
        VRDeviceCbPtr updateCb;

        bool on_any_event(GdkEvent* event, demoEntryPtr entry);
        Gtk::Image* loadGTKIcon(Gtk::Image* img, string path, int w, int h);
        void setButton(demoEntryPtr e);

        void clearTable(string t);
        void updateTable(string t);

        void setGuiState(demoEntryPtr e);
        void toggleDemo(demoEntryPtr e);
        void addEntry(string path, string table, bool running);

        void updatePixmap(demoEntryPtr e, Gtk::Image* img_pxb, int w, int h);
        void update();

        void writeGitignore(string path);
        string getFolderName(string path);
        string getFileName(string path);
        void normFileName(string& f);

        void initMenu();
        void on_menu_delete();
        void on_menu_advanced(demoEntryPtr e = 0);
        void on_menu_unpin();

        void on_advanced_cancel();
        void on_advanced_start();

        void on_lock_toggle(demoEntryPtr e);

        void on_diag_new_clicked();
        void on_diag_save_clicked();
        void on_diag_load_clicked();
        void on_new_clicked();
        void on_saveas_clicked();
        void on_load_clicked();

    public:
        VRDemos();

        //VRSignalPtr getSignal();
};

OSG_END_NAMESPACE;

#endif // VRDEMOS_H_INCLUDED
