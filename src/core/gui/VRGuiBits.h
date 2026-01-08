#ifndef VRGUIBITS_H_INCLUDED
#define VRGUIBITS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string.h>
#include <queue>
#include "core/navigation/VRNavigationFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "VRGuiRecWidget.h"
#include "VRGuiFwd.h"

const char* getVersionString();

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiBits {
    private:
        bool update_ward = false;
        string terminal;
        map<string, VRConsoleWidgetPtr> consoles;

	    VRUpdateCbPtr updatePtr;
	    VRToggleCbPtr recToggleCb;

        VRGuiRecWidget recorder;
        VRVisualLayerPtr recorder_visual_layer;

        void updateVisualLayer();
        void on_view_option_toggle(string layer, bool b);
        void toggleVerbose(string s);

        void on_camera_changed(string cam);
        void on_navigation_toggled(string name, bool b);

        void on_save_clicked();
        void on_web_export_clicked();
        void on_quit_clicked();
        void on_internal_clicked();
        void on_fullscreen_clicked();
        void on_seeall_clicked();

        void on_internal_close_clicked();

        void updateWebPortRessources(bool withXR, bool withEditor, bool runBrowser);
        void on_web_cancel();
        void on_web_start();

    public:
        VRGuiBits();

        void setSceneSignal(VRSignalPtr sig);

        VRConsoleWidgetPtr getConsole(string t);
        void update_terminals();

        void toggleDock(bool b);
        void toggleFullscreen();
        void toggleWidgets();
        void toggleStereo();
        bool pressFKey(int k);

        bool update();
        void wipeConsoles();
};

OSG_END_NAMESPACE;

#endif // VRGUIBITS_H_INCLUDED
