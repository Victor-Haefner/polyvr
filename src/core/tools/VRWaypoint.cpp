#include "VRWaypoint.h"

using namespace OSG;

VRWaypoint::VRWaypoint() {}

void VRWaypoint::setPose(VRPose p) { pose = p; }

Vec3f VRWaypoint::transform(Vec3f v) { // TODO
    return v;
}
