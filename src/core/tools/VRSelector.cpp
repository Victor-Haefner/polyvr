#include "VRSelector.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSelector::VRSelector() {;}

VRMaterial* VRSelector::getMat() {
    VRMaterial* mat = new VRMaterial("VRSelector");
    mat->setFrontBackModes(GL_NONE, GL_LINE);
    mat->setDiffuse(Vec3f(1,0,0));
    mat->setLineWidth(5);
    mat->setLit(false);
    return mat;
}

void VRSelector::deselect() {
    if (selection == 0) return;

    vector<VRObject*> geos = selection->filterByType("Geometry");
    for (auto o : geos) {
        VRGeometry* g = (VRGeometry*)o;
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

    vector<VRObject*> geos = obj->filterByType("Geometry");
    for (auto o : geos) {
        VRGeometry* g = (VRGeometry*)o;
        orig_mats[g] = g->getMaterial();
        VRMaterial* mat = getMat();
        mat->appendPasses(orig_mats[g]);
        g->setMaterial(mat);
    }
}

VRObject* VRSelector::get() { return selection; }

OSG_END_NAMESPACE;
