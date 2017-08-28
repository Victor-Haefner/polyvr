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
        vector<VRRoadPtr> ways;
        vector<VRRoadIntersectionPtr> intersections;

        GraphPtr graph;
        map<int, vector<Vec3d> > graphNormals;

        VRAsphaltPtr asphalt;
        VRAsphaltPtr asphaltArrow;
        VRPathtoolPtr tool;
        int nextRoadID = 0;

        VRGeometryPtr arrows;
        VRTexturePtr arrowTexture;
        map<Vec4i, int> arrowTemplates;

		float trackWidth = 1.6; // TODO

        void createArrow(Vec4i dirs, int N, const pose& p);

        vector<VREntityPtr> getRoadNodes();
        vector<VRRoadPtr> getNodeRoads(VREntityPtr node);

        void mergeRoads(VREntityPtr node, VRRoadPtr road1, VRRoadPtr road2);

        void init();

    public:
        VRRoadNetwork();
        ~VRRoadNetwork();

        static VRRoadNetworkPtr create();

        GraphPtr getGraph();
        void connectGraph(vector<VREntityPtr> nodes, vector<Vec3d> norms);
        vector<Vec3d> getGraphEdgeDirections(int e);
        void updateAsphaltTexture();
        VRAsphaltPtr getMaterial();
        int getRoadID();

        VREntityPtr addGreenBelt( VREntityPtr road, float width );
        VREntityPtr addNode( Vec3d pos, bool elevate = false, float elevationOffset = 0 );
        VRRoadPtr addWay( string name, vector<VREntityPtr> paths, int rID, string type );
        VRRoadPtr addRoad( string name, string type, VREntityPtr node1, VREntityPtr node2, Vec3d norm1, Vec3d norm2, int Nlanes );
        VRRoadPtr addLongRoad( string name, string type, vector<VREntityPtr> nodes, vector<Vec3d> normals, int Nlanes );

        void addKirb( VRPolygonPtr p, float height );
        void addGuardRail( pathPtr p, float height );

        void computeLanePaths( VREntityPtr road );
        void computeIntersections();
        void computeLanes();
        void computeSurfaces();
        void computeMarkings();
        void computeArrows();
        void computeSigns();
        vector<VRPolygonPtr> computeGreenBelts();

        void computeTracksLanes(VREntityPtr way);

        void clear();
        void compute();

        void test1();
};

OSG_END_NAMESPACE;

#endif // VRROADNETWORK_H_INCLUDED
