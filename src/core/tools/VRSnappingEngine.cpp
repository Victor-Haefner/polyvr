#include "VRSnappingEngine.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/math/Octree.h"

#include <OpenSG/OSGMatrixUtility.h>

OSG_BEGIN_NAMESPACE;

struct VRSnappingEngine::Rule {
    unsigned long long ID = 0;
    Type translation = NONE;
    Type orientation = NONE;
    Line prim_t, prim_o;

    bool local;
    float distance;
    float weight = 1;

    Rule(Type t, Type o, Line pt, Line po, float d, float w, bool l) :
        translation(t), orientation(o),
        prim_t(pt), prim_o(po), local(l),
        distance(d), weight(w) {
        static unsigned long long i = 0;
        ID = i++;
    }

    void apply(Matrix& m, VRTransform* t = 0) {
        Vec3f p = Vec3f(m[3]);

        Vec3f p2; // get point to snap to
        if (translation == POINT) p2 = prim_t.getDirection();
        if (translation == LINE) p2 = prim_t.getClosestPoint(p).subZero(); // project on line
        if (translation == PLANE) {
            Plane pl(prim_t.getDirection(), prim_t.getPosition());
            float d = pl.distance(p); // project on plane
            p2 = p + d*pl.getNormal();
        }

        if (t) { // TODO: go to local coords of t
            Matrix nm = t->getWorldMatrix();
            Vec3f np = Vec3f(nm[3]);

            Vec3f dir = p-np;
            float d = dir.length();
            //if (d < 1e-4) continue;
            dir /= d;
        }

        // check distance
        if ((p2-p).length() > distance) return;

        cout << "Snapp!\n";
        m.setTranslate(p2); // snap

        // apply orientation
        if (orientation == POINT) {
            Matrix r;
            MatrixLookAt(r, Pnt3f(), prim_o.getPosition(), prim_o.getDirection());
            m[0] = r[0];
            m[1] = r[1];
            m[2] = r[2];
        }
    }
};

VRSnappingEngine::VRSnappingEngine() {
    hintGeo = new VRGeometry("snapping_engine_hint");
    positions = new Octree(0.1);

    VRFunction<int>* fkt = new VRFunction<int>("snapping engine update", boost::bind(&VRSnappingEngine::update, this) );
    VRSceneManager::getCurrent()->addUpdateFkt(fkt, 100);
}

void VRSnappingEngine::clear() {
    //hintGeo->clear();
    //hintGeo->hide();
    positions->clear();
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

int VRSnappingEngine::addRule(Type t, Type o, Line pt, Line po, float d, float w, bool l) {
    Rule* r = new Rule(t,o,pt,po,d,w,l);
    rules[r->ID] = r;
    return r->ID;
}

void VRSnappingEngine::remRule(int i) {
    if (rules.count(i) == 0) return;
    delete rules[i];
    rules.erase(i);
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



void VRSnappingEngine::update() {
    for (auto dev : VRSetupManager::getCurrent()->getDevices()) { // get dragged objects
        VRTransform* obj = dev.second->getDraggedObject();
        VRTransform* gobj = dev.second->getDraggedGhost();
        if (obj == 0 || gobj == 0) continue;


        Matrix m = gobj->getWorldMatrix();
        Vec3f p = Vec3f(m[3]);

        vector<void*> neighbors = positions->radiusSearch(p[0], p[1], p[2], influence_radius);
        for (auto ri : rules) {
            Rule* r = ri.second;
            if (!r->local) r->apply(m);
            else for (auto n : neighbors) {
                VRTransform* t = (VRTransform*)n;
                if (t != obj) r->apply(m, t);
            }
        }

        obj->setWorldMatrix(m);
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
            addRule(POINT, POINT, t0, o0, 1, 1, true);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(1,0,0)), o0, 1, 1, true);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(0,1,0)), o0, 1, 1, true);
            addRule(LINE, POINT, Line(Pnt3f(), Vec3f(0,0,1)), o0, 1, 1, true);
            break;
        case SNAP_BACK:
            addRule(POINT, POINT, t0, o0, 1, 1, true);
            break;
    }
}

OSG_END_NAMESPACE;
