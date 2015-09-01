#ifndef VRPATHTOOL_H_INCLUDED
#define VRPATHTOOL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <vector>
#include <map>

#include "core/objects/geometry/VRGeometry.h"


using namespace std;
OSG_BEGIN_NAMESPACE

class VRStroke;
class VRTransform;
class VRDevice;
class VRObject;
class path;



class VRManipulator {
    private:
        VRObject* anchor;
        VRGeometry *gTX, *gTY, *gTZ;
        VRGeometry *gRX, *gRY, *gRZ;
        VRTransform* sel = 0;

        void setup() {
            anchor = new VRObject("manipulator");
            gTX = new VRGeometry("gTX");
            gTY = new VRGeometry("gTY");
            gTZ = new VRGeometry("gTZ");
            gRX = new VRGeometry("gRX");
            gRY = new VRGeometry("gRY");
            gRZ = new VRGeometry("gRZ");
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

        void handle(VRGeometry* g) {
            if (sel == 0) return;
            sel->toggleTConstraint(false);
            sel->toggleRConstraint(false);
            sel->setTConstraintMode(false);

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

        void manipulate(VRTransform* t) {
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
            map<VRGeometry*, int> handles;
            VRStroke* line = 0;
            VRObject* anchor = 0;
        };

        map<path*, entry*> paths;
        map<VRGeometry*, entry*> handles_dict;

        VRManipulator* manip = 0;
        VRExtruder* ext = 0;

        VRGeometry* newHandle();
        void updateHandle(VRGeometry* handle);
        void updateDevs();

    public:
        VRPathtool();

        path* newPath(VRDevice* dev, VRObject* anchor, int resolution = 10);
        VRGeometry* extrude(VRDevice* dev, path* p);
        void remPath(path* p);

        void addPath(path* p, VRObject* anchor);
        void setVisible(bool handles, bool lines);
        void clear(path* p);

        vector<path*> getPaths();
        vector<VRGeometry*> getHandles(path* p);
        VRStroke* getStroke(path* p);

        void select(VRGeometry* handle);
        void update();
};

OSG_END_NAMESPACE

#endif // VRPATHTOOL_H_INCLUDED
