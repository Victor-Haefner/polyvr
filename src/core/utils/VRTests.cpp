#include "VRTests.h"

#include "core/scene/VRScene.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/OSGObject.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeometry.h"

#include <map>
#include <OpenSG/OSGMaterial.h>

using namespace OSG;

void listActiveMaterials() {
    map<Material*, MaterialRecPtr> materials;

    auto root = VRScene::getCurrent()->getRoot();
    auto geos = root->getChildren(true, "Geometry");
    for (auto& g : geos) {
        VRGeometryPtr ge = dynamic_pointer_cast<VRGeometry>(g);
        if (!ge) continue;
        auto m = ge->getMesh();
        if (!m) continue;
        GeometryMTRecPtr geo = m->geo;
        if (!geo) continue;
        MaterialRecPtr mat = geo->getMaterial();
        if (!mat) continue;
        materials[mat.get()] = mat;
    }

    int i=0;
    for (auto& m : materials) {
        cout << "Material " << i << " : " << m.first << endl;
        i++;
    }
}

#include <vrpn/vrpn_Tracker.h>
class myTracker : public vrpn_Tracker_Remote {
    public:
        myTracker(string name) : vrpn_Tracker_Remote( name.c_str() ) {
            shutup = true;
        }

        void doLoop() {
            auto conn = connectionPtr();
            cout << "vrpn_client do\n";
            conn->mainloop();
            client_mainloop();
            cout << "vrpn_client done\n";
        }
};

void vrpn_client() {
    auto tracker = new myTracker( "Tracker0@localhost" );
    while (true) tracker->doLoop();
}

#include "core/setup/VRSetup.h"
void vrpn_server() {
    auto setup = VRSetup::getCurrent();
    if (setup) setup->startVRPNTestServer();
}

void VRRunTest(string test) {
    cout << "run test " << test << endl;

    if (test == "listActiveMaterials") listActiveMaterials();
    if (test == "vrpn_client") vrpn_client();
    if (test == "vrpn_server") vrpn_server();
}
