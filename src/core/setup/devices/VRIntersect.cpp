#include "VRIntersect.h"
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGIntersectAction.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGTriangleIterator.h>

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRFunction.h"
#include "VRSignal.h"
#include "VRDevice.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

Vec2f VRIntersect_computeTexel(VRIntersection& ins, NodeRecPtr node) {
    if (!ins.hit) return Vec2f(0,0);
    if (node == 0) return Vec2f(0,0);

    GeometryRefPtr geo = dynamic_cast<Geometry*>( node->getCore() );
    if (geo == 0) return Vec2f(0,0);
    TriangleIterator iter = geo->beginTriangles(); iter.seek( ins.triangle );

    Matrix m; node->getToWorld().inverse(m);
    Pnt3f local_pnt; m.mult(ins.point, local_pnt);

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

    return iter.getTexCoords(0) * a + iter.getTexCoords(1) * b + iter.getTexCoords(2) * c;
}

//VRIntersection VRIntersect::intersect(VRTransform* caster, VRObject* tree, VRDevice* dev) {
VRIntersection VRIntersect::intersect(VRObject* tree) {
    VRDevice* dev = (VRDevice*)this;
    VRTransform* caster = dev->getBeacon();
    Line ray = caster->castRay();
    IntersectActionRefPtr iAct = IntersectAction::create();
    iAct->setLine(ray);

    if (tree == 0) tree = dynTree;
    if (tree == 0) return VRIntersection();

    VRIntersection ins = intersections[tree];
    uint now = VRGlobals::get()->CURRENT_FRAME;
    if (ins.hit && ins.time == now) return ins; // allready found it
    ins.time = now;

    iAct->apply(tree->getNode());
    ins.hit = iAct->didHit();
    if (!ins.hit) { intersections[tree] = ins; lastIntersection = ins; return ins; }

    ins.object = tree->find(iAct->getHitObject()->getParent());
    ins.point = iAct->getHitPoint();
    ins.normal = iAct->getHitNormal();
    ins.triangle = iAct->getHitTriangle();
    ins.texel = VRIntersect_computeTexel(ins, iAct->getHitObject());
    intersections[tree] = ins;
    lastIntersection = ins;

    if (showHit) cross->setWorldPosition(Vec3f(ins.point));
    //cout << "VRIntersect::intersect " << obj << endl;

    return ins;
}

void VRIntersect::drag(VRTransform* caster, VRObject* tree, VRDevice* dev) {
    VRIntersection ins = intersect(tree);

    if (ins.object == 0 or dragged != 0 or !dnd) return;

    VRObject* obj2 = ins.object->findPickableAncestor();
    //cout << "\n pickable " << obj->isPickable() << " " << obj->getName() << " " << caster->getParent()->getName() << flush;
    if (obj2 == 0) return;
    if (!obj2->hasAttachment("transform")) return;

    dragged = (VRTransform*)obj2;
    dragged->drag(caster);

    return;
}

void VRIntersect::drop(VRDevice* dev) {
    if (dragged != 0) {
        dragged->drop();
        dragged = 0;
    }
}

void VRIntersect::initCross() {
    cross = new VRGeometry("Hit cross");
    cross->hide();
    showHit = false;

    vector<Vec3f> pos, norms;
    vector<Vec2f> texs;
    vector<int> inds;

    float d = 0.1;
    pos.push_back(Vec3f(-d,-d,-d));
    pos.push_back(Vec3f(d,d,d));
    pos.push_back(Vec3f(-d,d,-d));
    pos.push_back(Vec3f(d,-d,d));

    pos.push_back(Vec3f(-d,-d,d));
    pos.push_back(Vec3f(d,d,-d));
    pos.push_back(Vec3f(-d,d,d));
    pos.push_back(Vec3f(d,-d,-d));

    for (int i=0;i<8;i++) {
        norms.push_back(Vec3f(0,0,-1));
        inds.push_back(i);
        texs.push_back(Vec2f(0,0));
    }

    VRMaterial* mat = new VRMaterial("red_cross");
    mat->setDiffuse(Color3f(1,0,0));
    cross->create(GL_LINES, pos, norms, inds, texs);
    cross->setMaterial(mat);
}

VRIntersect::VRIntersect() {
    initCross();
    drop_fkt = new VRDevCb("Intersect_drop", boost::bind(&VRIntersect::drop, this, _1));
}

VRIntersect::~VRIntersect() { delete cross; }

VRDevCb* VRIntersect::addDrag(VRTransform* caster, VRObject* tree) {
    VRDevCb* fkt;
    if (int_fkt_map.count(tree)) return int_fkt_map[tree];

    fkt = new VRDevCb("Intersect_drag", boost::bind(&VRIntersect::drag, this, caster, tree, _1));
    dra_fkt_map[tree] = fkt;
    return fkt;
}

void VRIntersect::toggleDragnDrop(bool b) { dnd = b;}
void VRIntersect::showHitPoint(bool b) {//needs to be optimized for multiple scenes
    showHit = b;
    if (b) cross->show();
    else cross->hide();
}

void VRIntersect::addDynTree(VRObject* o) {
    dynTrees.push_back(o);
    dynTree = o;
}

void VRIntersect::remDynTree(VRObject* o) {
    vector<VRObject*>::iterator it = std::find(dynTrees.begin(), dynTrees.end(), o);
    if(it != dynTrees.end()) {
        dynTrees.erase(it);
        dynTree = 0;
    }
}

void VRIntersect::updateDynTree(VRObject* a) {
    dynTree = a;
    return; // TODO ?

    for (uint i=0;i<dynTrees.size();i++) {
        if (dynTrees[i]->hasAncestor(a)) {
            dynTree = dynTrees[i];
        }
    }
}

VRObject* VRIntersect::getCross() { return cross; }//needs to be optimized for multiple scenes
VRDevCb* VRIntersect::getDrop() { return drop_fkt; }
//Pnt3f VRIntersect::getHitPoint() { return hitPoint; }
//Vec2f VRIntersect::getHitTexel() { return hitTexel; }
//VRObject* VRIntersect::getHitObject() { return obj; }
VRTransform* VRIntersect::getDraggedObject() { return dragged; }

VRIntersection VRIntersect::getLastIntersection() { return lastIntersection; }

OSG_END_NAMESPACE;
