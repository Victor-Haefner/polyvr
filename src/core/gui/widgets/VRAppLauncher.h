#ifndef VRAPPLAUNCHER_H_INCLUDED
#define VRAPPLAUNCHER_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>

#include "core/utils/VRFwdDeclTemplate.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRAppLauncher);
ptrFwd(VRAppPanel);
ptrFwd(VRAppManager);

class VRAppLauncher : public std::enable_shared_from_this<VRAppLauncher> {
    public:
        string ID;
        string path;
        string lastStarted;
        string pxm_path;
        string table;
        bool running = false;
        bool pixmap = false;
        bool favorite = true;
        bool write_protected = false;
        VRDeviceCbPtr uPixmap;
        VRAppPanelWeakPtr section;

    public:
        VRAppLauncher(VRAppPanelPtr s, string path, string timestamp);
        ~VRAppLauncher();
        static VRAppLauncherPtr create(VRAppPanelPtr s, string path, string timestamp);

        void setup(VRAppManager* mgr);
        bool updatePixmap();

        void setState(int state);
        void toggle_lock();

        void show();
        void hide();
};

OSG_END_NAMESPACE;

#endif // VRAPPLAUNCHER_H_INCLUDED
