#include "VRWaypoint.h"
#include "core/objects/material/VRMaterial.h"

#include <OpenSG/OSGPlane.h>

using namespace OSG;

VRWaypoint::VRWaypoint(string name) : VRGeometry(name) {
    type = "Waypoint";
}

VRWaypointPtr VRWaypoint::create(string name) { return shared_ptr<VRWaypoint>(new VRWaypoint(name) ); }
VRWaypointPtr VRWaypoint::ptr() { return static_pointer_cast<VRWaypoint>( shared_from_this() ); }

void VRWaypoint::set(pose p) { Pose = p; updateGeo(); }
void VRWaypoint::set(VRTransformPtr t) { Pose = t->getWorldPose(); at = t->getAt(); updateGeo(); }
pose VRWaypoint::get() { return Pose; }

void VRWaypoint::apply(VRTransformPtr t) {
    t->setWorldPosition(Pose.pos());
    t->setWorldUp(Floor.up());
    t->setAt(at);
}

void VRWaypoint::setFloorPlane(pose p) { Floor = p; updateGeo(); }

void VRWaypoint::updateGeo() {
    auto m = VRMaterial::get("waypoint");
    m->setDiffuse(Vec3f(1,0,0));
    setMaterial(m);
    setPrimitive("Arrow", "1 1 0.5 0.4");

    // compute pos
    Vec3f pos = Pose.pos();
    Plane fPlane(Floor.up(), Floor.pos());
    float d = fPlane.distance(pos);
    pos -= d*Floor.up();

    // compute dir
    Vec3f dir = -Pose.dir();
    dir -= dir.dot(Floor.up())*Floor.up();

    // apply pose
    pose p;
    p.set(pos, dir, Floor.up());
    setWorldPose(p);
}
