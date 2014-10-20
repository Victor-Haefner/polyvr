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

void VRIntersect::intersect(VRTransform* caster, VRObject* tree, VRDevice* dev) {
    Line ray = caster->castRay();
    IntersectActionRefPtr iAct = IntersectAction::create();
    iAct->setLine(ray);
    //iAct->apply(scene->getRoot()->getNode());

    if (tree == 0) tree = dynTree;
    if (tree == 0) return;
    iAct->apply(tree->getNode());

    if (!iAct->didHit()) {obj = 0; return; }
    if (showHit) { // TODO: does nothing!
        cross->setFrom(Vec3f(iAct->getHitPoint()));
        cross->setAt(Vec3f(ray.getPosition()));
    }

    hitPoint = iAct->getHitPoint();
    Vec3f normal = iAct->getHitNormal();

    GeometryRefPtr geo = dynamic_cast<Geometry*>( iAct->getHitObject()->getCore() );
    TriangleIterator iter = geo->beginTriangles(); iter.seek( iAct->getHitTriangle() );

    Matrix m; iAct->getHitObject()->getToWorld().inverse(m);
    Pnt3f local_pnt; m.mult(hitPoint, local_pnt);

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

    hitTexel = iter.getTexCoords(0) * a + iter.getTexCoords(1) * b + iter.getTexCoords(2) * c;


    obj = tree->find(iAct->getHitObject()->getParent());
    if(obj == 0) return;
}

void VRIntersect::drag(VRTransform* caster, VRObject* tree, VRDevice* dev) {
    intersect(caster, tree, dev);
    if (obj == 0 or dragged != 0 or !dnd) return;

    VRObject* obj2 = obj->findPickableAncestor();
    //cout << "\n pickable " << obj->isPickable() << " " << obj->getName() << " " << caster->getParent()->getName() << flush;
    if (obj2 == 0) return;
    if (!obj2->hasAttachment("transform")) return;

    dragged = (VRTransform*)obj2;
    dragged->drag(caster);
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
    cross->create(GL_LINE, pos, norms, inds, texs);
    cross->setMaterial(mat);
}

VRIntersect::VRIntersect() {
    initCross();
    drop_fkt = new VRDevCb("Intersect_drop", boost::bind(&VRIntersect::drop, this, _1));
}

VRIntersect::~VRIntersect() {
    delete cross;
}

VRDevCb* VRIntersect::addIntersect(VRTransform* caster, VRObject* tree) {
    VRDevCb* fkt;
    if (int_fkt_map.count(tree)) return int_fkt_map[tree];

    fkt = new VRDevCb("Intersect_intersect", boost::bind(&VRIntersect::intersect, this, caster, tree, _1));
    int_fkt_map[tree] = fkt;
    return fkt;
}

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
Pnt3f VRIntersect::getHitPoint() { return hitPoint; }
Vec2f VRIntersect::getHitTexel() { return hitTexel; }
VRObject* VRIntersect::getHitObject() { return obj; }
VRTransform* VRIntersect::getDraggedObject() { return dragged; }

OSG_END_NAMESPACE;
