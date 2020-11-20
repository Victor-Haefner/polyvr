#include "VRTests.h"

#include "core/scene/VRScene.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/OSGObject.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeometry.h"
#ifndef WITHOUT_VIRTUOSE
#include "core/setup/devices/VRHaptic.h"
#endif
#include "core/utils/toString.h"

#include <map>
#include <OpenSG/OSGMaterial.h>
#include <OpenSG/OSGNameAttachment.h>

using namespace OSG;

void listActiveMaterials() {
    map<Material*, MaterialMTRecPtr> materials;

    auto root = VRScene::getCurrent()->getRoot();
    auto geos = root->getChildren(true, "Geometry");
    for (auto& g : geos) {
        VRGeometryPtr ge = dynamic_pointer_cast<VRGeometry>(g);
        if (!ge) continue;
        auto m = ge->getMesh();
        if (!m) continue;
        GeometryMTRecPtr geo = m->geo;
        if (!geo) continue;
        MaterialMTRecPtr mat = geo->getMaterial();
        if (!mat) continue;
        materials[mat.get()] = mat;
    }

    int i=0;
    for (auto& m : materials) {
        cout << "Material " << i << " : " << m.first << endl;
        i++;
    }
}

#ifndef WITHOUT_VRPN
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
#endif

#include <OpenSG/OSGDrawableStatsAttachment.h>

void debugFields(string data) {
    auto IDs = splitString(data);
    int N = FieldContainerFactory::the()->getNumTotalContainers();

    for (auto node : VRScene::getCurrent()->getRoot()->getNodes()) {
        string name = OSG::getName(node->node) ? OSG::getName(node->node) : "unknown";
        DrawableStatsAttachment *st = DrawableStatsAttachment::get(node->node->getCore());
        if (st) OSG::setName(st, (name+"_stats_attachment").c_str());
    }

    for (string s : IDs) {
        int fieldID = toInt(s);
        if (fieldID >= N) continue;

        FieldContainer* fc = FieldContainerFactory::the()->getContainer( fieldID );

        AttachmentContainer* ac = dynamic_cast<AttachmentContainer*>(fc);
        if (ac == 0) {
            Attachment* a = dynamic_cast<Attachment*>(fc);
            if (a != 0) {
                FieldContainer* dad = 0;
                if (a->getMFParents()->size() > 0) dad = a->getParents(0);
                ac = dynamic_cast<AttachmentContainer*>(dad);
            }
        }

        string name = "no name";
        if (auto n = OSG::getName(ac)) name = string(n);
        cout << "Debug field " << fieldID << ", type: " << fc->getTypeName() << " name: " << name << " refcount: " << fc->getRefCount() << endl;
    }
}

void VRRunTest(string test) {
    cout << "run test " << test << endl;

    if (test == "listActiveMaterials") listActiveMaterials();
#ifndef WITHOUT_VRPN
    if (test == "vrpn_client") vrpn_client();
    if (test == "vrpn_server") vrpn_server();
#endif
#ifndef WITHOUT_VIRTUOSE
    if (test == "haptic1") VRHaptic::runTest1();
#endif
    if (startsWith(test, "debugFields")) debugFields( subString(test, 12, test.size()-12) );
}
