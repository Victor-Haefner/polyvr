#ifndef VRPATHTOOL_H_INCLUDED
#define VRPATHTOOL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <vector>
#include <map>

#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/VRFunctionFwd.h"


using namespace std;
OSG_BEGIN_NAMESPACE

class VRStroke;
class VRTransform;
class VRDevice;
class VRObject;
class path;



class VRManipulator {
    private:
        VRObjectPtr anchor;
        VRGeometryPtr gTX, gTY, gTZ;
        VRGeometryPtr gRX, gRY, gRZ;
        VRTransformPtr sel = 0;

        void setup() {
            anchor = VRObject::create("manipulator");
            gTX = VRGeometry::create("gTX");
            gTY = VRGeometry::create("gTY");
            gTZ = VRGeometry::create("gTZ");
            gRX = VRGeometry::create("gRX");
            gRY = VRGeometry::create("gRY");
            gRZ = VRGeometry::create("gRZ");
            gTX->setPrimitive("Box", "0.1 0.02 0.02 1 1 1");
            gTY->setPrimitive("Box", "0.02 0.1 0.02 1 1 1");
            gTZ->setPrimitive("Box", "0.02 0.02 0.1 1 1 1");
            gRX->setPrimitive("Torus", "0.01 0.07 4 16");
            gRY->setPrimitive("Torus", "0.01 0.07 4 16");
            gRZ->setPrimitive("Torus", "0.01 0.07 4 16");
            gRX->setDir(Vec3f(1,0,0));
            gRY->setDir(Vec3f(0,1,0));
            gRY->setUp(Vec3f(1,0,0));
            anchor->addChild(gTX);
            anchor->addChild(gTY);
            anchor->addChild(gTZ);
            anchor->addChild(gRX);
            anchor->addChild(gRY);
            anchor->addChild(gRZ);
        }

    public:
        VRManipulator() {
            setup();
        }

        void handle(VRGeometryPtr g) {
            if (sel == 0) return;
            sel->toggleTConstraint(false);
            sel->toggleRConstraint(false);
            sel->setTConstraintMode(VRTransform::LINE);

            if (g == gTX || g == gTY || g == gTZ || g == gRX || g == gRY || g == gRZ) { // lock everything
                sel->toggleTConstraint(true);
                sel->toggleRConstraint(true);
                sel->setTConstraint(Vec3f(0,0,0));
                sel->setRConstraint(Vec3i(1,1,1));
            }

            if (g == gTX) sel->setTConstraint(Vec3f(1,0,0));
            if (g == gTY) sel->setTConstraint(Vec3f(0,1,0));
            if (g == gTZ) sel->setTConstraint(Vec3f(0,0,1));
            //if (g == gRX) sel->setRConstraint(Vec3i(0,1,1)); // TODO: fix rotation constraints!
            //if (g == gRY) sel->setRConstraint(Vec3i(1,0,1));
            //if (g == gRZ) sel->setRConstraint(Vec3i(1,1,0));
            if (g == gRX || g == gRY || g == gRZ) sel->setRConstraint(Vec3i(0,0,0));
        }

        void manipulate(VRTransformPtr t) {
            t->addChild(anchor);
            sel = t;
        }
};

class VRExtruder {
    private:
        void update();

    public:
        VRExtruder();

        // update fkt
        //  check if any device beacon changed
        //  intersect with invisible cones
        //  on intersection
        //   make cone visible

        //  check if any device drags a cone of ours
        //   trigger events -> start drag/ update drag/ stop drag
};

class VRPathtool {
    private:
        struct entry {
            path* p = 0;
            int resolution = 10;
            map<VRGeometryPtr, int> handles;
            VRStrokePtr line = 0;
            VRObjectPtr anchor = 0;
        };

        map<path*, entry*> paths;
        map<VRGeometry*, entry*> entries;
        vector<VRGeometryWeakPtr> handles;

        VRUpdatePtr updatePtr;
        VRManipulator* manip = 0;
        VRExtruder* ext = 0;

        VRGeometryPtr customHandle;
        VRGeometryPtr newHandle();
        void updateHandle(VRGeometryPtr handle);
        void updateDevs();

    public:
        VRPathtool();

        path* newPath(VRDevice* dev, VRObjectPtr anchor, int resolution = 10);
        VRGeometryPtr extrude(VRDevice* dev, path* p);
        void remPath(path* p);

        void addPath(path* p, VRObjectPtr anchor);
        void setVisible(bool handles, bool lines);
        void setHandleGeometry(VRGeometryPtr geo);
        void clear(path* p);

        vector<path*> getPaths();
        vector<VRGeometryPtr> getHandles(path* p);
        VRStrokePtr getStroke(path* p);

        void select(VRGeometryPtr handle);
        void update();
};

OSG_END_NAMESPACE

#endif // VRPATHTOOL_H_INCLUDED
