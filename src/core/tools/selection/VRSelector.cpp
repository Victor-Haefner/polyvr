#include "VRSelector.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"

//#include <OpenSG/OSGMaterial.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSelector::VRSelector() { color = Vec3f(0.2, 0.65, 0.9); }

VRMaterialPtr VRSelector::getMat() {
    VRMaterialPtr mat = VRMaterial::create("VRSelector");

    // stencil buffer
    mat->setFrontBackModes(GL_POINT, GL_POINT);
    mat->setDiffuse(color);
    mat->setPointSize(8);
    mat->setLit(false);
    //mat->setStencilBuffer(false, 1,-1, GL_NOTEQUAL, GL_KEEP, GL_KEEP, GL_REPLACE);

    mat->addPass();

    mat->setFrontBackModes(GL_LINE, GL_LINE);
    mat->setDiffuse(color);
    mat->setLineWidth(8);
    mat->setLit(false);
    //mat->setStencilBuffer(false, 1,-1, GL_NOTEQUAL, GL_KEEP, GL_KEEP, GL_REPLACE);

    mat->addPass();

    mat->setDiffuse(color);
    mat->setLit(false);

    return mat;
}

void VRSelector::setColor(Vec3f c) { color = c; }

void VRSelector::deselect() {
    if (selection == 0) return;
    clearSubselection();

    vector<VRGeometryPtr> geos;
    if (selection->hasAttachment("geometry")) geos.push_back(static_pointer_cast<VRGeometry>(selection));

    for (auto o : selection->getChildren(true)) {
        if (o->hasAttachment("geometry")) geos.push_back(static_pointer_cast<VRGeometry>(o));
    }

    for (auto g : geos) {
        if (orig_mats.count(g) == 0) continue;
        g->setMaterial(orig_mats[g]);
    }

    selection = 0;
    orig_mats.clear();
}

void VRSelector::select(VRObjectPtr obj) {
    deselect();
    if (obj == 0) return;
    selection = obj;

    vector<VRGeometryPtr> geos;
    if (obj->hasAttachment("geometry")) geos.push_back(static_pointer_cast<VRGeometry>(obj));

    for (auto o : obj->getChildren(true)) {
        if (o->hasAttachment("geometry")) geos.push_back(static_pointer_cast<VRGeometry>(o));
    }

    for (auto g : geos) {
        orig_mats[g] = g->getMaterial();
        VRMaterialPtr mat = getMat();
        mat->appendPasses(orig_mats[g]);
        //mat->prependPasses(orig_mats[g]);
        mat->setActivePass(0);
        //mat->setStencilBuffer(true, 1,-1, GL_ALWAYS, GL_KEEP, GL_KEEP, GL_REPLACE);
        g->setMaterial(mat);
    }
}

VRObjectPtr VRSelector::getSelection() { return selection; }

void VRSelector::subselect(vector<int> verts, bool add) {
    hasSubselection = true;
    if (add) for (auto &v : verts) subselection[v] = 1;
    else for (auto &v : verts) subselection.erase(v);
}

vector<int> VRSelector::getSubselection() {
    auto res = vector<int>();
    if (!hasSubselection) return res;
    res.reserve(subselection.size());
    for (auto &s : subselection) res.push_back( s.first );
    return res;
}

void VRSelector::clearSubselection() {
    subselection.clear();
    hasSubselection = false;
}

OSG_END_NAMESPACE;
