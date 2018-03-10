#include "VRConstructionKit.h"
#include "selection/VRSelector.h"

#include <boost/bind.hpp>

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRSignal.h"

OSG_BEGIN_NAMESPACE;

VRConstructionKit::VRConstructionKit() {
    snapping = VRSnappingEngine::create();
    selector = VRSelector::create();

    auto fkt = new VRFunction<VRSnappingEngine::EventSnap*>("on_snap_callback", boost::bind(&VRConstructionKit::on_snap, this, _1));
    snapping->getSignalSnap()->add(fkt);
}

VRConstructionKit::~VRConstructionKit() {}

VRConstructionKitPtr VRConstructionKit::create() { return VRConstructionKitPtr(new VRConstructionKit()); }

void VRConstructionKit::clear() {
    objects.clear();
    selector->clear();
    snapping->clear();
}

VRSnappingEnginePtr VRConstructionKit::getSnappingEngine() { return snapping; }
VRSelectorPtr VRConstructionKit::getSelector() { return selector; }

vector<VRObjectPtr> VRConstructionKit::getObjects() {
    vector<VRObjectPtr> res;
    for (auto m : objects) res.push_back(m.second);
    return res;
}

void VRConstructionKit::on_snap(VRSnappingEngine::EventSnap* e) {
    if (!doConstruction) return;
    if (e->snap == 0) { breakup(e->o1); return; }

    if (e->o1 == 0 || e->o2 == 0) return;
    VRObjectPtr p1 = e->o1->getDragParent();
    VRObjectPtr p2 = e->o2->getParent();
    if (p1 == 0 || p2 == 0) return;
    if (p1 == p2) if (p1->hasTag("kit_group")) return;

    if (p2->hasTag("kit_group")) {
        e->o1->rebaseDrag(p2);
        e->o1->setPickable(false);
        e->o2->setPickable(false);
        e->o1->setWorldMatrix(e->m);
        return;
    }

    VRTransformPtr group = VRTransform::create("kit_group");
    group->setPersistency(0);
    group->setPickable(true);
    group->addAttachment("kit_group", 0);
    p2->addChild(group);
    e->o1->rebaseDrag(group);
    Matrix4d m = e->o2->getWorldMatrix();
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

void VRConstructionKit::breakup(VRTransformPtr obj) {
    if (!doConstruction) return;
    if (obj == 0) return;

    auto p = obj->getParent();
    if (p == 0) return;
    if (p->hasTag("kit_group")) {
        obj->switchParent( p->getParent() );
        obj->setPickable(true);
        return;
    }

    p = obj->getDragParent();
    if (p == 0) return;
    if (p->hasTag("kit_group")) {
        obj->rebaseDrag( p->getParent() );
        obj->setPickable(true);
    }
}

int VRConstructionKit::addAnchorType(float size, Color3f color) {
    auto g = VRGeometry::create("anchor");
    string bs = toString(size);
    g->setPrimitive("Box", bs + " " + bs + " " + bs + " 1 1 1");
    auto m = VRMaterial::create("anchor");
    m->setDiffuse(color);
    g->setMaterial(m);
    int id = ID();
    anchors[id] = g;
    return id;
}

void VRConstructionKit::toggleConstruction(bool active) {
    doConstruction = active;
}

void VRConstructionKit::addObject(VRTransformPtr t) {
    objects[t.get()] = t;
    snapping->addObject(t);
}

void VRConstructionKit::remObject(VRTransformPtr t) {
    objects.erase(t.get());
    snapping->remObject(t);
}

VRGeometryPtr VRConstructionKit::addObjectAnchor(VRTransformPtr t, int a, Vec3d pos, float radius) {
    if (!anchors.count(a)) return 0;
    VRGeometryPtr anc = static_pointer_cast<VRGeometry>(anchors[a]->duplicate());
    anc->setTransform(pos, Vec3d(0,1,0), Vec3d(1,0,0));
    anc->show();
    anc->switchParent(t);
    snapping->addObjectAnchor(t, anc);
    snapping->addRule(VRSnappingEngine::POINT, VRSnappingEngine::POINT, Line(Pnt3f(0,0,0), Vec3f(0,0,0)), Line(Pnt3f(0,1,0), Vec3f(1,0,0)), radius, 1, t );
    return anc;
}

OSG_END_NAMESPACE;
