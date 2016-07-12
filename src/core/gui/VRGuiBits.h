#ifndef VRGUIBITS_H_INCLUDED
#define VRGUIBITS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <queue>
#include <gtkmm/combobox.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "VRGuiRecWidget.h"

namespace Gtk { class ToggleToolButton; class ScrolledWindow; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRVisualLayer;
class VRSignal;

class VRGuiBits {
    private:
        GtkWidget* term_box = 0;
        Gtk::ScrolledWindow* swin = 0;
	    std::queue<string> msg_queue;

	    shared_ptr<VRFunction<int> > updatePtr;
	    shared_ptr<VRFunction<bool> > recToggleCb;

        VRGuiRecWidget recorder;
        shared_ptr<VRVisualLayer> recorder_visual_layer;

        void hideAbout(int i);
        void updateVisualLayer();
        void on_view_option_toggle(VRVisualLayer* l, Gtk::ToggleToolButton* tb);
        void toggleVerbose(string s);
        void on_terminal_changed();

        void on_camera_changed();
        void on_navigation_changed();

        void on_save_clicked();
        void on_quit_clicked();
        void on_about_clicked();
        void on_internal_clicked();

        void on_new_cancel_clicked();
        void on_internal_close_clicked();

    public:
        VRGuiBits();

        void setSceneSignal(VRSignalPtr sig);

        void write_to_terminal(string s);
        void update_terminal();
        void clear_terminal();

        void toggleDock();
        bool toggleFullscreen(GdkEventKey* k);
        bool toggleWidgets(GdkEventKey* k);
        bool toggleStereo(GdkEventKey* k);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUIBITS_H_INCLUDED
