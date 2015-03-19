#include "VRClipPlane.h"
#include "core/objects/material/VRMaterial.h"

#include <OpenSG/OSGMatrix.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRClipPlane::VRClipPlane(string name) : VRGeometry(name) {
    // init plane geometry
    type = "ClipPlane";
    setPrimitive("Plane", "0.2 0.2 1 1");
    VRMaterial* m = new VRMaterial("clipPlane");
    setMaterial(m);
    setVisible(false);

    //m->setWireFrame(true);
    m->setFrontBackModes(GL_LINE, GL_FILL);
    m->setLit(false);
    m->setDiffuse(Vec3f(1,1,1));
    m->setLineWidth(3);
}

void VRClipPlane::setTree(VRObject* obj) {
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
    vector<VRObject*> objs = tree->getChildren(true, "Geometry");
    for (auto o : objs) {
        if (o->getType() == "ClipPlane") continue;
        VRGeometry* g = (VRGeometry*)o;
        VRMaterial* m = g->getMaterial();
        m->setClipPlane(true, Vec4f(0,0,-1,0), this);
        mats.push_back(m);
    }
}

void VRClipPlane::deactivate() {
    for (auto m : mats) m->setClipPlane(false, Vec4f(), 0);
    mats.clear();
}

OSG_END_NAMESPACE;
