#include "VRHandle.h"
#include "core/scene/VRScene.h"
#include "core/utils/VRFunction.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/math/pose.h"
#include <boost/bind.hpp>

using namespace OSG;

VRHandle::VRHandle(string name) : VRGeometry(name) {
    type = "Handle";
    updateCb = VRUpdateCb::create("handle_update", boost::bind(&VRHandle::updateHandle, this) );
    setPickable(true);
    setPrimitive("Box 0.1 0.1 0.1 1 1 1");
    auto m = VRMaterial::get("VRHandle");
    m->setDiffuse(Color3f(0.3,0.6,1.0));
    setMaterial( m );
}

VRHandlePtr VRHandle::create(string name) { return VRHandlePtr( new VRHandle(name) ); }
VRHandlePtr VRHandle::ptr() { return static_pointer_cast<VRHandle>( shared_from_this() ); }

void VRHandle::configure(VRAnimCbPtr cb, TYPE t, Vec3d n, float s, bool symmetric) {
    axis = n;
    paramCb = cb;
    constraint = t;
    scale = s;
    auto c = getConstraint();
    if (t == LINEAR) {
        c->lockRotation();
        c->setReferential( dynamic_pointer_cast<VRTransform>(getParent()) );
        c->setTConstraint(n, VRConstraint::LINE);
    }
}

void VRHandle::set(PosePtr p, float v) {
    value = v;
    origin = p;

    auto c = getConstraint();
    c->setActive(false);
    setPose( origin );

    if (constraint == LINEAR) {
        translate( axis*value*scale );
        c->setReference(getPose());
        c->setTConstraint(axis, VRConstraint::LINE);
        c->setActive(true);
    }
}

Vec3d VRHandle::getAxis() { return axis; }
PosePtr VRHandle::getOrigin() { return origin; }

void VRHandle::updateHandle() {
    if (!paramCb) return;

    VRGeometryPtr p =  dynamic_pointer_cast<VRGeometry>(getDragParent());
    if (!p) return;

    Matrix4d p0 = p->getWorldMatrix();
    p0.invert();
    Pnt3d p1 = getWorldPosition();
    p0.mult(p1,p1);

    Vec3d d = Vec3d(p1)-origin->pos();
    float v = axis.dot(d);
    value = abs(v)/scale;
    (*paramCb)(value);
}

void VRHandle::drag(VRTransformPtr new_parent) {
    VRTransform::drag(new_parent);
    auto scene = VRScene::getCurrent();
    scene->addUpdateFkt( updateCb );
}

void VRHandle::drop() {
    VRTransform::drop();
    auto scene = VRScene::getCurrent();
    scene->dropUpdateFkt( updateCb );
}

void VRHandle::setMatrix(Matrix4d m) { // for undo/redo
    VRTransform::setMatrix(m);
    (*updateCb)(); // problem: called non stop :(
}
