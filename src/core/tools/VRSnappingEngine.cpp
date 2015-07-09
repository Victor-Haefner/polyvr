#include "VRSnappingEngine.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/math/Octree.h"
#include "core/utils/VRDoublebuffer.h"

#include <OpenSG/OSGMatrixUtility.h>

OSG_BEGIN_NAMESPACE;

struct VRSnappingEngine::Rule {
    unsigned long long ID = 0;
    Type translation = NONE;
    Type orientation = NONE;
    Line prim_t, prim_o;
    Vec3f snapP;

    VRTransform* csys = 0;
    float distance = 1;
    float weight = 1;
    Matrix C;

    Rule(Type t, Type o, Line pt, Line po, float d, float w, VRTransform* l) :
        translation(t), orientation(o),
        prim_t(pt), prim_o(po), csys(l),
        distance(d), weight(w) {
        static unsigned long long i = 0;
        ID = i++;
    }

    Vec3f local(Vec3f p) {
        if (csys) {
            C = csys->getWorldMatrix();
            C.invert();
            Pnt3f pL;
            C.mult(p,pL);
            return Vec3f(pL);
        } else return p;
    }

    Vec3f getSnapPoint(Vec3f p) {
        if (translation == POINT) snapP = Vec3f(prim_t.getPosition());
        if (translation == LINE) snapP = prim_t.getClosestPoint(local(p)).subZero(); // project on line
        if (translation == PLANE) {
            Plane pl(prim_t.getDirection(), prim_t.getPosition());
            p = local(p);
            float d = pl.distance(p); // project on plane
            snapP = p + d*pl.getNormal();
        }
        return snapP;
    }

    void snap(Matrix& m) {
        if (csys) C = csys->getWorldMatrix();

        if (orientation == POINT) {
            MatrixLookAt(m, snapP, snapP+Vec3f(prim_o.getPosition()), prim_o.getDirection());
            m.multLeft(C);
        }
    }

    bool inRange(float d) { return (d <= distance); }
};

VRSnappingEngine::VRSnappingEngine() {
    hintGeo = new VRGeometry("snapping_engine_hint");
    positions = new Octree(0.1);
    event = new EventSnap();
    snapSignal = new VRSignal();

    VRFunction<int>* fkt = new VRFunction<int>("snapping engine update", boost::bind(&VRSnappingEngine::update, this) );
    VRSceneManager::getCurrent()->addUpdateFkt(fkt, 999);
}

void VRSnappingEngine::clear() {
    anchors.clear();
    positions->clear();
    objects.clear();
    for (auto r : rules) delete r.second;
    rules.clear();
    delete event;
}


VRSnappingEngine::Type VRSnappingEngine::typeFromStr(string t) {
    if (t == "NONE") return NONE;
    if (t == "POINT") return POINT;
    if (t == "LINE") return LINE;
    if (t == "PLANE") return PLANE;
    if (t == "POINT_LOCAL") return POINT_LOCAL;
    if (t == "LINE_LOCAL") return LINE_LOCAL;
    if (t == "PLANE_LOCAL") return PLANE_LOCAL;
    cout << "Warning: VRSnappingEngine::" << t << " is not a Type.\n";
    return NONE;
}

int VRSnappingEngine::addRule(Type t, Type o, Line pt, Line po, float d, float w, VRTransform* l) {
    Rule* r = new Rule(t,o,pt,po,d,w,l);
    rules[r->ID] = r;
    return r->ID;
}

void VRSnappingEngine::remRule(int i) {
    if (rules.count(i) == 0) return;
    delete rules[i];
    rules.erase(i);
}

void VRSnappingEngine::addObjectAnchor(VRTransform* obj, VRTransform* a) {
    if (anchors.count(obj) == 0) anchors[obj] = vector<VRTransform*>();
    anchors[obj].push_back(a);
}

void VRSnappingEngine::clearObjectAnchors(VRTransform* obj) {
    if (anchors.count(obj)) anchors[obj].clear();
}

void VRSnappingEngine::remLocalRules(VRTransform* obj) {
    vector<int> d;
    for (auto r : rules) if (r.second->csys == obj) d.push_back(r.first);
    for (int i : d) remRule(i);
}

void VRSnappingEngine::addObject(VRTransform* obj, float weight) {
    objects[obj] = obj->getWorldMatrix();
    Vec3f p = obj->getWorldPosition();
    positions->add(p[0], p[1], p[2], obj);
}

void VRSnappingEngine::remObject(VRTransform* obj) {
    if (objects.count(obj)) objects.erase(obj);
}

void VRSnappingEngine::addTree(VRObject* obj, float weight) {
    vector<VRObject*> objs = obj->getObjectListByType("Geometry");
    for (auto o : objs) addObject((VRTransform*)o, weight);
}

VRSignal* VRSnappingEngine::getSignalSnap() { return snapSignal; }

void VRSnappingEngine::update() {
    for (auto dev : VRSetupManager::getCurrent()->getDevices()) { // get dragged objects
        VRTransform* obj = dev.second->getDraggedObject();
        VRTransform* gobj = dev.second->getDraggedGhost();
        if (obj == 0 || gobj == 0) continue;
        if (objects.count(obj) == 0) continue;

        Matrix m = gobj->getWorldMatrix();
        Vec3f p = Vec3f(m[3]);

        bool lastEvent = event->snap;
        event->snap = 0;

        for (auto ri : rules) {
            Rule* r = ri.second;
            if (r->csys == obj) continue;

            if (anchors.count(obj)) {
                for (auto a : anchors[obj]) {
                    Matrix maL = a->getMatrix();
                    Matrix maW = m; maW.mult(maL);
                    Vec3f pa = Vec3f(maW[3]);
                    Vec3f paL = r->local( Vec3f(maW[3]) );
                    Vec3f psnap = r->getSnapPoint(pa);
                    float D = (psnap-paL).length(); // check distance
                    //cout << "dist " << D << " " << pa[1] << " " << paL[1] << " " << psnap[1] << endl;
                    if (!r->inRange(D)) continue;

                    r->snap(m);
                    maL.invert();
                    m.mult(maL);
                    event->set(obj, r->csys, m, dev.second, 1);
                    break;
                }
            } else {
                Vec3f p2 = r->getSnapPoint(p);
                float D = (p2-p).length(); // check distance
                if (!r->inRange(D)) continue;
                r->snap(m);
                event->set(obj, r->csys, m, dev.second, 1);
            }
        }

        obj->setWorldMatrix(m);
        if (lastEvent != event->snap) {
            if (event->snap) snapSignal->trigger<EventSnap>(event);
            else if (obj == event->o1) snapSignal->trigger<EventSnap>(event);
        }
    }

    // update geo
    if (!hintGeo->isVisible()) return;
}

void VRSnappingEngine::setVisualHints(bool b) {
    showHints = b;
    hintGeo->setVisible(b);
}

void VRSnappingEngine::setPreset(PRESET preset) {
    clear();

    Line t0(Pnt3f(0,0,0), Vec3f(0,0,0));
    Line o0(Pnt3f(0,0,-1), Vec3f(0,1,0));

    switch(preset) {
        case SIMPLE_ALIGNMENT:
            addRule(POINT, POINT, t0, o0, 1, 1, 0);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(1,0,0)), o0, 1, 1, 0);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(0,1,0)), o0, 1, 1, 0);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(0,0,1)), o0, 1, 1, 0);
            break;
        case SNAP_BACK:
            addRule(POINT, POINT, t0, o0, 1, 1, 0);
            break;
    }
}

OSG_END_NAMESPACE;
