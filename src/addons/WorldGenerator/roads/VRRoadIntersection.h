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
            UPLINK, // a road connects to a middle node of another road
            CROSSING
        };

        struct RoadFront {
            VRRoadPtr road;
            Pose pose;
            int dir = 1;
            float width = 0;
            vector<VREntityPtr> inLanes; // all lanes going in of the intersection
            vector<VREntityPtr> outLanes; // all lanes going out of the intersection

            RoadFront(VRRoadPtr road);
        };
        VRRoadIntersectionPtr isecPtr;

    private:
        VREntityPtr getRoadNode(VREntityPtr roadEnt);

        void setPtr(VRRoadIntersectionPtr i);

        IntersectionTYPE type = DEFAULT;
        VRPolygonPtr patch;
        VRPolygonPtr perimeter;
        Vec3d median;
        VREntityPtr forkSingleRoad;

        vector<shared_ptr<RoadFront>> roadFronts;
        vector<Vec3d> intersectionPoints;
        vector<pair<VREntityPtr, VREntityPtr>> laneMatches; // matches of ingoing lanes with outgoing lanes
        vector<string> laneTurnDirection;
        map<VREntityPtr, vector<VREntityPtr>> nextLanes; // sequences of lanes, for example ingoing -> outgoing, or ingoing -> lane -> outgoing
        map<VREntityPtr, VRTrafficLightPtr> matchedLights;

        VRTrafficLightsPtr system;

        vector<vector<int>> lsIDs;

    public:
        VRRoadIntersection();
        ~VRRoadIntersection();

        static VRRoadIntersectionPtr create();

        VREntityPtr addTrafficLight( PosePtr p, string asset, Vec3d root, VREntityPtr lane, VREntityPtr signal );
        VRGeometryPtr createGeometry();

        void computePatch();
        void computeLayout(GraphPtr graph);
        void computeLanes(GraphPtr graph);
        void computeMarkings();
        void computeTrafficLights();
        void computeTrafficSigns();
        void computeSemantics();

        void addRoad(VRRoadPtr road);
        vector<VRRoadPtr> getRoads();
        vector<VREntityPtr> getInLanes();
        vector<VRTrafficLightPtr> getTrafficLights();
        VRTrafficLightPtr getTrafficLight(VREntityPtr lane);
        map<VREntityPtr,Vec3d> crossingRoads;
        vector<Vec3d> crossingRoadsPos;
        vector<VREntityPtr> trafficLightRoads; // roads with a crossing

        map<int, vector<VRTrafficLightPtr>> getTrafficLightMap();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRROADINTERSECTION_H_INCLUDED
