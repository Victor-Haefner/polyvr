

#include "VRWaypoint.h"
#include "core/math/pose.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRStorage_template.h"

#include <OpenSG/OSGPlane.h>

using namespace OSG;


VRWaypoint::VRWaypoint(string name) : VRGeometry(name) {
    type = "Waypoint";

    store("wp_pose", &pose);
    store("wp_floor", &floor);
    store("wp_at", &at);
    store("wp_size", &size);

    regStorageSetupFkt( VRStorageCb::create("waypoint setup", bind(&VRWaypoint::setup, this, placeholders::_1)) );
}

VRWaypointPtr VRWaypoint::create(string name) { return shared_ptr<VRWaypoint>(new VRWaypoint(name) ); }
VRWaypointPtr VRWaypoint::ptr() { return static_pointer_cast<VRWaypoint>( shared_from_this() ); }

void VRWaypoint::setup(VRStorageContextPtr context) {
    auto m = VRMaterial::get("waypoint");
    m->setDiffuse(Color3f(1,0,0));
    setMaterial(m);
}

void VRWaypoint::set(PosePtr p) { pose = p; updateGeo(); }
void VRWaypoint::set(VRTransformPtr t) { pose = t->getWorldPose(); at = t->getWorldAt(); updateGeo(); }
PosePtr VRWaypoint::get() { return pose; }

void VRWaypoint::apply(VRTransformPtr t) {
    if (!t || !pose || !floor) return;
    t->setWorldPosition(pose->pos());
    t->setWorldUp(floor->up());
    t->setWorldAt(at);
}

void VRWaypoint::setFloorPlane(PosePtr p) { floor = p; updateGeo(); }
void VRWaypoint::setSize(float s) { size = s; updateGeo(); }

void VRWaypoint::updateGeo() {
    auto m = VRMaterial::get("waypoint");
    m->setDiffuse(Color3f(1,0,0));
    setMaterial(m);
    float s = size;
    string params = toString(s) + " " + toString(s) + " " + toString(s*0.5) + " " + toString(s*0.4) + " " + toString(s*0.4);
    setPrimitive("Arrow " + params);
    if (!pose || !floor) return;

    // compute pos
    Vec3d pos = pose->pos();
    Plane fPlane(Vec3f(floor->up()), Pnt3f(floor->pos()));
    float d = fPlane.distance(Vec3f(pos));
    pos -= floor->up()*d;

    // compute dir
    Vec3d dir = pose->dir();
    dir -= dir.dot(floor->up())*floor->up();

    // apply pose
    auto p = Pose::create(pos, dir, floor->up());
    setWorldPose(p);
}
