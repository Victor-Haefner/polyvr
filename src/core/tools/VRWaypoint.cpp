#include "VRWaypoint.h"

using namespace OSG;

VRWaypoint::VRWaypoint(string name) : VRGeometry(name) {}

VRWaypointPtr VRWaypoint::create(string name) { return shared_ptr<VRWaypoint>(new VRWaypoint(name) ); }
VRWaypointPtr VRWaypoint::ptr() { return static_pointer_cast<VRWaypoint>( shared_from_this() ); }

void VRWaypoint::setPose(VRPose p) { pose = p; }

Vec3f VRWaypoint::transform(Vec3f v) { // TODO
    return v;
}
