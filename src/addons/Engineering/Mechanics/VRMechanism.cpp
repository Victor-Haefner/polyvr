#include "VRMechanism.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"
#include "core/utils/isNan.h"
#include "core/tools/VRAnalyticGeometry.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#endif

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGQuaternion.h>

using namespace OSG;


/**

Simulation workflows:

1)
    component gets changed in SG
    register change of component
    update neighbors of component
    propagate change through system
    apply change to component

*/

MPart::MPart() {}
MPart::~MPart() {}
MGear::MGear() { type = "gear"; }
MGear::~MGear() {}
MChain::MChain() { type = "chain"; }
MChain::~MChain() {}
MThread::MThread() { type = "thread"; }
MThread::~MThread() {}

bool MPart::changed() {
    if (geo == 0) return false;
    //cout << "  part " << geo->getName() << " changed from timestamp " << timestamp << " (last change: " << geo->getLastChange() << ")";
    bool b = geo->changedSince(timestamp, true);
    //cout << " to " << timestamp << ", -> " << b << endl;
    return b;
}

void MPart::setBack() { if (geo) geo->setWorldMatrix(reference); }
void MPart::apply() {
    if (geo && type != "chain") reference = geo->getWorldMatrix();
    timestamp++;
}

MPart* MPart::make(VRTransformPtr g, VRTransformPtr t) {
    VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(g);
    if (!geo) return 0;
    string type = geo->getPrimitive()->getType();
    MPart* p = 0;
    if (type == "Gear") p = new MGear();
    if (type == "Thread") p = new MThread();
    if (type == "Chain") p = new MChain();
    if (p) {
        p->geo = g;
        p->prim = geo->getPrimitive();
        p->trans = t == 0 ? g : t;
    }
    return p;
}

void MPart::clearNeighbors() {
    for (auto n : neighbors) {
        //n->neighbors.erase( remove( begin(n->neighbors), end(n->neighbors), this ), end(n->neighbors) );
        n.first->neighbors.erase(this);
    }
    neighbors.clear();
}

void MPart::addNeighbor(MPart* p, MRelation* r) {
    //cout << "      MPart::addNeighbor " << p->geo->getName() << endl;
    neighbors[p] = r;
    p->neighbors[this] = r;
}

bool MPart::hasNeighbor(MPart* p) {
    for (auto n : neighbors) if (n.first == p) return true;
    return false;
}

void MPart::computeState() {
    int N = neighbors.size();
    if (state == FREE && N > 0) state = ENGAGING;
    if (state == ENGAGING && N > 0) state = ENGAGED;
    if (state == ENGAGED && N == 0) state = DISENGAGING;
    if (state == DISENGAGING && N == 0) state = FREE;
}

void MChange::flip() {
    a *= -1;
    dx *= -1;
}

bool MChange::same(MChange c) { // returning true, blocks the mechanism
    if (origin != c.origin) return false;
    if (a*c.a < 0) return false;
    return true;
}

bool MPart::propagateMovement() { // recursion
    bool res = true;
    for (auto n : neighbors) {
        res = n.first->propagateMovement(change, n.second) ? res : false;
    }
    return res;
}

bool MPart::propagateMovement(MChange c, MRelation* r) { // change
    r->translateChange(c);
    if (change.time == c.time) { // either it is the same change OR another change in the same timestep
        if (change.origin == c.origin) return change.same(c); // the same change
        else {} // another change in the same timestep
    }

    change = c;
    change.n = Vec3d();
    move();

    return propagateMovement();
}

void MPart::printChange() {
    cout.precision(2);
    cout.setf( ios::fixed, ios::floatfield );
    cout << geo->getName();
    if (change.isNull()) cout << " no change";
    else {
        cout << " trans: " << change.l;
        cout << " (vec: " << change.t << ") ";
        cout << "and rot " << change.a << " around " << change.n;
    }
    cout << endl;
}

void MPart::printNeighbors() {
    cout << geo->getName();
    cout << " neighbors: ";
    for (auto n : neighbors) cout << " " << n.first->geo->getName();
    cout << endl;
}

MGearGearRelation* checkGearGear(MGear* p1, MGear* p2) {
    Matrix4d r1 = p1->reference;
    Matrix4d r2 = p2->reference;
    Vec3d P1 = Vec3d(r1[3]) + p1->offset;
    Vec3d P2 = Vec3d(r2[3]) + p2->offset;
    Vec3d a1 = p1->rAxis;
    Vec3d a2 = p2->rAxis;
    Vec3d n1 = a1; n1.normalize();
    Vec3d n2 = a2; n2.normalize();

    VRGear* g1 = (VRGear*)p1->prim;
    VRGear* g2 = (VRGear*)p2->prim;
    Vec3d d = P2 - P1;
    float t = g1->teeth_size;

    Vec3d R1 = d - n1*n1.dot( d); R1.normalize();
    Vec3d R2 =-d - n2*n2.dot(-d); R2.normalize();
    R1 *= g1->radius();
    R2 *= g2->radius();
    float l = (P1+R1 - (P2+R2)).length();
    //cout << " l " << l << " t " << t << "  " << p1 << "  " << p2 << endl;
    if ( l > t) return 0; // not touching!

    Vec3d w1 = R1.cross(a1);
    Vec3d w2 = R2.cross(a2);

    MGearGearRelation* rel = new MGearGearRelation();
    rel->part1 = p1;
    rel->part2 = p2;
    rel->doFlip = bool(w1.dot(w2) < 0);
    return rel;
}

MObjRelation* checkPartObj(MPart* p1, MPart* p2) { // TODO
    if (p1->trans != p2->trans) return 0;
    MObjRelation* rel = new MObjRelation();
    rel->part1 = p1;
    rel->part2 = p2;
    return rel;
}

MRelation* checkGearThread(MPart* p1, MPart* p2) {
    //float R = g->radius() + t->radius;
    ; // TODO: check if line center distance is the gear + thread radius
    ; // TODO: check if thread && gear coplanar
    return 0;
}

MChainGearRelation* checkChainPart(MChain* c, MPart* p) {
    //cout << "checkChainPart " << endl;
    Matrix4d M = p->reference;
    Vec3d wpos = Vec3d(M[3]);
    float s = p->geo->getWorldScale()[0];
    if (!p->prim) return 0;
    if (p->prim->getType() != "Gear") return 0;
    float r = ((VRGear*)p->prim)->radius()*s;
    Vec3d dir = ((MGear*)p)->axis;
    M.mult(dir,dir);

    float eps = 1e-2; // 1e-3

    vector<pointPolySegment> psegs;
    for (auto ps : c->toPolygon(wpos) ) {
        double d = abs(ps.dist2 - r*r);
        if ( d < eps ) psegs.push_back(ps);
    }

    if (psegs.size() == 0) { /*cout << " fail1" << endl;*/ return 0; }

    dir.normalize();
    float fd = 0;
    int IDmin = 1e6;

    for (auto ps : psegs) {
        //cout << " checkChainPart " << ps.ID << endl;
        ps.Pseg.normalize();
        ps.seg.normalize();
        float n = ps.Pseg.cross(dir).dot(ps.seg);
        //cout << " checkChainPart n: " << n << ", d: " << dir << ", Pseg: " << ps.Pseg << ", seg: " << ps.seg << endl;
        if (abs(n) < (1-eps)) { cout << " fail2" << endl; continue; }
        if (IDmin > ps.ID) {
            //cout << " checkChainPart " << IDmin << "  " << ps.ID << endl;
            IDmin = ps.ID;
            fd = n;
        }
    }

    MChainGearRelation* rel = new MChainGearRelation();
    rel->part1 = c;
    rel->part2 = p;
    rel->dir = fd > 0 ? -1 : 1;
    rel->segID = IDmin;
    //cout << "checkChainPart " << IDmin << endl;
    return rel;
}

vector<pointPolySegment> MChain::toPolygon(Vec3d p) {
    vector<pointPolySegment> res;
    for (unsigned int i=0; i<polygon.size(); i+=2) {
        Vec3d p1 = polygon[i];
        Vec3d p2 = polygon[i+1];
        Vec3d d = p2-p1;
        Vec3d d1 = p1-p;
        Vec3d d2 = p2-p;

        Vec3d dx;
        float l2 = d.squareLength();
        int ID = i;
        if (l2 == 0.0) dx = d1;
        else {
            float t = d1.dot(d)/l2;
            if (t < 0.0) dx = d1;
            else if (t > 1.0) dx = d2;
            else dx = p1 + d*t - p; // vector from p to p1p2
            if (t > 0.5) ID = i+1;
        }

        pointPolySegment r;
        r.dist2 = dx.squareLength();
        r.Pseg = dx;
        r.seg = d;
        r.ID = ID;
        res.push_back(r);
    }

    return res;
}


/*bool checkThreadNut(VRScrewthread* t, VRNut* n, Matrix4d r1, Matrix4d r2) {
    ; // TODO: check if nut center on thread line
    ; // TODO: check if nut && thread same orientation
    return true;
}*/

VRGear* MGear::gear() { return (VRGear*)prim; }
VRScrewthread* MThread::thread() { return (VRScrewthread*)prim; }

void MPart::move() {}
void MChain::move() { if (!geo || !geo->isVisible("", true)) return; updateGeo(); }
void MThread::move() { trans->rotateWorld(change.a, Vec3d(0,0,-1)); }

void MGear::move() {
    if (!change.doMove) { // next gear on same object!
        change.dx = change.a*gear()->radius();
        return;
    }

    float a = change.dx/gear()->radius();
    change.a = a;
#ifndef WITHOUT_BULLET
    if (trans->getPhysics()->isPhysicalized()) {
        trans->getPhysics()->setDynamic(false, true);
        resetPhysics = true;
    }
#endif

    //cout << "MGear::move " << a << " / " << rAxis << " " << trans->getName() << endl;
    //trans->rotate(a, rAxis);
    trans->rotateWorld(a, rAxis);

#ifndef WITHOUT_BULLET
    if (trans->getPhysics()->isPhysicalized()) {
        trans->setBltOverrideFlag();
        trans->updatePhysics();
    }
#endif
}

void MPart::computeChange() {
    Matrix4d m1 = reference; // last world matrix
    Matrix4d m = geo->getWorldMatrix();
    Matrix4d r1 = m1; r1.setTranslate(Pnt3d()); // last world rotation matrix
    r1.invert();
    m.mult(r1);

    //change.t = Vec3d(m[3]);
    //change.l = change.t.length();

    change.a = acos( (m[0][0] + m[1][1] + m[2][2] - 1)*0.5 );
    if (isNan(change.a)) change.a = 0;

    change.n = -Vec3d(m[2][1] - m[1][2], m[0][2] - m[2][0], m[1][0] - m[0][1]);
    change.n.normalize();

    //if (change.n[2] < 0) { change.n *= -1; change.a *= -1; }
    if (abs(change.a) < 1e-5) return;
    //cout << "MPart::computeChange n " << change.n << "  a " << change.a << endl;
    change.time = timestamp;
    change.origin = this;
}

void MGear::computeChange() {
    MPart::computeChange();
    change.a *= rAxis.dot(change.n);
    //if (abs(change.a) > 1e-2) cout << "MGear::computeChange " << rAxis << "  n " << change.n << "  " << change.a << endl;
    change.dx = change.a*gear()->radius();
}

void MGear::updateNeighbors(vector<MPart*> parts) {
    //cout << "   MGear::updateNeighbors of " << geo->getName() << endl;
    clearNeighbors();
    for (auto part : parts) {
        if (part == this) continue;
        //cout << "    MGear::updateNeighbors check part " << part->geo->getName() << endl;
        VRPrimitive* p = part->prim;

        if (p == 0) { // chain
            MRelation* rel = checkChainPart((MChain*)part, this);
            if (rel) addNeighbor(part, rel);
        } else {
            MRelation* rel = checkPartObj(this, part);
            if (rel) addNeighbor(part, rel);
            else {
                if (p->getType() == "Gear") {
                    MRelation* rel = checkGearGear(this, (MGear*)part);
                    if (rel) addNeighbor(part, rel);
                }
                if (p->getType() == "Thread") {
                    MRelation* rel = checkGearThread(this, part);
                    if (rel) addNeighbor(part, rel);
                }
            }
        }
    }
}

void MChain::updateNeighbors(vector<MPart*> parts) {
    clearNeighbors();
    dirs = "";
    for (auto p : parts) {
        if (p == this) continue;
        if (!p->prim) continue;
        if (p->prim->getType() == "Gear") {
            MRelation* rel = checkChainPart(this, p);
            if (rel) addNeighbor(p, rel);
        }
    }
}

void MThread::updateNeighbors(vector<MPart*> parts) {
    clearNeighbors();
    for (auto part : parts) {
        if (part == this) continue;
        VRPrimitive* p = part->prim;
        if (p->getType() == "Gear") {
            MRelation* rel = checkGearThread((MGear*)part, this);
            if (rel) addNeighbor(part, rel);
        }
    }
}

MObjRelation::MObjRelation() { type = "obj"; }
MChainGearRelation::MChainGearRelation() { type = "chain"; }
MGearGearRelation::MGearGearRelation() { type = "gear"; }

void MRelation::translateChange(MChange& change) { change.doMove = true; }
void MObjRelation::translateChange(MChange& change) { change.doMove = false; }
void MChainGearRelation::translateChange(MChange& change) { change.doMove = true; if (dir == -1) change.flip(); }

void MGearGearRelation::translateChange(MChange& change) {
    change.doMove = true;
    //cout << "MGearGearRelation::translateChange doFlip " << doFlip << endl;
    if (doFlip) change.flip();
}

void MChain::setDirs(string dirs) { this->dirs = dirs; }
void MChain::addDir(char dir) { dirs.push_back(dir); }

void MChain::updateGeo() {
    //cout << "update chain\n";
    // update chain geometry

    // collect all polygon points
    //printNeighbors();
    polygon.clear();
    vector<MPart*> nbrs;
    map<int, MPart*> nbrs_m;
    for (auto n : neighbors) nbrs_m[((MChainGearRelation*)n.second)->segID] = n.first;
    for (auto n : nbrs_m) nbrs.push_back(n.second);

    int j=0;
    //cout << "MChain::updateGeo " << nbrs.size() << "   " << neighbors.size() << endl;
    for (unsigned int i=0; i<nbrs.size(); i++) {
        j = (i+1) % nbrs.size(); // next
        VRPrimitive* p1 = nbrs[i]->prim;
        VRPrimitive* p2 = nbrs[j]->prim;
        if (!p1 || !p2) continue;
        if (p1->getType() != "Gear") continue;
        if (p2->getType() != "Gear") continue;
        VRGear* g1 = (VRGear*)p1;
        VRGear* g2 = (VRGear*)p2;

        int d1 = dirs[i] == 'r' ? -1 : 1;
        int d2 = dirs[j] == 'r' ? -1 : 1;
        int z1 = -1*d1*d2;

        Vec3d c1 = nbrs[i]->geo->getWorldPosition();
        Vec3d c2 = nbrs[j]->geo->getWorldPosition();
        Vec3d D = c2-c1;
        float d = D.length();

        float s1 = 1;//nbrs[i]->geo->getWorldScale()[0];
        float s2 = 1;//nbrs[j]->geo->getWorldScale()[0];

        float r1 = g1->radius()*s1;
        float r2 = g2->radius()*s2;
        int z2 = r2 > r1 ? z1 : 1;
        r2 *= z1;

        //cout << " A " << nbrs[i]->geo->getName() << " " << D << " " << r1 << " " << r2 << endl;

        float x1 = 0;
        float x2 = 0;
        float y1 = r1;
        float y2 = z1*r2;

        if (abs(r1+r2) > 0.001 ) {
            Vec3d Ch = c1*r2/(r1+r2) + c2*r1/(r1+r2); // homothetic center
            float dCh1 = (Ch-c1).length();
            float dCh2 = (Ch-c2).length();
            float rCh1 = sqrt( dCh1*dCh1 - r1*r1 );
            float rCh2 = sqrt( dCh2*dCh2 - r2*r2 );
            x1 =    z2*r1*r1  /dCh1;
            x2 = z1*z2*r2*r2  /dCh2;
            y1 =    d1*r1*rCh1/dCh1;
            y2 = z1*d2*r2*rCh2/dCh2;
        }

        Matrix4d M1 = nbrs[i]->reference;
        Matrix4d M2 = nbrs[j]->reference;
        Vec3d a1 = ((MGear*)nbrs[i])->axis; // nbrs[i]->geo->getWorldDirection()
        Vec3d a2 = ((MGear*)nbrs[j])->axis; // nbrs[j]->geo->getWorldDirection()
        M1.mult(a1,a1);
        M2.mult(a2,a2);

        Vec3d dn = D/d;
        Vec3d t1 = dn.cross(a1);
        Vec3d t2 = dn.cross(a2);
        t1 = c1 + t1*y1 + dn*x1;
        t2 = c2 + t2*y2 - dn*x2;
        polygon.push_back(t1);
        polygon.push_back(t2);

        /*Vec3d check = t2-t1;
        cout << nbrs[i]->geo->getName() << " " << nbrs[j]->geo->getName();
        cout << " check " << check.dot(t1-c1) << " " << check.dot(t2-c2);
        cout << " r1 " << r1 << " r2 " << r2;
        cout << endl;*/
    }

    if (polygon.size() < 2) return;

    Vec3d c;
    for (auto v : polygon) c += v;
    c *= (1.0/polygon.size());
    reference[3] = Vec4d(c[0], c[1], c[2], 1 );

    VRGeoData data; // draw polygon
    for (unsigned int i=0; i<polygon.size()-1; i+=2) {
        data.pushVert(polygon[i], Vec3d(0,1,0), Color4f(0,1,0,1));
        data.pushVert(polygon[i+1], Vec3d(0,1,0), Color4f(1,1,0,1));
        data.pushLine();
    }
    data.apply( dynamic_pointer_cast<VRGeometry>(geo) );
}

VRTransformPtr MChain::init() {
    geo = VRGeometry::create("chain");
    updateGeo();
    VRMaterialPtr cm = VRMaterial::get("chain_mat");
    cm->setLit(false);
    cm->setLineWidth(3);
    VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(geo);
    g->setMaterial(cm);
    return geo;
}

// -------------- mechanism ------------------------

VRMechanism::VRMechanism() : VRObject("mechanism") { setPersistency(0); }
VRMechanism::~VRMechanism() { clear(); }

shared_ptr<VRMechanism> VRMechanism::create() { return shared_ptr<VRMechanism>(new VRMechanism()); }

void VRMechanism::clear() {
    for (auto part : parts) delete part;
    parts.clear();
    cache.clear();
}

void VRMechanism::add(VRTransformPtr part, VRTransformPtr trans) {
    MPart* p = MPart::make(part, trans);
    if (p == 0) return;

    cache[part].push_back(p);
    parts.push_back(p);
    p->apply();
    p->updateNeighbors(parts);
    p->setup();
}

void VRMechanism::addGear(VRTransformPtr part, float width, float hole, float pitch, int N_teeth, float teeth_size, float bevel, Vec3d axis, Vec3d offset) {
    auto p = new MGear();
    p->axis = axis;
    p->offset = offset;
    p->prim = new VRGear(width, hole, pitch, N_teeth, teeth_size, bevel);
    p->geo = part;
    p->trans = part;
    cache[part].push_back(p);
    parts.push_back(p);
    p->apply();
    p->updateNeighbors(parts);
    p->setup();
}

void MPart::setup() {}

void MGear::setup() {
    auto m = trans->getWorldMatrix();
    //auto m = trans->getMatrix();
    m.mult(axis, rAxis);
    rAxis.normalize();
}

VRTransformPtr VRMechanism::addChain(float w, vector<VRTransformPtr> geos, string dirs) {
    MChain* c = new MChain();
    for (unsigned int i=0; i<geos.size(); i++) {
        int j = (i+1)%geos.size();
        int k = (i+2)%geos.size();

        VRTransformPtr g1 = geos[i];
        VRTransformPtr g2 = geos[j];
        VRTransformPtr g3 = geos[k];

        if (cache.count(g1) == 0) continue;
        if (cache.count(g2) == 0) continue;
        if (cache.count(g3) == 0) continue;

        MChainGearRelation* rel = new MChainGearRelation();
        rel->part1 = c;
        rel->part2 = cache[g2][0]; // TODO
        rel->dir = (dirs[i] == 'l') ? 1 : -1;
        rel->prev = cache[g1][0]; // TODO
        rel->next = cache[g3][0]; // TODO
        rel->segID = j;
        c->addNeighbor(cache[g2][0], rel); // TODO
    }
    c->setDirs(dirs);
    parts.push_back(c);
    return c->init();
}

bool MChange::isNull() {
    auto eps = 1e-6;
    return (abs(a) < eps && abs(a) < eps);
}

MChange MPart::getChange() { return change; }

void VRMechanism::updateNeighbors() {
    for (auto p : parts) p->apply(); // first apply the part transformations
    for (auto p : parts) p->updateNeighbors(parts);
}

void VRMechanism::update() {
    //cout << "\nVRMechanism::update" << endl;

    vector<MPart*> changed_parts;
#ifndef WITHOUT_BULLET
    for (auto& part : parts) {
        if (part->resetPhysics) {
            part->trans->getPhysics()->setDynamic(true, true);
            part->resetPhysics = false;
        }
    }
#endif

    //for (auto& part : parts) part->change = MChange();
    for (auto& part : parts) if (part->changed()) changed_parts.push_back(part);

    for (auto& part : changed_parts) {
        part->updateNeighbors(parts);
        part->computeState();
        part->computeChange();
        //part->printChange();
    }

    for (auto& part : changed_parts) {
        //cout << " update changes " << part->geo->getName() << endl;
        if (part->getChange().isNull()) continue;
        bool block = !part->propagateMovement();
        if (block && 0) { // mechanism is blocked, TODO: add parameter to allow blocking or not
            cout << "  block!" << endl;
            for (auto part : changed_parts) {
                if (part->state == MPart::ENGAGED) part->setBack();
            }
            return;
        }
    }

    for (auto part : parts) part->apply();
    for (auto part : changed_parts) part->changed();
}

void VRMechanism::updateVisuals() {
    if (!geo) geo = VRAnalyticGeometry::create();
    if(!geo->isVisible("", true)) return;
    addChild(geo);
    geo->clear();

    // visualize part params
    for (auto p : parts) {
        if (p->type != "gear") continue;
        VRGear* g = (VRGear*)p->prim;
        Vec3d a = ((MGear*)p)->rAxis;
        Vec3d o = ((MGear*)p)->offset;
        float w = g->width*0.5;
        auto u = Vec3d(0,1,0); // TODO: change to orthogonal to a!
        float ts = g->teeth_size;
        float r = g->radius();
        a.normalize();
        Vec3d pos = Vec3d(p->reference[3]) + o;
        geo->addVector(pos, a*w, Color3f(0.2,1,0.3));

        // last change
        Vec3d t = p->change.n.cross(u);
        Vec3d cP = pos + a*w + u*(r+ts*0.5);
        float A = p->change.a;
        float d = p->change.n.dot(a);
        if (abs(d) > 1e-2) A /= d;
        geo->addVector(cP, p->change.n*w, Color3f(0.9,0,0.7)); // change rot axis
        geo->addVector(cP, t*A, Color3f(0.6,0,0.4)); // tangant * angle
        geo->addVector(cP, u*A, Color3f(0.2,0,0.4)); // up * angle


        bool b = false;
        for (auto p2 : p->neighbors) {
            if (p2.first->type != "gear") continue;
            if (p2.second->type != "gear") continue;
            auto pos2 = Vec3d(p2.first->reference[3]);
            pos2 += ((MGear*)p2.first)->offset;
            auto d = pos2-pos;
            d.normalize();
            float ts = g->teeth_size;
            geo->addVector(pos + a*w, d*(r-ts*0.5), Color3f(1,1,0));
            geo->addVector(pos + a*w + d*(r-ts*0.5), d*ts, Color3f(0.9,0.4,0));
            b = true;
        }

        if (!b) {
            geo->addVector(pos + a*w, u*(r-ts*0.5), Color3f(1,1,0));
            geo->addVector(pos + a*w + u*(r-ts*0.5), u*ts, Color3f(0.9,0.4,0));
        }
    }

    // visualize neighbor relations
    for (auto p1 : parts) {
        auto pos1 = Vec3d(p1->reference[3]);
        if (p1->type == "gear") pos1 += ((MGear*)p1)->offset;
        for (auto p2 : p1->neighbors) {
            auto color = Color3f(0.4,0.6,1.0);
            if (p1->type == "chain" || p2.first->type == "chain") color = Color3f(1.0,0.6,0.4);
            auto pos2 = Vec3d(p2.first->reference[3]);
            if (p2.first->type == "gear") pos2 += ((MGear*)p2.first)->offset;
            Vec3d d = (pos2-pos1)*0.45;
            geo->addVector(pos1, d, color);
        }
    }
}



