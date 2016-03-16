#include "VRHandle.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunction.h"
#include <boost/bind.hpp>

using namespace OSG;

VRHandle::VRHandle(string name) : VRGeometry(name) {
    updateCb = VRFunction<int>::create("handle_update", boost::bind(&VRHandle::updateHandle, this) );
    setPickable(true);
    setPrimitive("Sphere", "0.1 1");
}

VRHandlePtr VRHandle::create(string name) { return VRHandlePtr( new VRHandle(name) ); }
VRHandlePtr VRHandle::ptr() { return static_pointer_cast<VRHandle>( shared_from_this() ); }

void VRHandle::configure(VRAnimPtr cb, TYPE t, Vec3f n, float s, bool symmetric) {
    axis = n;
    paramCb = cb;
    constraint = t;
    scale = s;
    if (t == LINEAR) { // TODO: need local constraints!
        //setTConstraint(n);
        //setTConstraintMode(VRTransform::LINE);
    }
}

void VRHandle::set(pose p, float v) {
    value = v;
    origin = p;

    setPose( origin );
    if (constraint == LINEAR) translate( axis*value*scale );
}

void VRHandle::updateHandle() {
    if (!paramCb) return;

    VRGeometryPtr p =  dynamic_pointer_cast<VRGeometry>(getDragParent());
    if (!p) return;

    Vec3f p0 = p->getWorldPosition(); // TODO, use origin offset
    Vec3f p1 = getWorldPosition();
    Vec3f d = p1-p0;
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

