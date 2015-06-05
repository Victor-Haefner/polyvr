#include "VRConstructionKit.h"
#include "VRSnappingEngine.h"
#include "VRSelector.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRSignal.h"

OSG_BEGIN_NAMESPACE;

void VRConstructionKit_on_snap(VRConstructionKit* kit, VRSnappingEngine::EventSnap* e);

VRConstructionKit::VRConstructionKit() {
    snapping = new VRSnappingEngine();
    selector = new VRSelector();

    auto fkt = new VRFunction<VRSnappingEngine::EventSnap*>("on_snap_callback", boost::bind(VRConstructionKit_on_snap, this, _1));
    snapping->getSignalSnap()->add(fkt);
}

VRSnappingEngine* VRConstructionKit::getSnappingEngine() { return snapping; }
VRSelector* VRConstructionKit::getSelector() { return selector; }

void VRConstructionKit_on_snap(VRConstructionKit* kit, VRSnappingEngine::EventSnap* e) {
    if (e->o1 == 0 || e->o2 == 0) return;
    VRObject* p1 = e->o1->getDragParent();
    VRObject* p2 = e->o2->getParent();
    if (p1 == 0 || p2 == 0) return;
    if (p1 == p2) if (p1->hasAttachment("kit_group")) return;

    if (p2->hasAttachment("kit_group")) {
        e->o1->rebaseDrag(p2);
        e->o1->setPickable(false);
        e->o2->setPickable(false);
        e->o1->setWorldMatrix(e->m);
        return;
    }

    VRTransform* group = new VRTransform("kit_group");
    group->addAttachment("dynamicaly_generated", 0);
    group->setPickable(true);
    group->addAttachment("kit_group", 0);
    p2->addChild(group);
    e->o1->rebaseDrag(group);
    Matrix m = e->o2->getWorldMatrix();
    e->o2->switchParent(group);
    e->o1->setPickable(false);
    e->o2->setPickable(false);
    e->o1->setWorldMatrix(e->m);
    e->o2->setWorldMatrix(m);
}

int VRConstructionKit::ID() {
    static int i = 0; i++;
    return i;
}

void VRConstructionKit::breakup(VRObject* obj) {
    if (obj == 0) return;
    auto p = obj->getParent();
    if (!p->hasAttachment("kit_group")) return;
    p = p->getParent();
    obj->switchParent(p);
    obj->setPickable(true);
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
