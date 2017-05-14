#ifndef VRGUIMANAGER_H_INCLUDED
#define VRGUIMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <map>
#include "core/utils/VRFunctionFwd.h"
#include "VRGuiFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiManager {
    private:
        bool standalone = false;
	    VRUpdateCbPtr updatePtr;
	    vector<VRDeviceCbPtr> guiSignalCbs;

	    map<Gtk::Window*, Gtk::WindowPtr> windows;

        VRGuiManager();

        void update();

    public:
        static VRGuiManager* get(bool init = true);
        ~VRGuiManager();

        static void broadcast(string sig);

        VRConsoleWidgetPtr getConsole(string t);
        void focusScript(string name, int line, int column);
        void updateGtk();
        void wakeWindow();

        Gtk::WindowPtr newWindow();
        void remWindow(Gtk::WindowPtr w);
};

OSG_END_NAMESPACE;

#endif // VRGUIMANAGER_H_INCLUDED
