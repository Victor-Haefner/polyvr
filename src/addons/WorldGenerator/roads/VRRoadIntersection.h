#ifndef VRROADINTERSECTION_H_INCLUDED
#define VRROADINTERSECTION_H_INCLUDED

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

class VRRoadIntersection : public VRRoadBase {
    private:
        VREntityPtr getRoadNode(VREntityPtr roadEnt);
        vector<VRRoadPtr> roads;

    public:
        VRRoadIntersection();
        ~VRRoadIntersection();

        static VRRoadIntersectionPtr create();

        VREntityPtr addTrafficLight( posePtr p, string asset, Vec3f root );
        VRGeometryPtr createGeometry();

        void computeLayout();
        void computeLanes();
        void computeMarkings();
        void computeTrafficLights();

        void addRoad(VRRoadPtr road);
};

OSG_END_NAMESPACE;

#endif // VRROADINTERSECTION_H_INCLUDED
