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

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiBits {
    private:
        VRConsoleWidgetPtr openConsole;
        Gtk::Notebook* terminal;
        map<string, VRConsoleWidgetPtr> consoles;

	    VRUpdateCbPtr updatePtr;
	    VRToggleCbPtr recToggleCb;

        VRGuiRecWidget recorder;
        VRVisualLayerPtr recorder_visual_layer;

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

        VRConsoleWidgetPtr getConsole(string t);
        void update_terminals();

        void toggleDock();
        bool toggleFullscreen(GdkEventKey* k);
        bool toggleWidgets(GdkEventKey* k);
        bool toggleStereo(GdkEventKey* k);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUIBITS_H_INCLUDED
