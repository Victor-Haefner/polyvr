#ifndef VRSETUP_H_INCLUDED
#define VRSETUP_H_INCLUDED

#include "windows/VRViewManager.h"
#include "windows/VRWindowManager.h"
#include "devices/VRDeviceManager.h"
#include "tracking/VRPN.h"
#include "tracking/ART.h"
#include "core/utils/VRName.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScene;

class VRSetup : public VRViewManager, public VRWindowManager, public VRDeviceManager, public ART, public VRPN, public VRName {
    private:
        string cfgfile;
        string tracking;

        VRTransform* real_root;
        VRTransform* user;
        VRCamera* setup_cam;

        void parseSetup(xmlpp::Element* setup);

        void processOptions();

        xmlpp::Element* getElementChild(xmlpp::Element* e, string name);

    public:
        VRSetup(string name);
        ~VRSetup();

        VRTransform* getUser();

        void addObject(VRObject* o);
        VRTransform* getRoot();
        VRTransform* getTracker(string t);

        void setScene(VRScene* s);
        void showSetup(bool b);

        void save(string file);
        void load(string file);
};

// TODO: make a setup manager and manage setups like a bit the scene manager!


OSG_END_NAMESPACE;

#endif // VRSETUP_H_INCLUDED
