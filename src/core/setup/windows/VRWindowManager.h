#ifndef VRWINDOWMANAGER_H_INCLUDED
#define VRWINDOWMANAGER_H_INCLUDED

#include <OpenSG/OSGRenderAction.h>
#include "core/setup/VRSetupFwd.h"
#include "core/utils/VRChangeList.h"
#include "core/utils/VRUtilsFwd.h"

namespace xmlpp{ class Element; }
namespace Gtk{ class Window; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRWindow;

class VRWindowManager {
    private:
        map<string, VRWindowPtr> windows;
        VRGtkWindowPtr editorWindow;
        RenderActionRefPtr ract;
        VRChangeList changeListStats;
        bool rendering_paused = false;

        bool checkWin(string name);

    public:
        VRWindowManager();
        ~VRWindowManager();

        map<string, VRWindowPtr> getWindows();
        VRWindowPtr getWindow(string name);

        VRWindowPtr addGlutWindow  (string name);
        VRWindowPtr addGtkWindow   (string name, string glarea = "glarea", string msaa = "x4");
        VRWindowPtr addMultiWindow (string name);
        void removeWindow   (string name);

        void setWindowView(string name, VRViewPtr view);
        void addWindowServer(string name, string server);
        void changeWindowName(string& name, string new_name);

        void getWindowSize(string name, int& w, int& h);
        void resizeWindow(string name, int w, int h);

        void pauseRendering(bool b);
        RenderActionRefPtr getRenderAction();
        void updateWindows();
        void stopWindows();

        VRGtkWindowPtr getEditorWindow();

        void save(XMLElementPtr node);
        void load(XMLElementPtr node);

        static bool doRenderSync;
};

OSG_END_NAMESPACE;

#endif // VRWINDOWMANAGER_H_INCLUDED
