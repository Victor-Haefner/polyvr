#ifndef VRROADINTERSECTION_H_INCLUDED
#define VRROADINTERSECTION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRRoadBase.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "core/math/graph.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRoadIntersection : public VRRoadBase {
    public:
        enum IntersectionTYPE {
            DEFAULT, // normal intersection with a patch to fill
            CONTINUATION, // one road goes directly into another
            FORK, // a road forks into two roads or two merge into one
            MERGE, // a road forks into two roads or two merge into one
            UPLINK // a road connects to a middle node of another road
        };

        struct RoadFront {
            VRRoadPtr road;
            Pose pose;
            int dir = 1;
            float width = 0;
            vector<VREntityPtr> inLanes; // all lanes going in of the intersection
            vector<VREntityPtr> outLanes; // all lanes going out of the intersection
        };

    private:
        VREntityPtr getRoadNode(VREntityPtr roadEnt);

        IntersectionTYPE type = DEFAULT;
        VRPolygonPtr patch;
        VRPolygonPtr perimeter;
        Vec3d median;

        vector<shared_ptr<RoadFront>> roadFronts;
        /*vector<VRRoadPtr> roads;
        map<VRRoadPtr, vector<VREntityPtr>> inLanes; // all lanes going in of the intersection
        map<VRRoadPtr, vector<VREntityPtr>> outLanes; // all lanes going out the intersection*/

        vector<Vec3d> intersectionPoints;
        vector<pair<VREntityPtr, VREntityPtr>> laneMatches; // matches of ingoing lanes with outgoing lanes
        map<VREntityPtr, vector<VREntityPtr>> nextLanes; // sequences of lanes, for example ingoing -> outgoing, or ingoing -> lane -> outgoing

    public:
        VRRoadIntersection();
        ~VRRoadIntersection();

        static VRRoadIntersectionPtr create();

        VREntityPtr addTrafficLight( PosePtr p, string asset, Vec3d root );
        VRGeometryPtr createGeometry();

        void computePatch();
        void computeLayout(GraphPtr graph);
        void computeLanes(GraphPtr graph);
        void computeMarkings();
        void computeTrafficLights();

        void addRoad(VRRoadPtr road);
};

OSG_END_NAMESPACE;

#endif // VRROADINTERSECTION_H_INCLUDED
