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
#include "core/math/graph.h"

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

            entry(VRObjectPtr anchor);
            void addHandle(VRGeometryPtr h);
        };

        struct knot {
            vector<int> in;
            vector<int> out;
            VRGeometryWeakPtr handle;
        };

        map<path*, entry*> paths;
        map<VRGeometry*, vector<entry*> > entries; // map handle geometries to the entries
        vector<VRGeometryWeakPtr> handles;
        graph_basePtr graph;
        map<int, knot> knots; // maps graph node ids to pathtool knots
        path* selectedPath = 0;

        VRMaterialPtr lmat;
        VRMaterialPtr lsmat;

        VRUpdatePtr updatePtr;
        VRManipulator* manip = 0;
        VRExtruder* ext = 0;

        VRGeometryPtr customHandle;
        VRGeometryPtr newHandle();
        void updateHandle(VRGeometryPtr handle);
        void updateEntry(entry* e);
        void updateDevs();

    public:
        VRPathtool();
        static VRPathtoolPtr create();

        void setGraph(graph_basePtr g);

        path* newPath(VRDevicePtr dev, VRObjectPtr anchor, int resolution = 10);
        VRGeometryPtr extrude(VRDevicePtr dev, path* p);
        void remPath(path* p);

        void addPath(path* p, VRObjectPtr anchor = 0, VRGeometryPtr ha = 0, VRGeometryPtr he = 0);
        void setVisible(bool handles, bool lines);
        void setHandleGeometry(VRGeometryPtr geo);
        void clear(path* p = 0);

        vector<path*> getPaths();
        path* getPath(VRGeometryPtr h1, VRGeometryPtr h2);
        vector<VRGeometryPtr> getHandles(path* p);
        VRStrokePtr getStroke(path* p);

        VRMaterialPtr getPathMaterial();

        void select(VRGeometryPtr handle);
        void select(path* p);
        void deselect();
        void update();
};

OSG_END_NAMESPACE

#endif // VRPATHTOOL_H_INCLUDED
