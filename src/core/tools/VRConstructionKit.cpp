#include "VRConstructionKit.h"
#include "VRSnappingEngine.h"
#include "VRSelector.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"

OSG_BEGIN_NAMESPACE;

VRConstructionKit::VRConstructionKit() {
    snapping = new VRSnappingEngine();
    selector = new VRSelector();
}

VRSnappingEngine* VRConstructionKit::getSnappingEngine() { return snapping; }
VRSelector* VRConstructionKit::getSelector() { return selector; }

int VRConstructionKit::ID() {
    static int i = 0; i++;
    return i;
}

int VRConstructionKit::addAnchorType(float size, Vec3f color) {
    auto g = new VRGeometry("anchor");
    string bs = toString(size);
    g->setPrimitive("Box", bs + " " + bs + " " + bs + " 1 1 1");
    auto m = new VRMaterial("anchor");
    m->setDiffuse(color);
    g->setMaterial(m);
    int id = ID();
    anchors[id] = g;
    return id;
}

void VRConstructionKit::addObject(VRTransform* t) {
    objects[t] = t;
    snapping->addObject(t);
}

void VRConstructionKit::addObjectAnchor(VRTransform* t, int a, Vec3f pos, float radius) {
    VRGeometry* anc = (VRGeometry*)anchors[a]->duplicate();
    anc->setPose(pos, Vec3f(0,1,0), Vec3f(1,0,0));
    anc->show();
    anc->switchParent(t);
    snapping->addObjectAnchor(t, anc);
    snapping->addRule(VRSnappingEngine::POINT, VRSnappingEngine::POINT, Line(Pnt3f(pos), Vec3f(0,0,0)), Line(Pnt3f(0,1,0), Vec3f(1,0,0)), radius, 1, t );
}

OSG_END_NAMESPACE;
