#ifndef VRGUIMANAGER_H_INCLUDED
#define VRGUIMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <map>
#include "core/utils/VRFunctionFwd.h"
#include "core/scripting/VRScriptFwd.h"
#include "VRGuiFwd.h"

namespace boost { class recursive_mutex; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiManager {
    private:
        bool standalone = false;
	    VRUpdateCbPtr updatePtr;
        VRThreadCbPtr gtkUpdateCb;
        int gtkUpdateThreadID = -1;
	    vector<VRDeviceCbPtr> guiSignalCbs;
        boost::recursive_mutex* mtx = 0;

	    map<Gtk::Window*, Gtk::WindowPtr> windows;

        VRGuiManager();

        void update();

    public:
        static VRGuiManager* get(bool init = true);
        ~VRGuiManager();

        static void broadcast(string sig);

        VRConsoleWidgetPtr getConsole(string t);
        void focusScript(string name, int line, int column);
        void getScriptFocus(VRScriptPtr& script, int& line, int& column);
        void updateGtk();
        void updateGtkThreaded(VRThreadWeakPtr t);
        void startThreadedUpdate();
        void wakeWindow();

        Gtk::WindowPtr newWindow();
        void remWindow(Gtk::WindowPtr w);

        boost::recursive_mutex& guiMutex();
};

OSG_END_NAMESPACE;

#endif // VRGUIMANAGER_H_INCLUDED
