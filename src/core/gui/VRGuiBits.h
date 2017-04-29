#ifndef VRGUIBITS_H_INCLUDED
#define VRGUIBITS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <queue>
#include <gtkmm/combobox.h>
#include <gtkmm/textbuffer.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "VRGuiRecWidget.h"
#include "VRGuiFwd.h"

namespace Gtk {
    class ToggleToolButton;
    class ScrolledWindow;
    class Notebook;
    class Label;
}

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRVisualLayer;
class VRSignal;

struct VRConsoleWidget {
    Glib::RefPtr<Gtk::TextBuffer> buffer;
    Gtk::ScrolledWindow* swin = 0;
    Gtk::Label* label = 0;
    std::queue<string> msg_queue;
    bool paused = 0;
    bool isOpen = 0;

    string notifyColor = "#006fe0";

    void forward();
    void write(string s);
    void update();

    VRConsoleWidget();

    void queue(string s);
    void clear();
    void pause();
    void setOpen(bool b);
    void setLabel(Gtk::Label* lbl);
    void setColor(string color);
    void configColor(string color);
    void resetColor();
};

class VRGuiBits {
    private:
        VRConsoleWidgetPtr openConsole;
        Gtk::Notebook* terminal;
        map<string, VRConsoleWidgetPtr> consoles;

	    shared_ptr<VRFunction<int> > updatePtr;
	    shared_ptr<VRFunction<bool> > recToggleCb;

        VRGuiRecWidget recorder;
        shared_ptr<VRVisualLayer> recorder_visual_layer;

        void hideAbout(int i);
        void updateVisualLayer();
        void on_view_option_toggle(VRVisualLayer* l, Gtk::ToggleToolButton* tb);
        void toggleVerbose(string s);

        void on_camera_changed();
        void on_navigation_changed();

        void on_save_clicked();
        void on_quit_clicked();
        void on_about_clicked();
        void on_internal_clicked();

        void on_new_cancel_clicked();
        void on_internal_close_clicked();
        void on_console_switch(GtkNotebookPage* page, unsigned int page_num);

    public:
        VRGuiBits();

        void setSceneSignal(VRSignalPtr sig);

        void write_to_terminal(string t, string s);
        void update_terminals();

        void toggleDock();
        bool toggleFullscreen(GdkEventKey* k);
        bool toggleWidgets(GdkEventKey* k);
        bool toggleStereo(GdkEventKey* k);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUIBITS_H_INCLUDED
