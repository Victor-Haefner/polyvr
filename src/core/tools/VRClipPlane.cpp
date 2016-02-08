#include "VRClipPlane.h"
#include "core/objects/material/VRMaterial.h"

#include <OpenSG/OSGMatrix.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRClipPlane::VRClipPlane(string name) : VRGeometry(name) {
    // init plane geometry
    type = "ClipPlane";
    setPrimitive("Plane", "0.2 0.2 1 1");
    VRMaterialPtr m = VRMaterial::create("clipPlane");
    setMaterial(m);
    setVisible(false);

    //m->setWireFrame(true);
    m->setFrontBackModes(GL_LINE, GL_FILL);
    m->setLit(false);
    m->setDiffuse(Vec3f(1,1,1));
    m->setLineWidth(3);
}

VRClipPlane::~VRClipPlane() {
    deactivate();
}

VRClipPlanePtr VRClipPlane::create(string name) { return shared_ptr<VRClipPlane>(new VRClipPlane(name) ); }
VRClipPlanePtr VRClipPlane::ptr() { return static_pointer_cast<VRClipPlane>( shared_from_this() ); }

void VRClipPlane::setTree(VRObjectPtr obj) {
    if (tree == obj) return;
    tree = obj;
    deactivate();
    if (tree && active) activate();
}

bool VRClipPlane::isActive() { return active; }

void VRClipPlane::setActive(bool a) {
    if (active == a) return;

    active = a;
    setVisible(active);
    if (tree == 0) return;

    if (active) activate();
    else deactivate();
}

Vec4f VRClipPlane::getEquation() { // not used, but may come in handy
    Vec4f plane = Vec4f(0.0, -1.0, 0.0, 0.0);
    Matrix m = getWorldMatrix();
    m.mult(plane,plane);
    plane[3] = -plane.dot(m[3]);//n*r
    return plane;
}

void VRClipPlane::activate() {
    vector<VRObjectPtr> objs = tree->getChildren(true, "Geometry");
    for (auto o : objs) {
        if (o->getType() == "ClipPlane") continue;
        VRGeometryPtr g = static_pointer_cast<VRGeometry>(o);
        VRMaterialPtr m = g->getMaterial();
        if (m == 0) continue;
        m->setClipPlane(true, Vec4f(0,0,-1,0), ptr() );
        mats.push_back(m);
    }
}

void VRClipPlane::deactivate() {
    for (auto m : mats) m->setClipPlane(false, Vec4f(), 0);
    mats.clear();
}

OSG_END_NAMESPACE;
