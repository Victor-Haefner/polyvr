#ifndef VRWINDOWMANAGER_H_INCLUDED
#define VRWINDOWMANAGER_H_INCLUDED

#include <OpenSG/OSGRenderAction.h>

namespace xmlpp{ class Element; }
namespace Gtk{ class Window; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRWindow;
class VRView;

class VRWindowManager {
    private:
        map<string, VRWindow*> windows;
        RenderActionRefPtr ract;
        bool rendering_paused = false;

        bool checkWin(string name);

    public:
        VRWindowManager();
        ~VRWindowManager();

        map<string, VRWindow*> getWindows();
        VRWindow* getWindow(string name);

        void initGlut();

        VRWindow* addGlutWindow  (string name);
        VRWindow* addGtkWindow   (string name, string glarea = "glarea");
        VRWindow* addMultiWindow (string name);
        void removeWindow   (string name);

        void setWindowView(string name, VRView* view);
        void addWindowServer(string name, string server);
        void changeWindowName(string& name, string new_name);

        void getWindowSize(string name, int& w, int& h);
        void resizeWindow(string name, int w, int h);

        void pauseRendering(bool b);
        RenderActionRefPtr getRenderAction();
        void updateWindows();

        void save(xmlpp::Element* node);
        void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE;

#endif // VRWINDOWMANAGER_H_INCLUDED
