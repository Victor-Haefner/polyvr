#include "VRKinematictool.h"
#include "core/utils/toString.h"
#include "core/objects/VRTransform.h"

#include <OpenSG/OSGPlane.h>
#include <OpenSG/OSGLine.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRJointTool::VRJointTool(string name) {
    setName(name);
    setLabelParams(0.05, true, true);
}

VRJointToolPtr VRJointTool::ptr() { return static_pointer_cast<VRJointTool>( shared_from_this() ); }
VRJointToolPtr VRJointTool::create(string name) {
    auto ptr = shared_ptr<VRJointTool>(new VRJointTool(name) );
    ptr->init();
    return ptr;
}

void VRJointTool::clear() {
    obj1.reset();
    obj2.reset();
    VRAnalyticGeometry::clear();
}

void VRJointTool::append(VRTransformPtr t, pose p) {
    if (lastAppended) {
        obj1 = t;
        anchor1 = p;
    } else {
        obj2 = t;
        anchor2 = p;
    }

    lastAppended = !lastAppended;
    update();
}

void VRJointTool::update() {
    // get initial rotation axis
    Plane p1(anchor1.dir(), anchor1.pos());
    Plane p2(anchor2.dir(), anchor2.pos());
    Line axis;
    p1.intersect(p2, axis);

    pose lp;
    lp.set(Vec3f(axis.getPosition()), axis.getDirection(), anchor1.dir());
    Matrix L = lp.asMatrix();

    VRTransformPtr o1 = obj1.lock();
    VRTransformPtr o2 = obj2.lock();
    if (!o1 || !o2) return;

    Vec3f r(1,0,0);
    Vec3f g(0,1,0);
    Vec3f y(1,1,0);

    setVector(0, anchor1.pos(), anchor1.dir(), r, "p1");
    setVector(1, anchor2.pos(), anchor2.dir(), g, "p2");
    setVector(2, lp.pos(), lp.dir(), y, "a");

    auto c = o2->getConstraint();
    c->setReferential(o1);
    c->setRConstraint(lp.dir(), VRConstraint::LINE);
    c->setTConstraint(lp.pos(), VRConstraint::POINT);
    //c->setTConstraint(Vec3f(0,0,0), VRConstraint::POINT);
    c->toggleRConstraint(true, o2);
    c->toggleTConstraint(true, o2);
}

/**

This is about one joint - one VRJointTool per joint

- body 1
- body 2
- constraint
 - joint type
 - joint parameter (axis, ...)
 - joint ranges (min/max)

Interaction:
- click on obj1
- click on obj2
-> both intersection points will define two planes in space
 -> plane to plane intersection are the hinge axis!

**/

OSG_END_NAMESPACE
