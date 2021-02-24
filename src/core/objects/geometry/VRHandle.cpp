#include "VRHandle.h"
#include "core/scene/VRScene.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRGlobals.h"
#include "core/objects/material/VRMaterial.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/math/pose.h"

using namespace OSG;


VRHandle::VRHandle(string name) : VRGeometry(name) {
    type = "Handle";
    updateCb = VRUpdateCb::create("handle_update", bind(&VRHandle::updateHandle, this, true) );
    setPickable(true);
    setPrimitive("Box 0.1 0.1 0.1 1 1 1");
    auto m = VRMaterial::get("VRHandle");
    m->setDiffuse(Color3f(0.3,0.6,1.0));
    setMaterial( m );
}

VRHandlePtr VRHandle::create(string name) { return VRHandlePtr( new VRHandle(name) ); }
VRHandlePtr VRHandle::ptr() { return static_pointer_cast<VRHandle>( shared_from_this() ); }

void VRHandle::configure(VRAnimCbPtr cb, TYPE t, Vec3d n, float s) {
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

void VRHandle::addSibling(VRHandlePtr h) { siblings.push_back(h); }

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

void VRHandle::setSize(float size) {
    string s = toString(size);
    setPrimitive("Box "+s+" "+s+" "+s+" 1 1 1");
}

bool lockHandle = false; // TODO: remove global lock

void VRHandle::updateHandle(bool sceneUpdate) {
    if (!paramCb) return;
    VRGeometryPtr p =  dynamic_pointer_cast<VRGeometry>(getDragParent());
    if (!p) return;

    if (lockHandle) return;
    lockHandle = true; // WARNING! do not return after this line!

    PosePtr pi = p->getPoseTo(ptr());
    Vec3d d = pi->pos() - origin->pos();
    float v = axis.dot(d);
    value = abs(v)/scale;
    (*paramCb)(value);

    for (auto hw : siblings) {
        if (auto h = hw.lock()) {
            Vec3d pos = h->origin->pos() + h->axis*h->scale*v/scale;
            //cout << "VRHandle::updateHandle sibling v: " << v << " scale: " << scale << " hscale: " << h->scale << " haxis: " << h->axis << " pos: " << pos << endl;
            h->setFrom(pos);
        }
    }

    lockHandle = false;
}

void VRHandle::drag(VRTransformPtr new_parent, VRIntersection i) {
    lockHandle = true;
    VRTransform::drag(new_parent, i);
    auto scene = VRScene::getCurrent();
    scene->addUpdateFkt( updateCb );
    lockHandle = false;
}

void VRHandle::drop() {
    lockHandle = true;
    VRTransform::drop();
    auto scene = VRScene::getCurrent();
    scene->dropUpdateFkt( updateCb );
    lockHandle = false;
}

void VRHandle::setMatrix(Matrix4d m) { // for undo/redo, PROBLEM: called by the constraint non stop
    VRTransform::setMatrix(m);
    if (lockHandle) return;
    //printBacktrace();
    if (isUndoing()) updateHandle(); // problem: called non stop :(
}
