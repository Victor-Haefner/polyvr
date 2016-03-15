#include "VRHandle.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/VRFunction.h"
#include <boost/bind.hpp>

using namespace OSG;

VRHandle::VRHandle(string name) : VRGeometry(name) {
    updateCb = VRFunction<int>::create("handle_update", boost::bind(&VRHandle::update, this) );
    setPickable(true);
    setPrimitive("Sphere", "0.2 1");
}

VRHandlePtr VRHandle::create(string name) { return VRHandlePtr( new VRHandle(name) ); }
VRHandlePtr VRHandle::ptr() { return static_pointer_cast<VRHandle>( shared_from_this() ); }

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

void VRHandle::update() {
    if (!paramCb) return;
    (*paramCb)(value);
}

void VRHandle::configure(VRAnimPtr cb, TYPE t, Vec3f n, bool symmetric) {
    axis = n;
    paramCb = cb;
    if (t == LINEAR) {
        setTConstraint(n);
        setTConstraintMode(VRTransform::LINE);
    }
}

void VRHandle::set(pose p, float v) {
    value = v;
    origin = p;

    if (constraint == LINEAR) {
        setPose( origin );
        translate( axis*value );
    }
}
