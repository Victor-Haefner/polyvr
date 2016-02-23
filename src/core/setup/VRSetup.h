#ifndef VRSETUP_H_INCLUDED
#define VRSETUP_H_INCLUDED

#include "windows/VRViewManager.h"
#include "windows/VRWindowManager.h"
#include "devices/VRDeviceManager.h"
#include "tracking/VRPN.h"
#include "tracking/ART.h"
#include "core/utils/VRName.h"
#include "core/setup/VRSetupFwd.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;
class VRVisualLayer;

class VRSetup : public VRViewManager, public VRWindowManager, public VRDeviceManager, public ART, public VRPN {
    private:
        string cfgfile;
        string tracking;

        VRTransformPtr real_root = 0;
        VRTransformPtr user = 0;
        VRCameraPtr setup_cam = 0;

        VRVisualLayer* setup_layer = 0;
        VRVisualLayer* stats_layer = 0;
        VRTogglePtr layer_setup_toggle;
        VRTogglePtr layer_stats_toggle;

        void parseSetup(xmlpp::Element* setup);

        void processOptions();

        xmlpp::Element* getElementChild(xmlpp::Element* e, string name);

    public:
        VRSetup(string name);
        ~VRSetup();

        static VRSetupPtr create(string name);
        VRTransformPtr getUser();

        void addObject(VRObjectPtr o);
        VRTransformPtr getRoot();
        VRTransformPtr getTracker(string t);

        void setScene( shared_ptr<VRScene> s);
        void showSetup(bool b);

        void printOSG();

        void save(string file);
        void load(string file);
};

// TODO: make a setup manager && manage setups like a bit the scene manager!


OSG_END_NAMESPACE;

#endif // VRSETUP_H_INCLUDED
