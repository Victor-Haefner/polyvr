#ifndef VRWAYPOINT_H_INCLUDED
#define VRWAYPOINT_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;

class VRPose {
    Vec3f from, dir, up;
};

class VRWaypoint : public VRGeometry {
    private:
        VRPose pose;

    public:
        VRWaypoint(string name);

        static VRWaypointPtr create(string name);
        VRWaypointPtr ptr();

        void setPose(VRPose p);

        Vec3f transform(Vec3f v);
};

OSG_END_NAMESPACE;

#endif // VRWAYPOINT_H_INCLUDED
