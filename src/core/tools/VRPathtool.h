#ifndef VRPATHTOOL_H_INCLUDED
#define VRPATHTOOL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <vector>
#include <map>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE

class VRDevice;
class path;

class VRManipulator {
    private:
        VRObjectPtr anchor;
        VRGeometryPtr gTX, gTY, gTZ;
        VRGeometryPtr gRX, gRY, gRZ;
        VRTransformPtr sel = 0;

        void setup();

    public:
        VRManipulator();

        void handle(VRGeometryPtr g);
        void manipulate(VRTransformPtr t);
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

class VRPathtool : public VRObject {
    private:
        struct entry {
            path* p = 0;
            int resolution = 10;
            map<VRGeometry*, int> points;
            vector<VRGeometryWeakPtr> handles;
            VRStrokeWeakPtr line;
            VRObjectWeakPtr anchor;
        };

        map<path*, entry*> paths;
        map<VRGeometry*, entry*> entries;
        vector<VRGeometryWeakPtr> handles;

        VRMaterialPtr lmat;

        VRUpdatePtr updatePtr;
        VRManipulator* manip = 0;
        VRExtruder* ext = 0;

        VRGeometryPtr customHandle;
        VRGeometryPtr newHandle();
        void updateHandle(VRGeometryPtr handle);
        void updateDevs();

    public:
        VRPathtool();
        static VRPathtoolPtr create();

        path* newPath(VRDevicePtr dev, VRObjectPtr anchor, int resolution = 10);
        VRGeometryPtr extrude(VRDevicePtr dev, path* p);
        void remPath(path* p);

        void addPath(path* p, VRObjectPtr anchor);
        void setVisible(bool handles, bool lines);
        void setHandleGeometry(VRGeometryPtr geo);
        void clear(path* p);

        vector<path*> getPaths();
        vector<VRGeometryPtr> getHandles(path* p);
        VRStrokePtr getStroke(path* p);

        VRMaterialPtr getPathMaterial();

        void select(VRGeometryPtr handle);
        void update();
};

OSG_END_NAMESPACE

#endif // VRPATHTOOL_H_INCLUDED
