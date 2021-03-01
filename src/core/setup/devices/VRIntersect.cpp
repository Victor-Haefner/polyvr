#include "VRIntersect.h"
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGIntersectAction.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGTriangleIterator.h>

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/OSGObject.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"
#include "VRSignal.h"
#include "VRDevice.h"

using namespace OSG;

VRObjectPtr VRIntersection::getIntersected() { return object.lock(); }
Pnt3d VRIntersection::getIntersection() { return point; }


Vec2d VRIntersect_computeTexel(VRIntersection& ins, NodeMTRecPtr node) {
    if (!ins.hit) return Vec2d(0,0);
    if (node == 0) return Vec2d(0,0);

    GeometryRefPtr geo = dynamic_cast<Geometry*>( node->getCore() );
    if (geo == 0) return Vec2d(0,0);
    auto type = geo->getTypes()->getValue(0);
    if ( type == GL_PATCHES ) return Vec2d(0,0);

    auto texcoords = geo->getTexCoords();
    if (texcoords == 0) return Vec2d(0,0);
    TriangleIterator iter = geo->beginTriangles(); iter.seek( ins.triangle );


    Matrix4f m = node->getToWorld();
    m.invert();
    Pnt3f local_pnt; m.mult(Pnt3f(ins.point), local_pnt);

    Pnt3f p0 = iter.getPosition(0);
    Pnt3f p1 = iter.getPosition(1);
    Pnt3f p2 = iter.getPosition(2);
    Vec3f cr = (p1 - p0).cross(p2 - p0);
    Vec3f n = cr; n.normalize();

    float areaABC = n.dot(cr);
    float areaPBC = n.dot((p1 - local_pnt).cross(p2 - local_pnt));
    float areaPCA = n.dot((p2 - local_pnt).cross(p0 - local_pnt));
    float a = areaPBC / areaABC;
    float b = areaPCA / areaABC;
    float c = 1.0f - a - b;

    return Vec2d( iter.getTexCoords(0) * a + iter.getTexCoords(1) * b + iter.getTexCoords(2) * c );
}

Vec3i VRIntersect_computeVertices(VRIntersection& ins, NodeMTRecPtr node) {
    if (!ins.hit) return Vec3i(0,0,0);
    if (node == 0) return Vec3i(0,0,0);

    GeometryRefPtr geo = dynamic_cast<Geometry*>( node->getCore() );
    if (geo == 0) return Vec3i(0,0,0);
    auto type = geo->getTypes()->getValue(0);
    if ( type == GL_PATCHES ) return Vec3i(0,0,0);

    TriangleIterator iter = geo->beginTriangles(); iter.seek( ins.triangle );
    return Vec3i(iter.getPositionIndex(0), iter.getPositionIndex(1), iter.getPositionIndex(2));
}

VRIntersection VRIntersect::intersectRay(VRObjectWeakPtr wtree, Line ray, bool skipVols) {
    VRIntersection ins;
    auto tree = wtree.lock();
    if (!tree) return ins;
    if (!tree->getNode()) return ins;
    if (!tree->getNode()->node) return ins;

    unsigned int now = VRGlobals::CURRENT_FRAME;

    VRIntersectAction iAct;
    iAct.setSkipVolumes(skipVols);
    iAct.setTravMask(8);
    iAct.setLine(ray);
    iAct.apply(tree->getNode()->node);

    ins.ray = ray;
    ins.hit = iAct.didHit();
    if (ins.hit) {
        ins.object = tree->find(OSGObject::create(iAct.getHitObject()->getParent()));
        if (auto sp = ins.object.lock()) ins.name = sp->getName();
        ins.point = Pnt3d(iAct.getHitPoint());
        ins.normal = Vec3d(iAct.getHitNormal());
        if (tree->getParent()) {
            auto m = toMatrix4d( tree->getParent()->getNode()->node->getToWorld() );
            m.mult( ins.point, ins.point );
            m.mult( ins.normal, ins.normal );
        }
        ins.triangle = iAct.getHitTriangle();
        ins.triangleVertices = VRIntersect_computeVertices(ins, iAct.getHitObject());
        ins.texel = VRIntersect_computeTexel(ins, iAct.getHitObject());
        ins.customID = iAct.getHitLine();
        lastIntersection = ins;
        ins.time = now;
    } else {
        ins.object.reset();
        if (lastIntersection.time < ins.time) lastIntersection = ins;
    }

    intersections[tree.get()] = ins;

    if (showHit) cross->setWorldPosition(Vec3d(ins.point));
    return ins;
}

/**
* @param wtree: root of sub scene graph to be intersected with
* @param force: determines if reevaluation of intersection within a single frame should be forced
* @param caster: beacon which should be used for intersection (for use of multiple beacons with multitouch)
* @param dir: local ray casting vector
*/
VRIntersection VRIntersect::intersect(VRObjectWeakPtr wtree, bool force, VRTransformPtr caster, Vec3d dir, bool skipVols) {
    vector<VRObjectPtr> trees;
    if (auto sp = wtree.lock()) trees.push_back(sp);
    else for (auto grp : dynTrees) {
        for (auto swp : grp.second) if (auto sp = swp.second.lock()) trees.push_back(sp);
    }

    VRIntersection ins;
    VRDevice* dev = (VRDevice*)this;

    if (caster == 0) caster = dev->getBeacon();
    if (caster == 0) { cout << "Warning: VRIntersect::intersect, caster is 0!\n"; return ins; }
    unsigned int now = VRGlobals::CURRENT_FRAME;
    for (auto t : trees) {
        if (intersections.count(t.get())) {
            auto ins_tmp = intersections[t.get()];
            if (ins_tmp.hit && ins_tmp.time == now && !force) {
                ins = ins_tmp;
                break;
            }
        }

        Line ray = caster->castRay(t, dir);
        auto ins_tmp = intersectRay(t, ray, skipVols);
        //if (force) cout << ray.getPosition()[1] << " " << caster->getWorldPosition()[1] << endl;
        if (ins_tmp.hit) return ins_tmp;
        else ins.ray = ray;
    }
    return ins;
}

VRIntersect::VRIntersect() {
#ifndef WASM
    initCross();
#endif
    drop_fkt = VRFunction<VRDeviceWeakPtr>::create("Intersect_drop", bind(&VRIntersect::drop, this, _1, VRTransformPtr(0)));
    dragged_ghost = VRTransform::create("dev_ghost");
}

VRIntersect::~VRIntersect() {}

void VRIntersect::initIntersect(VRDevicePtr dev) {
    dragSignal = VRSignal::create(dev);
    dropSignal = VRSignal::create(dev);
}

void VRIntersect::dragCB(VRTransformWeakPtr caster, VRObjectWeakPtr tree, VRDeviceWeakPtr dev) {
    VRIntersection ins = intersect(tree);
    drag(ins, caster);
}

void VRIntersect::clearDynTrees() { dynTrees.clear(); }

void VRIntersect::drag(VRIntersection i, VRTransformWeakPtr wcaster) {
    VRObjectWeakPtr wobj = i.object;
    auto obj = wobj.lock();
    auto caster = wcaster.lock();
    if (!obj || !caster) return;

    auto d = getDraggedObject(caster);
    if (obj == 0 || d != 0 || !dnd) return;

    obj = obj->findPickableAncestor();
    if (obj == 0) return;
    if (!obj->hasTag("transform")) return;

    auto dobj = static_pointer_cast<VRTransform>(obj);
    dragged[caster.get()] = dobj;
    dobj->drag(caster, i);

    dragged_ghost->setMatrix(dobj->getMatrix());
    dragged_ghost->switchParent(caster);

    dragSignal->triggerPtr<VRDevice>();
}

void VRIntersect::drop(VRDeviceWeakPtr dev, VRTransformWeakPtr beacon) {
    auto d = getDraggedObject(beacon.lock());
    if (d) {
        d->drop();
        if (d != dragged_ghost) dropSignal->triggerPtr<VRDevice>();
        drop_time = VRGlobals::CURRENT_FRAME;
        dragged.erase(beacon.lock().get());
        if (beacon.lock() == 0) dragged.clear();
    }
}

VRTransformPtr VRIntersect::getDraggedObject(VRTransformPtr beacon) {
    if (beacon == 0) return dragged.begin()->second.lock();
    if (!dragged.count(beacon.get())) return 0;
    return dragged[beacon.get()].lock();
}

VRTransformPtr VRIntersect::getDraggedGhost() { return dragged_ghost; }
VRSignalPtr VRIntersect::getDragSignal() { return dragSignal; }
VRSignalPtr VRIntersect::getDropSignal() { return dropSignal; }

void VRIntersect::initCross() {
    cross = VRGeometry::create("Hit cross");
    cross->hide();
    showHit = false;

    vector<Vec3d> pos, norms;
    vector<Vec2d> texs;
    vector<int> inds;

    float d = 0.1;
    pos.push_back(Vec3d(-d,-d,-d));
    pos.push_back(Vec3d(d,d,d));
    pos.push_back(Vec3d(-d,d,-d));
    pos.push_back(Vec3d(d,-d,d));

    pos.push_back(Vec3d(-d,-d,d));
    pos.push_back(Vec3d(d,d,-d));
    pos.push_back(Vec3d(-d,d,d));
    pos.push_back(Vec3d(d,-d,-d));

    for (int i=0;i<8;i++) {
        norms.push_back(Vec3d(0,0,-1));
        inds.push_back(i);
        texs.push_back(Vec2d(0,0));
    }

    VRMaterialPtr mat = VRMaterial::create("red_cross");
    mat->setDiffuse(Color3f(1,0,0));
    cross->create(GL_LINES, pos, norms, inds, texs);
    cross->setMaterial(mat);
}

VRDeviceCbPtr VRIntersect::addDrag(VRTransformWeakPtr caster) {
    return addDrag(caster, VRObjectWeakPtr());
}

VRDeviceCbPtr VRIntersect::addDrag(VRTransformWeakPtr caster, VRObjectWeakPtr tree) {
    auto sc = caster.lock();
    auto st = tree.lock();
    if (!sc) return 0;

    auto key = sc.get();
    if (int_fkt_map.count(key)) return int_fkt_map[key];

    VRDeviceCbPtr fkt = VRFunction<VRDeviceWeakPtr>::create("Intersect_drag", bind(&VRIntersect::dragCB, this, caster, tree, _1));
    dra_fkt_map[key] = fkt;
    return fkt;
}

void VRIntersect::toggleDragnDrop(bool b) { dnd = b;}
void VRIntersect::showHitPoint(bool b) {//needs to be optimized for multiple scenes
    showHit = b;
    if (b) cross->show();
    else cross->hide();
}

void VRIntersect::addDynTree(VRObjectPtr o, int prio) {
    if (!o) return;
    if (!dynTrees.count(prio)) dynTrees[prio] = map<VRObject*, VRObjectWeakPtr>();
    dynTrees[prio][ o.get() ] = o;
}

void VRIntersect::remDynTree(VRObjectPtr o) {
    if (!o) return;
    for (auto grp : dynTrees) {
        if (grp.second.count( o.get())) {
            grp.second.erase( o.get() );
            return;
        }
    }
}

VRObjectPtr VRIntersect::getCross() { return cross; }//needs to be optimized for multiple scenes
VRDeviceCbPtr VRIntersect::getDrop() { return drop_fkt; }

VRIntersection VRIntersect::getLastIntersection() { return lastIntersection; }

