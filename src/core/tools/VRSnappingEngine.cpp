#include "VRSnappingEngine.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/object/VRObjectT.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRScene.h"
#include "core/math/partitioning/Octree.h"
#include "core/math/partitioning/OctreeT.h"
#include "core/math/pose.h"
#include "core/utils/toString.h"
#include "core/utils/VRDoublebuffer.h"
#include "core/setup/devices/VRSignalT.h"

#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

template<> string typeName(const VRSnappingEngine::PRESET* o) { return "VRSnappingEngine::PRESET"; }
template<> string typeName(const VRSnappingEngine::Type* o) { return "VRSnappingEngine::Type"; }
template<> string typeName(const VRSnappingEngine::EventSnap* o) { return "VRSnappingEngine::EventSnap"; }

template<> int toValue(stringstream& ss, VRSnappingEngine::PRESET& e) {
    string s = ss.str();
    if (s == "SIMPLE_ALIGNMENT") { e = VRSnappingEngine::SIMPLE_ALIGNMENT; return true; }
    if (s == "SNAP_BACK") { e = VRSnappingEngine::SNAP_BACK; return true; }
    return false;
}

template<> int toValue(stringstream& ss, VRSnappingEngine::Type& e) {
    string s = ss.str();
    if (s == "NONE") { e = VRSnappingEngine::NONE; return true; }
    if (s == "POINT") { e = VRSnappingEngine::POINT; return true; }
    if (s == "LINE") { e = VRSnappingEngine::LINE; return true; }
    if (s == "PLANE") { e = VRSnappingEngine::PLANE; return true; }
    if (s == "POINT_LOCAL") { e = VRSnappingEngine::POINT_LOCAL; return true; }
    if (s == "LINE_LOCAL") { e = VRSnappingEngine::LINE_LOCAL; return true; }
    if (s == "PLANE_LOCAL") { e = VRSnappingEngine::PLANE_LOCAL; return true; }
    return false;
}

struct VRSnappingEngine::Rule {
    unsigned long long ID = 0;
    Type translation = NONE;
    Type orientation = NONE;
    PosePtr prim_t;
    PosePtr prim_o;
    Vec3d snapP;

    VRTransformPtr csys = 0;
    float distance = 1;
    int group = 0;
    Matrix4d C;

    Rule(Type t, Type o, PosePtr pt, PosePtr po, float d, int g, VRTransformPtr l) :
        translation(t), orientation(o),
        prim_t(pt), prim_o(po), csys(l),
        distance(d), group(g) {
        static unsigned long long i = 0;
        ID = i++;
    }

    Vec3d local(Vec3d p) {
        if (csys) {
            C = csys->getWorldMatrix();
            C.invert();
            Pnt3d pL;
            C.mult(p,pL);
            return Vec3d(pL);
        } else return p;
    }

    Vec3d getSnapPoint(Vec3d p) {
        if (translation == POINT) snapP = prim_t->pos();

        if (translation == LINE) {
            Line l(Vec3f(prim_t->pos()), Vec3f(prim_t->dir()));
            snapP = Vec3d( l.getClosestPoint( Vec3f(p) ) ); // project on line
        }

        if (translation == PLANE) {
            Plane pl(Vec3f(prim_t->pos()), Vec3f(prim_t->dir()));
            float d = pl.distance(Pnt3f(p)); // project on plane
            snapP = p + Vec3d(d*pl.getNormal());
        }

        return snapP;
    }

    void snap(Matrix4d& m, PosePtr o = 0) {
        if (csys) C = csys->getWorldMatrix();

        PosePtr O = prim_o;
        if (o) O = prim_o->multRight(o);

        if (orientation == POINT) {
            MatrixLookAt(m, snapP, snapP+O->dir(), O->up());
            if (csys) {
                m.multLeft(C);
            }
        }
    }

    bool inRange(Vec3d pa, double& dmin, Vec3d pAnchor = Vec3d()) {
        Vec3d paL = local( pa ) - pAnchor;
        Vec3d psnap = getSnapPoint( paL );
        float D = (psnap-paL).length(); // check distance
        bool b = (D <= distance && D < dmin);
        if (b) dmin = D;
        return b;
    }

    bool checkGroup(VRObjectPtr o) {
        int og = o->getAttachment<int>("snapGroup");
        return (og == group);
    }
};

VRSnappingEngine::VRSnappingEngine() {
    positions = Octree<VRTransform*>::create(0.1);
    event = EventSnapPtr(new EventSnap());
    snapSignal = VRSignal::create();

    updatePtr = VRUpdateCb::create("snapping engine update", bind(&VRSnappingEngine::update, this) );
    VRScene::getCurrent()->addUpdateFkt(updatePtr, 0);
}

VRSnappingEngine::~VRSnappingEngine() {
    clear();
    if (event) event = 0;
}

shared_ptr<VRSnappingEngine> VRSnappingEngine::create() { return shared_ptr<VRSnappingEngine>(new VRSnappingEngine()); }

void VRSnappingEngine::addCallback(VRSnapCbPtr cb) { callbacks.push_back(cb); }

void VRSnappingEngine::clear() {
    anchors.clear();
    positions->clear();
    objects.clear();
    for (auto r : rules) delete r.second;
    rules.clear();
    if (event) event = 0;
    event = EventSnapPtr(new EventSnap());
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

void VRSnappingEngine::setActive(bool b) { active = b; }
bool VRSnappingEngine::isActive() { return active; }

void VRSnappingEngine::enableGhosts(bool b) { doGhosts = b; }

int VRSnappingEngine::addRule(Type t, Type o, PosePtr pt, PosePtr po, float d, int g, VRTransformPtr l) {
    Rule* r = new Rule(t,o,pt,po,d,g,l);
    rules[r->ID] = r;
    return r->ID;
}

void VRSnappingEngine::remRule(int i) {
    if (rules.count(i) == 0) return;
    delete rules[i];
    rules.erase(i);
}

void VRSnappingEngine::addObjectAnchor(VRTransformPtr obj, VRTransformPtr a, int grp, int snpgrp) {
    if (anchors.count(obj) == 0) anchors[obj] = vector<Anchor>();
    Anchor A;
    A.a = a;
    A.grp = grp;
    A.snpgrp = snpgrp;
    anchors[obj].push_back(A);
}

void VRSnappingEngine::pauseObjectAnchors(VRTransformPtr obj, bool paused) {
    if (anchors.count(obj))
        for (auto& a : anchors[obj]) a.active = !paused;
}

void VRSnappingEngine::pauseObjectAnchor(VRTransformPtr obj, int i, bool paused) {
    if (anchors.count(obj))
        if ( i >= 0 && i < anchors[obj].size())
            anchors[obj][i].active = !paused;
}

void VRSnappingEngine::clearObjectAnchors(VRTransformPtr obj) {
    if (anchors.count(obj)) anchors[obj].clear();
}

void VRSnappingEngine::remLocalRules(VRTransformPtr obj) {
    vector<int> d;
    for (auto r : rules) if (r.second->csys == obj) d.push_back(r.first);
    for (int i : d) remRule(i);
}

void VRSnappingEngine::addObject(VRTransformPtr obj, int group) {
    if (!obj) return;
    objects[obj] = obj->getWorldMatrix();
    Vec3d p = obj->getWorldPosition();
    positions->add(p, obj.get());
    obj->addAttachment<int>("snapGroup", group);
}

void VRSnappingEngine::remObject(VRTransformPtr obj) {
    if (objects.count(obj)) objects.erase(obj);
}

void VRSnappingEngine::addTree(VRObjectPtr obj, int group) {
    vector<VRObjectPtr> objs = obj->getChildren(true, "", true);
    for (auto o : objs) {
        if (auto geo = dynamic_pointer_cast<VRGeometry>(o)) addObject(geo, group);
    }
}

VRSignalPtr VRSnappingEngine::getSignalSnap() { return snapSignal; }
void VRSnappingEngine::showSnapping(bool b) { showSnaps = b; }

void VRSnappingEngine::terminateGhost() {
    Vec3d scale = ghostHost->getScale(); // conserve scale
    ghostHost->setWorldMatrix(event->m);
    ghostHost->setScale(scale);

    ghostHook->clearLinks();
    ghostParent->remLink(ghostMat);
    ghostDevice = 0;
    ghostParent = 0;
    ghostHost = 0;
}

void VRSnappingEngine::updateGhost(VRDevicePtr dev, VRTransformPtr obj) {
    if (!ghostHook) {
        ghostHook = VRTransform::create("ghostHook");
        ghostMat = VRMaterial::create("ghostMat");
        ghostMat->setDiffuse(Color3f(0,0.5,1.0));
        ghostMat->setZOffset(-1,-1);
        ghostMat->setTransparency(0.4);
        ghostMat->ignoreMeshColors(true);
        ghostMat->addChild(ghostHook);
    }

    if (ghostHost != obj) {
        if (ghostHost) {
            ghostHook->clearLinks();
            ghostParent->remLink(ghostMat);
        }

        ghostHost = obj;
        if (ghostHost) {
            //ghostHost->getDragParent()->addLink(ghostMat);
            ghostDevice = dev;
            ghostParent = ghostHost->getParent();
            ghostParent->addLink(ghostMat);
            ghostHook->addLink(ghostHost);
        }
    }

    if (!ghostHost) {
        ghostDevice = 0;
        ghostParent = 0;
        return;
    }

    Matrix4d Mpi, moi;
    Matrix4d Mp = ghostHost->getWorldMatrix(true);
    Mp.inverse(Mpi);

    Matrix4d mscale;
    mscale.setScale( ghostHost->getScale() );
    auto mo = ghostHost->getMatrix();
    mo.inverse(moi);
    moi.multLeft(mscale);

    //    Mg = Me = Mp * mg * mo
    // -> mg = Mp_1 * Me * mo_1
    auto mg = event->m;
    mg.multLeft(Mpi);
    mg.mult(moi);
    ghostHook->setMatrix(mg);
}

void VRSnappingEngine::handleDraggedObject(VRDevicePtr dev, VRTransformPtr obj, VRTransformPtr gobj) { // dragged object, dragged ghost object
    Matrix4d moW = obj->getWorldMatrix();
    Vec3d p = Vec3d(moW[3]);

    event->snap = 0;
    int snapID = -1;

    double dmin = 1e9;

    for (auto ri : rules) {
        Rule* r = ri.second;
        if (r->csys == obj) continue;
        if (!r->checkGroup(obj)) continue;

        if (anchors.count(obj)) {
            for (auto& A : anchors[obj]) { // check if anchor snapped
                if (!A.active) continue;
                Matrix4d maL = A.a->getMatrix();
                Matrix4d maW = moW; maW.mult(maL);
                Vec3d paW = Vec3d(maW[3]);
                Matrix4d maLi;
                maL.inverse(maLi);

                if (r->csys && anchors.count(r->csys)) { // if local system has anchors, check for snap to them
                    Matrix4d m2 = r->csys->getWorldMatrix();
                    Vec3d po2W = Vec3d(m2[3]);

                    for (auto& B : anchors[r->csys]) {
                        if (!B.active) continue;
                        if (A.snpgrp != B.grp) continue;
                        Matrix4d ma2L = B.a->getMatrix();
                        Matrix4d ma2W = m2; ma2W.mult(ma2L);
                        Vec3d pa2W = Vec3d(ma2W[3]);
                        Vec3d pa2 = Vec3d(ma2L[3]);
                        snapID++;
                        if (!r->inRange(paW, dmin, pa2)) continue;
                        //cout << " " << paW << ", -> " << pa2W << ", d: " << (paW - pa2W).length() << ", rd: " << r->distance << endl;

                        ma2L[3] = Vec4d(0,0,0,1);
                        Matrix4d ma2Li;
                        ma2L.inverse(ma2Li);

                        r->snapP += pa2;
                        Matrix4d mm = moW;
                        r->snap(mm, B.a->getPose());
                        mm.mult(maLi);
                        event->set(obj, r->csys, mm, dev, 1, snapID, A.a, B.a);
                        event->po1 = p;
                        event->pa1 = paW;
                        event->po2 = po2W;
                        event->pa2 = pa2W;
                    }
                } else { // just check if anchor snapps to rule
                    snapID++;
                    if (!r->inRange(paW, dmin)) continue;
                    Matrix4d mm = moW;
                    r->snap(mm);
                    mm.mult(maLi);
                    event->set(obj, r->csys, mm, dev, 1, snapID, A.a, 0);
                    event->po1 = p;
                    event->pa1 = paW;
                    if (r->csys) event->po2 = r->csys->getWorldPosition();
                }
            }
        } else { // simple snap, obj origin
            snapID++;
            if (!r->inRange(p, dmin)) continue;
            Matrix4d mm = moW;
            r->snap(mm);
            event->set(obj, r->csys, mm, dev, 1, snapID, 0, 0);
            event->po1 = p;
            if (r->csys) event->po2 = r->csys->getWorldPosition();
        }
    }
}

void VRSnappingEngine::postProcessEvent(VRDevicePtr dev, VRTransformPtr obj, VRTransformPtr gobj) {
    if (event->snap) {
        if (!doGhosts) {
            Vec3d scale = obj->getScale(); // conserve scale
            obj->setWorldMatrix(event->m);
            obj->setScale(scale);
        } else {
            updateGhost(dev, obj);
        }
    } else {
        if (doGhosts) updateGhost(0, 0);
        else obj->setMatrix( gobj->getMatrix() );
    }

    if (lastEvent != event->snap || lastEventID != event->snapID) {
        if (event->o1 == obj) {
            snapSignal->triggerAll<EventSnap>(event);
            for (auto cb : callbacks) (*cb)(event);
        }
    }
}

void VRSnappingEngine::updateSnapVisual() {
    if (!snapVisual) {
        VRGeoData data;
        data.pushVert(Vec3d(0,0,0));
        data.pushVert(Vec3d(0,0,0));
        data.pushColor(Color3f(0.2,0.6,1));
        data.pushColor(Color3f(0.2,0.6,1));
        data.pushLine(0,1);

        if (0) {
            data.pushVert(Vec3d(0,0,0));
            data.pushVert(Vec3d(0,0,0));
            data.pushColor(Color3f(1,0.6,0));
            data.pushColor(Color3f(1,0.6,0));
            data.pushLine(2,3);
        }

        auto m = VRMaterial::create("snapMat");
        m->setLit(false);
        m->setLineWidth(5);
        m->setDepthTest(GL_ALWAYS);
        snapVisual = data.asGeometry("snap");
        snapVisual->setMaterial(m);
        snapVisual->setPersistency(0);
        VRScene::getCurrent()->getRoot()->addChild(snapVisual);
    }

    snapVisual->setVisible(event->snap);

    if (event->snap) {
        auto pos = (GeoPnt3fProperty*)snapVisual->getMesh()->geo->getPositions();
        if (event->a1) pos->setValue(event->pa1,0);
        else           pos->setValue(event->po1,0);
        if (event->a2) pos->setValue(event->pa2,1);
        else           pos->setValue(event->po2,0);

        if (0) {
            pos->setValue(event->po1,2);
            pos->setValue(event->po2,3);
        }
    }
}

void VRSnappingEngine::update() {
    if (!active) return;

    auto setup = VRSetup::getCurrent();
    if (!setup) return;

    bool noneDragged = true;
    for (auto dev : setup->getDevices()) { // get dragged objects
        VRTransformPtr obj = dev.second->getDraggedObject();
        VRTransformPtr gobj = dev.second->getDraggedGhost();
        if (ghostDevice == dev.second && ghostHost && objects.count(obj) == 0) terminateGhost();
        if (obj != 0 && gobj != 0 && objects.count(obj) != 0) {
            noneDragged = false;
            lastEvent = event->snap;
            lastEventID = event->snapID;
            handleDraggedObject(dev.second, obj, gobj);
            postProcessEvent(dev.second, obj, gobj);
            if (showSnaps) updateSnapVisual();
        } //else event->snap = 0;
    }

    if (noneDragged && showSnaps && snapVisual) snapVisual->hide();
}

void VRSnappingEngine::setPreset(PRESET preset) {
    clear();

    PosePtr pX = Pose::create(Vec3d(), Vec3d(-1,0,0), Vec3d(0,1,0));
    PosePtr pY = Pose::create(Vec3d(), Vec3d(0,-1,0), Vec3d(0,0,1));
    PosePtr pZ = Pose::create(Vec3d(), Vec3d(0,0,-1), Vec3d(0,1,0));

    switch(preset) {
        case SIMPLE_ALIGNMENT:
            addRule(POINT, POINT, pZ, pZ, 1, 0, 0);
            addRule(LINE , POINT, pX, pZ, 1, 0, 0);
            addRule(LINE , POINT, pY, pZ, 1, 0, 0);
            addRule(LINE , POINT, pZ, pZ, 1, 0, 0);
            break;
        case SNAP_BACK:
            addRule(POINT, POINT, pZ, pZ, 1, 0, 0);
            break;
    }
}



