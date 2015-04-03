#include "VRSelector.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSelector::VRSelector() { color = Vec3f(0.2, 0.65, 0.9); }

VRMaterial* VRSelector::getMat() {
    VRMaterial* mat = new VRMaterial("VRSelector");

    mat->setFrontBackModes(GL_POINT, GL_NONE);
    mat->setDiffuse(color);
    mat->setPointSize(8);
    mat->setLit(false);
    mat->setStencilBuffer(false, 1,-1, GL_NOTEQUAL, GL_KEEP, GL_KEEP, GL_REPLACE);

    mat->addPass();

    mat->setFrontBackModes(GL_LINE, GL_NONE);
    mat->setDiffuse(color);
    mat->setLineWidth(8);
    mat->setLit(false);
    mat->setStencilBuffer(false, 1,-1, GL_NOTEQUAL, GL_KEEP, GL_KEEP, GL_REPLACE);

    return mat;
}

void VRSelector::setColor(Vec3f c) { color = c; }

void VRSelector::deselect() {
    if (selection == 0) return;

    vector<VRGeometry*> geos;
    if (selection->hasAttachment("geometry")) geos.push_back((VRGeometry*)selection);

    for (auto o : selection->getChildren(true)) {
        if (o->hasAttachment("geometry")) geos.push_back((VRGeometry*)o);
    }

    for (auto g : geos) {
        if (orig_mats.count(g) == 0) continue;
        VRMaterial* mat = g->getMaterial();
        g->setMaterial(orig_mats[g]);
        delete mat;
    }

    selection = 0;
    orig_mats.clear();
}

void VRSelector::select(VRObject* obj) {
    deselect();
    if (obj == 0) return;
    selection = obj;

    vector<VRGeometry*> geos;
    if (obj->hasAttachment("geometry")) geos.push_back((VRGeometry*)obj);

    for (auto o : obj->getChildren(true)) {
        if (o->hasAttachment("geometry")) geos.push_back((VRGeometry*)o);
    }

    for (auto g : geos) {
        orig_mats[g] = g->getMaterial();
        VRMaterial* mat = getMat();
        mat->prependPasses(orig_mats[g]);
        mat->setActivePass(0);
        mat->setStencilBuffer(true, 1,-1, GL_ALWAYS, GL_KEEP, GL_KEEP, GL_REPLACE);
        g->setMaterial(mat);
    }
}

VRObject* VRSelector::get() { return selection; }

OSG_END_NAMESPACE;
