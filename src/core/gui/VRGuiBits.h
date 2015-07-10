#ifndef VRGUIBITS_H_INCLUDED
#define VRGUIBITS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <queue>
#include <gtkmm/combobox.h>
#include <boost/thread.hpp>
#include "core/setup/devices/VRSignal.h"

namespace Gtk { class ToggleToolButton; class ScrolledWindow; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRVisualLayer;
class VRRecorder;

class VRGuiBits {
    private:
        GtkWidget* term_box = 0;
        Gtk::ScrolledWindow* swin = 0;
	    std::queue<string> msg_queue;
	    mutable boost::mutex msg_mutex;

        VRRecorder* recorder = 0;
        VRVisualLayer* recorder_visual_layer = 0;

        void hideAbout(int i);
        void updateVisualLayer();
        void on_view_option_toggle(VRVisualLayer* l, Gtk::ToggleToolButton* tb);
        void toggleVerbose(string s);
        void on_terminal_changed();

    public:
        VRGuiBits();

        void setSceneSignal(VRSignal* sig);

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
