#ifndef VRGUIMANAGER_H_INCLUDED
#define VRGUIMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/scripting/VRScriptFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "VRGuiFwd.h"
#include "VRGuiSignals.h"

namespace boost { class recursive_mutex; }

OSG_BEGIN_NAMESPACE;
using namespace std;

void saveScene(string path = "", bool saveas = false, string encryptionKey = "");

class VRGuiManager {
    private:
        bool headless = false;
        bool standalone = false;
	    VRUpdateCbPtr updatePtr;
        VRThreadCbPtr gtkUpdateCb;
        int gtkUpdateThreadID = -1;
        VRMutex* mtx = 0;
	    vector<VRDeviceCbPtr> guiSignalCbs;

        VRGuiManager();
        void update();
        void onWindowClose();

    public:
        static VRGuiManager* get(bool init = true);
        ~VRGuiManager();

        void init();
        void initImgui();
        void initImguiPopup();

        static string genUUID();

        static void broadcast(string name);
        static bool trigger(string name, VRGuiSignals::Options options = {});
        static bool triggerResize(string name, int,int,int,int);

        VRConsoleWidgetPtr getConsole(string t);
        void focusScript(string name, int line, int column);
        void getScriptFocus(VRScriptPtr& script, int& line, int& column);
        void updateGtk();
        void updateGtkThreaded(VRThreadWeakPtr t);
        void wakeWindow();

        void openHelp(string search = "");
        void updateSystemInfo();

        void selectObject(VRObjectPtr obj);

        /*_GtkWindow* newWindow();
        void remWindow(_GtkWindow* w);*/

        void setWindowTitle(string title);

        VRMutex& guiMutex();
};

OSG_END_NAMESPACE;

#endif // VRGUIMANAGER_H_INCLUDED
