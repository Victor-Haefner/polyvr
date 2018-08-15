#ifndef VRROAD_H_INCLUDED
#define VRROAD_H_INCLUDED

#include <map>
#include <OpenSG/OSGVector.h>
#include "VRRoadBase.h"
#include "core/objects/VRObjectFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRoad : public VRRoadBase {
    private:
        struct edgePoint {
            Vec3d p1;
            Vec3d p2;
            Vec3d n;
            VREntityPtr entry;

            edgePoint() {}
            edgePoint(Vec3d p1, Vec3d p2, Vec3d n, VREntityPtr e) : p1(p1), p2(p2), n(n), entry(e) {}
        };

        float offsetIn = 0;
        float offsetOut = 0;
        map<VREntityPtr, edgePoint> edgePoints;

    public:
        VRRoad();
        ~VRRoad();

        static VRRoadPtr create();

        void addParkingLane( int direction, float width, int capacity, string type );
        void addTrafficLight( Vec3d pos );

        void setOffsetIn(float o);
        void setOffsetOut(float o);
        float getWidth();
        VRGeometryPtr createGeometry();
        VREntityPtr getNodeEntry( VREntityPtr node );
        edgePoint& getEdgePoints( VREntityPtr node );
        void computeMarkings();
        bool hasMarkings();
        PosePtr getRightEdge(Vec3d pos);
        vector<VRRoadPtr> splitAtIntersections(VRRoadNetworkPtr network);

        PosePtr getPosition(float t);
};

OSG_END_NAMESPACE;

#endif // VRROAD_H_INCLUDED
