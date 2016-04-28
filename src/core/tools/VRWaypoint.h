#ifndef VRWAYPOINT_H_INCLUDED
#define VRWAYPOINT_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "core/tools/VRToolsFwd.h"
#include "core/math/pose.h"

OSG_BEGIN_NAMESPACE;

class VRWaypoint : public VRGeometry {
    private:
        pose Pose;
        pose Floor;
        Vec3f at;
        float size;

        void setup();
        void updateGeo();

    public:
        VRWaypoint(string name);

        static VRWaypointPtr create(string name = "None");
        VRWaypointPtr ptr();

        void set(pose p);
        void set(VRTransformPtr t);
        void apply(VRTransformPtr t);
        pose get();

        void setSize(float s);
        void setFloorPlane(pose p);
};

OSG_END_NAMESPACE;

#endif // VRWAYPOINT_H_INCLUDED
