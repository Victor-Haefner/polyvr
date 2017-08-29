#ifndef VRROADBASE_H_INCLUDED
#define VRROADBASE_H_INCLUDED

#include <map>
#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
#include "../VRWorldModule.h"
#include "core/objects/object/VRObject.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRRoadBase : public VRObject, public VRWorldModule {
    protected:
        float roadTerrainOffset = 0.02; // same as terrain physics offset
        float markingsWidth = 0.15;

    public:
        VRRoadBase(string name);
        ~VRRoadBase();

        pathPtr toPath( VREntityPtr pathEntity, int resolution );
        void setupTexCoords( VRGeometryPtr geo, VREntityPtr way );
        vector<string> toStringVector(const Vec3d& v);

        VREntityPtr addNode( int nodeID, Vec3d pos, bool elevate = false, float elevationOffset = 0 );
        VREntityPtr addPath( string type, string name, vector<VREntityPtr> nodes, vector<Vec3d> normals );
        VREntityPtr addLane( int direction, float width, bool pedestrian = false );
        VREntityPtr addArrows( VREntityPtr lane, float t, vector<float> dirs );

        VRGeometryPtr addPole( Vec3d root, Vec3d end, float radius );
};

OSG_END_NAMESPACE;

#endif // VRROADBASE_H_INCLUDED
