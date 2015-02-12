#ifndef VRGUIBITS_H_INCLUDED
#define VRGUIBITS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <gtkmm/combobox.h>
#include "core/setup/devices/VRSignal.h"

namespace Gtk { class ToggleToolButton; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRVisualLayer;

class VRGuiBits {
    private:
        GtkWidget* term_box;

        void hideAbout(int i);
        void updateVisualLayer();
        void on_view_option_toggle(VRVisualLayer* l, Gtk::ToggleToolButton* tb);

    public:
        VRGuiBits();

        void setSceneSignal(VRSignal* sig);

        static void write_to_terminal(string s);

        void toggleDock();
        bool toggleFullscreen(GdkEventKey* k);
        bool toggleWidgets(GdkEventKey* k);
        bool toggleStereo(GdkEventKey* k);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUIBITS_H_INCLUDED
