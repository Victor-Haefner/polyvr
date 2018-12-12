#ifndef VRROADNETWORK_H_INCLUDED
#define VRROADNETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRRoadBase.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/RealWorld/VRRealWorldFwd.h"
#include "core/math/graph.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRoadNetwork : public VRRoadBase {
    private:
        vector<VRRoadPtr> roads;
        map<VREntityPtr, VRRoadPtr> roadsByEntity;
        map<VREntityPtr, VRRoadIntersectionPtr> intersectionsByEntity;
        vector<VRRoadPtr> ways;
        vector<VRRoadIntersectionPtr> intersections;
        vector<VRGeometryPtr> assets;

        vector<VRTunnelPtr> tunnels;
        vector<VRBridgePtr> bridges;

        GraphPtr graph;
        map<int, vector<Vec3d> > graphNormals;
        map<int, VREntityPtr> graphEdgeEntities;
        map<VREntityPtr, int> graphEdgeIDs;
        //map<int, VRRoadPtr> roadsByEdgeID;

        VRAsphaltPtr asphalt;
        VRAsphaltPtr asphaltArrow;
        VRPathtoolPtr tool;
        int nextRoadID = 0;

        VRUpdateCbPtr updateCb;

        VRGeometryPtr roadsGeo;
        VRGeometryPtr arrows;
        VRGeometryPtr fences;
        VRGeometryPtr kirbs;
        VRGeometryPtr guardRails;
        VRGeometryPtr guardRailPoles;
        VRTexturePtr arrowTexture;
        map<Vec4i, int> arrowTemplates;

        VRGeometryPtr collisionMesh;

		float trackWidth = 1.6; // TODO
        float roadTerrainOffset = 0.06; // same as terrain physics offset
        float markingsWidth = 0.15;
        int arrowType = 0;

        void createArrow(Vec4i dirs, int N, const Pose& p, int type = 0);

        vector<VREntityPtr> getRoadNodes();
        vector<VRRoadPtr> getNodeRoads(VREntityPtr node);

        void init();

    public:
        VRRoadNetwork();
        ~VRRoadNetwork();

        static VRRoadNetworkPtr create();
        VRRoadNetworkPtr ptr();

        void setRoadStyle(int arrowType);
        int getArrowStyle();

        void setTerrainOffset(float o);
        void setMarkingsWidth(float w);
        float getTerrainOffset();
        float getMarkingsWidth();

        GraphPtr getGraph();
        void connectGraph(vector<VREntityPtr> nodes, vector<Vec3d> norms, VREntityPtr entity);
        vector<Vec3d> getGraphEdgeDirections(int e);
        void updateAsphaltTexture();
        VRAsphaltPtr getMaterial();
        int getRoadID();

        PosePtr getPosition(Graph::position p);
        VREntityPtr addRoute(vector<int> nodes);

        VREntityPtr getLane(int edgeID);
        int getLaneID(VREntityPtr lane);
        VRRoadPtr getRoad(VREntityPtr road);
        VRRoadIntersectionPtr getIntersection(VREntityPtr intersection);

        vector<VRRoadPtr> getRoads();
        vector<VRRoadIntersectionPtr> getIntersections();
        vector<VREntityPtr> getPreviousRoads(VREntityPtr road);
        vector<VREntityPtr> getNextRoads(VREntityPtr road);

        VREntityPtr addGreenBelt( VREntityPtr road, float width );
        VREntityPtr addNode( Vec3d pos, bool elevate = false, float elevationOffset = 0 );
        VRRoadPtr addWay( string name, vector<VREntityPtr> paths, int rID, string type );
        VRRoadPtr addRoad( string name, string type, VREntityPtr node1, VREntityPtr node2, Vec3d norm1, Vec3d norm2, int Nlanes );
        VRRoadPtr addLongRoad( string name, string type, vector<VREntityPtr> nodes, vector<Vec3d> normals, int Nlanes );

        VRTunnelPtr addTunnel(VRRoadPtr road);
        VRBridgePtr addBridge(VRRoadPtr road);

        void addKirb( VRPolygonPtr p, float height );
        void addGuardRail( PathPtr p, float height );
        void addFence( PathPtr p, float height );

        void computeLanePaths( VREntityPtr road );
        void computeIntersections();
        void computeLanes();
        void computeSurfaces();
        void computeMarkings();
        void computeArrows();
        void computeSigns();
        void physicalizeAssets(Boundingbox volume);
        vector<VRPolygonPtr> computeGreenBelts();

        void computeTracksLanes(VREntityPtr way);

        void clear();
        void compute();
        void update();

        void test1();

        double getMemoryConsumption();

        VRGeometryPtr getAssetCollisionObject();
};

OSG_END_NAMESPACE;

#endif // VRROADNETWORK_H_INCLUDED
