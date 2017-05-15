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
        GraphPtr graph;
        VRAsphaltPtr asphalt;
        VROntologyPtr ontology;
        VRPathtoolPtr tool;
        int nextRoadID = 0;

		float trackWidth = 1.6; // TODO

        void setupTexCoords( VRGeometryPtr geo, VREntityPtr way );
        pathPtr toPath( VREntityPtr pathEntity, int resolution );

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
        VREntityPtr addPath( string type, string name, VREntityPtr node1, VREntityPtr node2, Vec3f normal1, Vec3f normal2 );

        void addRoad( string name, VREntityPtr node1, VREntityPtr node2, Vec3f norm1, Vec3f norm2, int Nlanes );
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
