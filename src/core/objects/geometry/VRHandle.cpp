#include "VRHandle.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunction.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRConstraint.h"
#include <boost/bind.hpp>

using namespace OSG;

VRHandle::VRHandle(string name) : VRGeometry(name) {
    updateCb = VRFunction<int>::create("handle_update", boost::bind(&VRHandle::updateHandle, this) );
    setPickable(true);
    setPrimitive("Box", "0.1 0.1 0.1 1 1 1");
    auto m = VRMaterial::get("VRHandle");
    m->setDiffuse(Vec3f(0.3,0.6,1.0));
    setMaterial( m );
}

VRHandlePtr VRHandle::create(string name) { return VRHandlePtr( new VRHandle(name) ); }
VRHandlePtr VRHandle::ptr() { return static_pointer_cast<VRHandle>( shared_from_this() ); }

void VRHandle::configure(VRAnimPtr cb, TYPE t, Vec3f n, float s, bool symmetric) {
    axis = n;
    paramCb = cb;
    constraint = t;
    scale = s;
    if (t == LINEAR) { // TODO: need local constraints!
        setTConstraint(n);
        setRConstraint(Vec3i(1,1,1));
        setTConstraintMode(VRConstraint::LINE);
        setRestrictionReferential( dynamic_pointer_cast<VRTransform>(getParent()) );
    }
}

void VRHandle::set(pose p, float v) {
    value = v;
    origin = p;

    toggleRConstraint(0);
    toggleTConstraint(0);
    setPose( origin );


    if (constraint == LINEAR) {
        translate( axis*value*scale );
        cout << "VRHandle::set " << getFrom() << endl;
        toggleTConstraint(1);
        toggleRConstraint(1);
        setRestrictionReference(getMatrix());
    }
}

void VRHandle::updateHandle() {
    if (!paramCb) return;

    VRGeometryPtr p =  dynamic_pointer_cast<VRGeometry>(getDragParent());
    if (!p) return;

    Matrix p0 = p->getWorldMatrix();
    p0.invert();
    Pnt3f p1 = getWorldPosition();
    p0.mult(p1,p1);

    Vec3f d = Vec3f(p1)-origin.pos();
    float v = axis.dot(d);
    value = abs(v)/scale;

    (*paramCb)(value);
}

void VRHandle::drag(VRTransformPtr new_parent) {
    VRTransform::drag(new_parent);
    auto scene = VRSceneManager::getCurrent();
    scene->addUpdateFkt( updateCb );
}

void VRHandle::drop() {
    VRTransform::drop();
    auto scene = VRSceneManager::getCurrent();
    scene->dropUpdateFkt( updateCb );
}

