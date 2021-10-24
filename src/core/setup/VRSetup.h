#ifndef VRSETUP_H_INCLUDED
#define VRSETUP_H_INCLUDED

#include "windows/VRViewManager.h"
#include "windows/VRWindowManager.h"
#include "devices/VRDeviceManager.h"
#include "core/scripting/VRScriptManager.h"
#include "core/scene/VRCallbackManager.h"
#include "core/utils/VRName.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/setup/VRSetupFwd.h"
#include "core/scene/VRSceneFwd.h"
#include "core/scripting/VRScriptFwd.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;
class VRVisualLayer;

class VRSetup : public VRViewManager, public VRWindowManager, public VRDeviceManager, public VRName {
    private:
        string cfgfile;
        string tracking;
        string path;
        Vec3d globalOffset;

        VRTransformPtr real_root = 0;
        VRTransformPtr user = 0;
        VRCameraPtr setup_cam = 0;

#ifndef WITHOUT_ART
        ARTPtr art;
#endif
#ifndef WITHOUT_VRPN
        VRPNPtr vrpn;
#endif

        VivePtr vive = 0;

        VRVisualLayerPtr setup_layer;
        VRVisualLayerPtr stats_layer;
        VRVisualLayerPtr stencil_layer;
        VRToggleCbPtr layer_setup_toggle;
        VRToggleCbPtr layer_stats_toggle;
        VRToggleCbPtr layer_stencil_toggle;

        VRNetworkPtr network;
        map<string, VRScriptPtr> scripts;

        void showStats(bool b);
        void showStencil(bool b);
        void parseSetup(XMLElementPtr setup);
        void processOptions();

    public:
        VRSetup(string name);
        ~VRSetup();

        static VRSetupPtr create(string name);
        static VRSetupPtr getCurrent();

        VRNetworkPtr getNetwork();
        VRTransformPtr getUser();

        ARTPtr getART();
        VRPNPtr getVRPN();

        void addObject(VRObjectPtr o);
        VRTransformPtr getRoot();
        VRTransformPtr getTracker(string t);

        void setScene( shared_ptr<VRScene> s);
        void showSetup(bool b);

        VRScriptPtr addScript(string name);
        VRScriptPtr getScript(string name);
        map<string, VRScriptPtr> getScripts();

        void printOSG();
        void updateTracking();
        void updateGtkDevices();

        void save(string file = "");
        void load(string file);

        Vec3d getDisplaysOffset();
        void setDisplaysOffset(Vec3d o);
        void setupLESCCAVELights(VRScenePtr scene); // TODO: temporary until scripts for VRSetup implemented!

        void makeTestCube();
        static void sendToBrowser(const string& msg);
};

OSG_END_NAMESPACE;

#endif // VRSETUP_H_INCLUDED
