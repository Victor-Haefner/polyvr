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

int VRJointTool::append(VRTransformPtr t, pose p) {
    if (lastAppended) {
        obj1 = t;
        anchor1 = p;
    } else {
        obj2 = t;
        anchor2 = p;
    }

    lastAppended = !lastAppended;
    update();
    return lastAppended ? 2 : 1;
}

void VRJointTool::update() {
    Vec3f r(1,0,0);
    Vec3f g(0,1,0);
    Vec3f y(1,1,0);

    Vec3f ad1 = anchor1.dir();
    Vec3f ad2 = anchor2.dir();
    Vec3f ap1 = anchor1.pos();
    Vec3f ap2 = anchor2.pos();

    // plane plane intersection axis
    /*Plane pl1(ad1, ap1);
    Plane pl2(ad2, ap2);
    Line axis;
    pl1.intersect(pl2, axis);
    ap3 = axis.getPosition();
    ad3 = axis.getDirection();*/

    // axis axis intersection axis
    Vec3f ad3 = ad1.cross(ad2);
    Vec3f ap3;
    Matrix m;
    m[0] = Vec4f(ad1); m[1] = Vec4f(ad2); m[2] = Vec4f(ad3);
    m.invert();
    m.mult(ap2-ap1, ap3);
    ap3 = ap1+ad1*ap3[0];

    setVector(0, ap1, ad1, r, "p1");
    setVector(1, ap2, ad2, g, "p2");
    setVector(2, ap3, ad3, y, "a");

    VRTransformPtr o1 = obj1.lock();
    VRTransformPtr o2 = obj2.lock();
    if (!o1 || !o2) return;

    Matrix L; L.setTranslate(ap3);
    Matrix A = o1->getWorldMatrix();
    A.invert();
    A.mult(L);

    auto c = o2->getConstraint();
    c->setReferential(o1);
    c->setReferenceB(L);
    c->setRConstraint(ad3, VRConstraint::LINE);
    //c->setTConstraint(lp.pos(), VRConstraint::POINT);
    c->setTConstraint(Vec3f(0,0,0), VRConstraint::POINT);
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
