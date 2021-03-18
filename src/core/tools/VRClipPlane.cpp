#include "VRClipPlane.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"

#include <OpenSG/OSGMatrix.h>

using namespace OSG;


VRClipPlane::VRClipPlane(string name) : VRGeometry(name) {
    // init plane geometry
    type = "ClipPlane";
    setPrimitive("Plane 0.2 0.2 1 1");
    VRMaterialPtr m = VRMaterial::create("clipPlane");
    setMaterial(m);

    //m->setWireFrame(true);
#ifndef OSG_OGL_ES2
    m->setFrontBackModes(GL_LINE, GL_FILL);
#endif
    m->setLit(false);
    m->setDiffuse(Color3f(1,1,1));
    m->setLineWidth(3);

    clipShaderUpdate = VRUpdateCb::create("clipShaderUpdate", bind(&VRClipPlane::updateShader, this));
    VRScene::getCurrent()->addUpdateFkt(clipShaderUpdate);
}

VRClipPlane::~VRClipPlane() {
    deactivate();
}

VRClipPlanePtr VRClipPlane::ptr() { return static_pointer_cast<VRClipPlane>( shared_from_this() ); }
VRClipPlanePtr VRClipPlane::create(string name) {
    auto p = shared_ptr<VRClipPlane>(new VRClipPlane(name) );
    p->setVisible(false);
    // make it dragable first
    for (auto d : VRSetup::getCurrent()->getDevices()) {
        VRDevicePtr dev = d.second;
        dev->addDynTree(p, -1);
    }
    return p;
}

// see shader usage example in VRPlanet shader
void VRClipPlane::updateShader() {
    if (!active) return;

    Vec4f plane = Vec4f(getEquation());

    for (auto& m : mats) {
        if (!m->getShaderProgram()) continue;
        m->setShaderParameter("clipPlane", plane);
        m->enableShaderParameter("OSGWorldMatrix");
    }
}

void VRClipPlane::setSize(float W, float H) {
    setPrimitive("Plane " + toString(W) + " " + toString(H) + " 1 1");
}

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

Vec4d VRClipPlane::getEquation() {
    Vec4d plane = Vec4d(0,0,-1,0);
    Matrix4d m = getWorldMatrix();
    m.mult(plane,plane);
    plane[3] = -plane.dot(m[3]);//n*r
    return plane;
}

void VRClipPlane::activate() {
    vector<VRObjectPtr> objs = tree->getChildren(true, "Geometry", true);
    for (auto o : objs) {
        if (o->getType() == "ClipPlane") continue;
        VRGeometryPtr g = static_pointer_cast<VRGeometry>(o);
        VRMaterialPtr m = g->getMaterial();
        if (m == 0) continue;
        m->setClipPlane(true, Vec4d(0,0,-1,0), ptr() );
        mats.push_back(m);
    }
}

void VRClipPlane::deactivate() {
    for (auto m : mats) m->setClipPlane(false, Vec4d(), 0);
    mats.clear();
}

