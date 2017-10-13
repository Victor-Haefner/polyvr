#include "VRWaypoint.h"
#include "core/math/pose.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRStorage_template.h"

#include <OpenSG/OSGPlane.h>
#include <boost/bind.hpp>

using namespace OSG;

VRWaypoint::VRWaypoint(string name) : VRGeometry(name) {
    type = "Waypoint";

    store("wp_pose", &Pose);
    store("wp_floor", &Floor);
    store("wp_at", &at);
    store("wp_size", &size);

    regStorageSetupFkt( VRUpdateCb::create("waypoint setup", boost::bind(&VRWaypoint::setup, this)) );
}

VRWaypointPtr VRWaypoint::create(string name) { return shared_ptr<VRWaypoint>(new VRWaypoint(name) ); }
VRWaypointPtr VRWaypoint::ptr() { return static_pointer_cast<VRWaypoint>( shared_from_this() ); }

void VRWaypoint::setup() {
    auto m = VRMaterial::get("waypoint");
    m->setDiffuse(Color3f(1,0,0));
    setMaterial(m);
}

void VRWaypoint::set(PosePtr p) { Pose = p; updateGeo(); }
void VRWaypoint::set(VRTransformPtr t) { Pose = t->getWorldPose(); at = t->getAt(); updateGeo(); }
PosePtr VRWaypoint::get() { return Pose; }

void VRWaypoint::apply(VRTransformPtr t) {
    t->setWorldPosition(Pose->pos());
    t->setWorldUp(Floor->up());
    t->setAt(at);
}

void VRWaypoint::setFloorPlane(PosePtr p) { Floor = p; updateGeo(); }
void VRWaypoint::setSize(float s) { size = s; updateGeo(); }

void VRWaypoint::updateGeo() {
    auto m = VRMaterial::get("waypoint");
    m->setDiffuse(Color3f(1,0,0));
    setMaterial(m);
    float s = size;
    string params = toString(s) + " " + toString(s) + " " + toString(s*0.5) + " " + toString(s*0.4);
    setPrimitive("Arrow", params);
    if (!Pose) return;

    // compute pos
    Vec3d pos = Pose->pos();
    Plane fPlane(Vec3f(Floor->up()), Pnt3f(Floor->pos()));
    float d = fPlane.distance(Vec3f(pos));
    pos -= Floor->up()*d;

    // compute dir
    Vec3d dir = -Pose->dir();
    dir -= dir.dot(Floor->up())*Floor->up();

    // apply pose
    auto p = Pose::create(pos, dir, Floor->up());
    setWorldPose(p);
}
