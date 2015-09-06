#ifndef VRWAYPOINT_H_INCLUDED
#define VRWAYPOINT_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;

class VRPose {
    Vec3f from, at, up;
};

class VRWaypoint : public VRGeometry {
    private:
        VRPose pose;

    public:
        VRWaypoint();

        void setPose(VRPose p);
};

OSG_END_NAMESPACE;

#endif // VRWAYPOINT_H_INCLUDED
