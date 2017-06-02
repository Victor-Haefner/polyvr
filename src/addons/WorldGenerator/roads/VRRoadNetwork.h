#ifndef VRROADNETWORK_H_INCLUDED
#define VRROADNETWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "core/math/graph.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRoadNetwork : public VRObject {
    private:
        vector<VREntityPtr> roads;

        GraphPtr graph;
        VRAsphaltPtr asphalt;
        VRAsphaltPtr asphaltArrow;
        VROntologyPtr ontology;
        VRPathtoolPtr tool;
        int nextRoadID = 0;

        VRGeometryPtr arrows;
        VRTexturePtr arrowTexture;
        map<Vec4i, int> arrowTemplates;

		float trackWidth = 1.6; // TODO

        void setupTexCoords( VRGeometryPtr geo, VREntityPtr way );
        pathPtr toPath( VREntityPtr pathEntity, int resolution );
        void createArrow(Vec4i dirs, int N, const pose& p);

        vector<VREntityPtr> getRoadNodes();
        vector<VREntityPtr> getNodeRoads(VREntityPtr node);
        VREntityPtr getIntersectionRoadNode(VREntityPtr roadEnt, VREntityPtr intersectionEnt);

        void init();

    public:
        VRRoadNetwork();
        ~VRRoadNetwork();

        static VRRoadNetworkPtr create();

        void setOntology(VROntologyPtr ontology);
        GraphPtr getGraph();
        void updateTexture();
        VRAsphaltPtr getMaterial();
        int getRoadID();

        VREntityPtr addNode( Vec3f pos );
        VREntityPtr addLane( int direction, VREntityPtr road, float width );
        VREntityPtr addWay( string name, vector<VREntityPtr> paths, int rID, string type );
        VREntityPtr addPath( string type, string name, vector<VREntityPtr> nodes, vector<Vec3f> normals );
        VREntityPtr addRoad( string name, string type, VREntityPtr node1, VREntityPtr node2, Vec3f norm1, Vec3f norm2, int Nlanes );
        VREntityPtr addArrows( VREntityPtr lane, float t, vector<float> dirs );

        VRGeometryPtr createRoadGeometry( VREntityPtr road );
        VRGeometryPtr createIntersectionGeometry( VREntityPtr intersectionEnt );

        void computeIntersectionLanes( VREntityPtr intersection );
        void computeLanePaths( VREntityPtr road );
        void computeIntersections();
        void computeLanes();
        void computeSurfaces();
        void computeMarkings();

        void computeTracksLanes(VREntityPtr way);
        void computeMarkingsRoad2(VREntityPtr roadEnt);
        void computeMarkingsIntersection(VREntityPtr intersection);

        void clear();
        void compute();
};

OSG_END_NAMESPACE;

#endif // VRROADNETWORK_H_INCLUDED
