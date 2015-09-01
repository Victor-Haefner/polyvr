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
    auto texcoords = geo->getTexCoords();
    if (texcoords == 0) return Vec2f(0,0);
    TriangleIterator iter = geo->beginTriangles(); iter.seek( ins.triangle );


    Matrix m = node->getToWorld();
    m.invert();
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

class VRIntersectAction : public IntersectAction {
    private:
        /*Action::ResultE GeoInstancer::intersectEnter(Action  *action) {
            if(_sfBaseGeometry.getValue() == NULL) return Action::Continue;
            return _sfBaseGeometry.getValue()->intersectEnter(action);
            return Action::Continue;
        }*/

    public:
        VRIntersectAction() {
            // TODO: check OSG::Geometry::intersectEnter

            //IntersectAction::registerEnterDefault( getClassType(), reinterpret_cast<Action::Callback>(&GeoInstancer::intersectEnter));
        }
};

VRIntersection VRIntersect::intersect(VRObject* tree, Line ray) {
    VRIntersectAction iAct;
    //IntersectActionRefPtr iAct = IntersectAction::create();
    iAct.setTravMask(8);
    iAct.setLine(ray);
    iAct.apply(tree->getNode());

    VRIntersection ins;
    ins.hit = iAct.didHit();
    if (ins.hit) {
        ins.object = tree->find(iAct.getHitObject()->getParent());
        ins.point = iAct.getHitPoint();
        ins.normal = iAct.getHitNormal();
        if (tree->getParent()) tree->getParent()->getNode()->getToWorld().mult( ins.point, ins.point );
        if (tree->getParent()) tree->getParent()->getNode()->getToWorld().mult( ins.normal, ins.normal );
        ins.triangle = iAct.getHitTriangle();
        ins.texel = VRIntersect_computeTexel(ins, iAct.getHitObject());
        lastIntersection = ins;
    } else {
        ins.object = 0;
        if (lastIntersection.time < ins.time) lastIntersection = ins;
    }

    intersections[tree] = ins;

    if (showHit) cross->setWorldPosition(Vec3f(ins.point));
    return ins;
}

VRIntersection VRIntersect::intersect(VRObject* tree) {
    VRDevice* dev = (VRDevice*)this;
    VRTransform* caster = dev->getBeacon();
    VRIntersection ins;
    if (caster == 0) { cout << "Warning: VRIntersect::intersect, caster is 0!\n"; return ins; }
    if (tree == 0) tree = dynTree;
    if (tree == 0) return ins;

    if (intersections.count(tree)) ins = intersections[tree];
    uint now = VRGlobals::get()->CURRENT_FRAME;
    if (ins.hit && ins.time == now) return ins; // allready found it
    ins.time = now;

    Line ray = caster->castRay(tree);
    return intersect(tree, ray);
}

void VRIntersect::dragCB(VRTransform* caster, VRObject* tree, VRDevice* dev) {
    VRIntersection ins = intersect(tree);
    drag(ins.object, caster);
}

void VRIntersect::drag(VRObject* obj, VRTransform* caster) {
    auto d = getDraggedObject();
    if (obj == 0 || d != 0 || !dnd) return;

    obj = obj->findPickableAncestor();
    if (obj == 0) return;
    if (!obj->hasAttachment("transform")) return;

    dragged = (VRTransform*)obj;
    dragged->drag(caster);
    dragged_ghost->setMatrix(dragged->getMatrix());
    dragged_ghost->switchParent(caster);

    dragSignal->trigger<VRDevice>();
}

void VRIntersect::drop(VRDevice* dev) {
    auto d = getDraggedObject();
    if (d != 0) {
        dropSignal->trigger<VRDevice>();
        d->drop();
        drop_time = VRGlobals::get()->CURRENT_FRAME;
        dragged = 0;
    }
}

VRTransform* VRIntersect::getDraggedObject() { return dragged; }

VRTransform* VRIntersect::getDraggedGhost() { return dragged_ghost; }
VRSignal* VRIntersect::getDragSignal() { return dragSignal; }
VRSignal* VRIntersect::getDropSignal() { return dropSignal; }

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
    dragged_ghost = new VRTransform("dev_ghost");
    dragSignal = new VRSignal((VRDevice*)this);
    dropSignal = new VRSignal((VRDevice*)this);
}

VRIntersect::~VRIntersect() {
    delete cross;
    delete dragged_ghost;
    delete dragSignal;
    delete dropSignal;
}

VRDevCb* VRIntersect::addDrag(VRTransform* caster, VRObject* tree) {
    VRDevCb* fkt;
    if (int_fkt_map.count(tree)) return int_fkt_map[tree];

    fkt = new VRDevCb("Intersect_drag", boost::bind(&VRIntersect::dragCB, this, caster, tree, _1));
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

VRIntersection VRIntersect::getLastIntersection() { return lastIntersection; }

OSG_END_NAMESPACE;
