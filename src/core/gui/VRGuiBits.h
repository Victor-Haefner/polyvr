#ifndef VRGUIBITS_H_INCLUDED
#define VRGUIBITS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <queue>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "VRGuiRecWidget.h"
#include "VRGuiFwd.h"

struct _GtkNotebook;
struct _GtkToggleToolButton;
struct _GtkWidget;
struct _GdkEventKey;

const char* getVersionString();

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiBits {
    private:
        bool update_ward = false;
        VRConsoleWidgetPtr openConsole;
        _GtkNotebook* terminal;
        map<string, VRConsoleWidgetPtr> consoles;

	    VRUpdateCbPtr updatePtr;
	    VRToggleCbPtr recToggleCb;

        VRGuiRecWidget recorder;
        VRVisualLayerPtr recorder_visual_layer;

        void hideAbout(int i);
        void updateVisualLayer();
        void on_view_option_toggle(VRVisualLayer* l, _GtkToggleToolButton* tb);
        void toggleVerbose(string s);

        void on_camera_changed();
        void on_navigation_changed();

        void on_save_clicked();
        void on_web_export_clicked();
        void on_quit_clicked();
        void on_about_clicked();
        void on_internal_clicked();
        void on_fullscreen_clicked();

        void on_internal_close_clicked();
        void on_console_switch(_GtkWidget* page, unsigned int page_num);

    public:
        VRGuiBits();

        void setSceneSignal(VRSignalPtr sig);

        VRConsoleWidgetPtr getConsole(string t);
        void update_terminals();

        void toggleDock();
        void toggleFullscreen();
        void toggleWidgets();
        void toggleStereo();
        bool pressFKey(_GdkEventKey* k);

        void update();
        void wipeConsoles();
};

OSG_END_NAMESPACE;

#endif // VRGUIBITS_H_INCLUDED
