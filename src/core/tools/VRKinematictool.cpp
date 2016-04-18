#include "VRKinematictool.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRTransform.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRSceneManager.h"
#include "VRAnalyticGeometry.h"

#include <OpenSG/OSGPlane.h>
#include <OpenSG/OSGLine.h>
#include <boost/bind.hpp>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRJointTool::VRJointTool(string name) : VRGeometry(name) {
    type = "JointTool";
    setPrimitive("Cylinder", "0.05 0.1 16 0 0 1");

    auto mat = VRMaterial::create("VRJointTool");
    mat->setDiffuse(Vec3f(0,0,0));
    mat->setWireFrame(1);
    mat->setLineWidth(4);
    mat->setLit(0);
    setMaterial(mat);

    ageo = VRAnalyticGeometry::create();
    ageo->setLabelParams(0.05, true, true);
    ageo->setPersistency(0);

    store("jt_anchor1", &anchor1);
    store("jt_anchor2", &anchor2);
    storeObjName("jt_obj1_name", &obj1, &obj1_name);
    storeObjName("jt_obj2_name", &obj2, &obj2_name);
    //store("jt_active", &active);
    //store("jt_lastAppended", &lastAppended);

    regStorageSetupAfterFkt( VRFunction<int>::create("jointtool setup", boost::bind(&VRJointTool::delayed_setup, this)) );
}

VRJointTool::~VRJointTool() { clear(); }

VRJointToolPtr VRJointTool::ptr() { return static_pointer_cast<VRJointTool>( shared_from_this() ); }
VRJointToolPtr VRJointTool::create(string name) {
    auto ptr = shared_ptr<VRJointTool>(new VRJointTool(name) );
    ptr->addChild(ptr->ageo);
    ptr->ageo->init();
    return ptr;
}

void VRJointTool::delayed_setup() {
    obj1 = dynamic_pointer_cast<VRTransform>( getRoot()->find(obj1_name) );
    obj2 = dynamic_pointer_cast<VRTransform>( getRoot()->find(obj2_name) );
    updateVis();
}

void VRJointTool::clear() {
    setActive(0);
    obj1.reset();
    obj2.reset();
    ageo->hide();
}

int VRJointTool::append(VRTransformPtr t, pose p) {
    if (lastAppended) {
        obj1 = t;
        obj1_name = t->getName();
        anchor1 = p;
    } else {
        obj2 = t;
        obj2_name = t->getName();
        anchor2 = p;
    }

    lastAppended = !lastAppended;
    updateVis();
    return lastAppended ? 2 : 1;
}

void VRJointTool::setActive(bool b) { active = b; updateVis(); }
void VRJointTool::select(bool b) { selected = b; updateVis(); }

void VRJointTool::updateVis() {
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

    VRTransformPtr o1 = obj1.lock();
    VRTransformPtr o2 = obj2.lock();

    if (selected) {
        ageo->show();
        if (o1) ageo->setVector(0, ap1, ad1, g, "p1");
        if (o2) ageo->setVector(1, ap2, ad2, r, "p2");
        if (o2) ageo->setVector(2, ap3, ad3, y, "a");
    } else ageo->hide();

    float D = (ad1.cross(ad3)).squareLength();
    if (D > 0) setPose(ap3, ad1, ad3);
    else setFrom(ap3);
    m = getMatrix(); m.invert();
    ageo->setMatrix(m);

    auto mat = getMaterial();
    mat->setDiffuse(Vec3f(1,0,0));
    if (o1 && !o2) mat->setDiffuse(Vec3f(1,0.8,0));

    if (!o1 || !o2 || o1 == o2) return;
    mat->setDiffuse(Vec3f(0,1,0));

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
    c->toggleRConstraint(active, o2);
    c->toggleTConstraint(active, o2);
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
