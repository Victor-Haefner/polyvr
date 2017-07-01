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
        VRAsphaltPtr asphalt;
        VRAsphaltPtr asphaltArrow;
        VRPathtoolPtr tool;
        VRWoodsPtr natureManager;
        int nextRoadID = 0;

        VRGeometryPtr arrows;
        VRTexturePtr arrowTexture;
        map<Vec4i, int> arrowTemplates;

		float trackWidth = 1.6; // TODO

        void createArrow(Vec4i dirs, int N, const pose& p);

        vector<VREntityPtr> getRoadNodes();
        vector<VRRoadPtr> getNodeRoads(VREntityPtr node);

        void init();

    public:
        VRRoadNetwork();
        ~VRRoadNetwork();

        static VRRoadNetworkPtr create();

        void setNatureManager(VRWoodsPtr mgr);
        GraphPtr getGraph();
        void updateAsphaltTexture();
        VRAsphaltPtr getMaterial();
        int getRoadID();

        VREntityPtr addGreenBelt( VREntityPtr road, float width );
        VRRoadPtr addWay( string name, vector<VREntityPtr> paths, int rID, string type );
        VRRoadPtr addRoad( string name, string type, VREntityPtr node1, VREntityPtr node2, Vec3f norm1, Vec3f norm2, int Nlanes );
        VREntityPtr addArrows( VREntityPtr lane, float t, vector<float> dirs );

        void addPole( Vec3f root, Vec3f end );

        void computeLanePaths( VREntityPtr road );
        void computeIntersections();
        void computeLanes();
        void computeSurfaces();
        void computeMarkings();
        vector<VRPolygonPtr> computeGreenBelts();

        void computeTracksLanes(VREntityPtr way);

        void clear();
        void compute();
};

OSG_END_NAMESPACE;

#endif // VRROADNETWORK_H_INCLUDED
