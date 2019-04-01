#include "VRMechanism.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"
#include "core/utils/isNan.h"
#include "core/tools/VRAnalyticGeometry.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;

template<> string typeName(const VRMechanism& m) { return "Mechanism"; }


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


MGearGearRelation* checkGearGear(MPart* p1, MPart* p2) {
    Matrix4d r1 = p1->reference;
    Matrix4d r2 = p2->reference;
    Vec3d P1 = Vec3d(r1[3]);
    Vec3d P2 = Vec3d(r2[3]);
    Vec3d n1 = Vec3d(r1[2]); n1.normalize();
    Vec3d n2 = Vec3d(r2[2]); n2.normalize();

    VRGear* g1 = (VRGear*)p1->prim;
    VRGear* g2 = (VRGear*)p2->prim;
    Vec3d d = P2 - P1;
    float t = g1->teeth_size;

    Vec3d R1 = d - n1*n1.dot( d); R1.normalize();
    Vec3d R2 =-d - n2*n2.dot(-d); R2.normalize();
    R1 *= g1->radius();
    R2 *= g2->radius();
    if ( (P1+R1 - (P2+R2)).length() > t) return 0; // not touching!

    MGearGearRelation* rel = new MGearGearRelation();
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
    Vec3d wpos = p->geo->getWorldPosition();
    if (!p->prim) return 0;
    if (p->prim->getType() != "Gear") return 0;
    float r = ((VRGear*)p->prim)->radius();
    Vec3d dir = p->geo->getWorldDirection();

    float eps = 1e-4; // 1e-4

    vector<pointPolySegment> psegs;
    for (auto ps : c->toPolygon(wpos) ) {
        double d = abs(ps.dist2 - r*r);
        if ( d < eps ) {
            //cout << "checkChainPart " << d << endl;
            psegs.push_back(ps);
        }
    }

    //for (auto ps : c->toPolygon(pp) ) psegs.push_back(ps);
    if (psegs.size() == 0) return 0;

    dir.normalize();
    float fd = 0;
    int IDmin = 1e6;

    for (auto ps : psegs) {
        ps.Pseg.normalize();
        ps.seg.normalize();
        float n = ps.Pseg.cross(dir).dot(ps.seg);
        if (abs(n) < (1-eps)) continue;
        if (IDmin > ps.ID) {
            IDmin = ps.ID;
            fd = n;
        }
    }

    MChainGearRelation* rel = new MChainGearRelation();
    rel->part1 = c;
    rel->part2 = p;
    rel->dir = fd > 0 ? -1 : 1;
    rel->segID = IDmin;
    return rel;
}

vector<pointPolySegment> MChain::toPolygon(Vec3d p) {
    vector<pointPolySegment> res;
    for (uint i=0; i<polygon.size(); i+=2) {
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
void MGear::move() { trans->rotate(change.dx/gear()->radius(), Vec3d(0,0,1)); trans->setBltOverrideFlag(); trans->updatePhysics(); }
void MChain::move() { if (geo == 0) return; updateGeo(); }
void MThread::move() { trans->rotate(change.a, Vec3d(0,0,1)); }

void MPart::computeChange() {
    Matrix4d m = reference;
    m.invert();
    m.mult(geo->getWorldMatrix());

    change.t = Vec3d(m[3]);
    change.l = change.t.length();

    change.a = acos( (m[0][0] + m[1][1] + m[2][2] - 1)*0.5 );
    if (isNan(change.a)) change.a = 0;

    change.n = Vec3d(m[2][1] - m[1][2], m[0][2] - m[2][0], m[1][0] - m[0][1]);
    change.n.normalize();

    if (change.n[2] < 0) { change.n *= -1; change.a *= -1; }
    if (abs(change.a) < 1e-5) return;
    change.time = timestamp;
    change.origin = this;
}

void MGear::computeChange() {
    MPart::computeChange();
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
            continue;
        }
        if (p->getType() == "Gear") {
            MRelation* rel = checkGearGear(this, part);
            //cout << "     MGear::updateNeighbors check Gear, rel " << rel << endl;
            if (rel) addNeighbor(part, rel);
        }
        if (p->getType() == "Thread") {
            MRelation* rel = checkGearThread(this, part);
            if (rel) addNeighbor(part, rel);
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

void MRelation::translateChange(MChange& change) {;}
void MGearGearRelation::translateChange(MChange& change) { change.flip();}
void MChainGearRelation::translateChange(MChange& change) { if (dir == -1) change.flip();}

void MChain::setDirs(string dirs) { this->dirs = dirs; }
void MChain::addDir(char dir) { dirs.push_back(dir); }

void MChain::updateGeo() {
    //cout << "update chain\n";
    // update chain geometry
    GeoVec3fPropertyRecPtr pos = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
    GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();

    // collect all polygon points
    //printNeighbors();
    polygon.clear();
    vector<MPart*> nbrs;
    map<int, MPart*> nbrs_m;
    for (auto n : neighbors) nbrs_m[((MChainGearRelation*)n.second)->segID] = n.first;
    for (auto n : nbrs_m) nbrs.push_back(n.second);

    int j=0;
    for (uint i=0; i<nbrs.size(); i++) {
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

        float r1 = g1->radius();
        float r2 = g2->radius();
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

        Vec3d dn = D/d;
        Vec3d t1 = dn.cross(nbrs[i]->geo->getWorldDirection());
        Vec3d t2 = dn.cross(nbrs[j]->geo->getWorldDirection());
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

    // draw polygon
    j=0;
    for (uint i=0; i<polygon.size(); i+=2) {
        Vec3d p1 = polygon[i];
        Vec3d p2 = polygon[i+1];
        pos->addValue(p1);
        pos->addValue(p2);
        norms->addValue(Vec3d(0,1,0));
        norms->addValue(Vec3d(0,1,0));
        cols->addValue(Vec4d(0,1,0,1));
        cols->addValue(Vec4d(1,1,0,1));
        inds->addValue(j++);
        inds->addValue(j++);
    }
    lengths->addValue(j);

    VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(geo);
    if (g) {
        g->setPositions(pos);
        g->setNormals(norms);
        g->setColors(cols);
        g->setIndices(inds);
        g->setLengths(lengths);
    }

    Vec3d c;
    for (auto v : polygon) c += v;
    c *= (1.0/polygon.size());
    reference[3] = Vec4d(c[0], c[1], c[2], 1 );
}

VRTransformPtr MChain::init() {
    geo = VRGeometry::create("chain");
    updateGeo();
    VRMaterialPtr cm = VRMaterial::get("chain_mat");
    cm->setLit(false);
    cm->setLineWidth(3);
    VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(geo);
    if (g) {
        g->setMaterial(cm);
        g->setType(GL_LINES);
    }
    return geo;
}

// -------------- mechanism ------------------------

VRMechanism::VRMechanism() : VRObject("mechanism") {;}
VRMechanism::~VRMechanism() { clear();}

shared_ptr<VRMechanism> VRMechanism::create() { return shared_ptr<VRMechanism>(new VRMechanism()); }

void VRMechanism::clear() {
    for (auto part : parts) delete part;
    parts.clear();
    cache.clear();
}

void VRMechanism::add(VRTransformPtr part, VRTransformPtr trans) {
    MPart* p = MPart::make(part, trans);
    if (p == 0) return;

    cache[part] = p;
    parts.push_back(p);
    p->apply();
    p->updateNeighbors(parts);
}

void VRMechanism::addGear(VRTransformPtr part, float width, float hole, float pitch, int N_teeth, float teeth_size, float bevel) {
    auto p = new MGear();
    p->prim = new VRGear(width, hole, pitch, N_teeth, teeth_size, bevel);
    p->geo = part;
    p->trans = part;
    cache[part] = p;
    parts.push_back(p);
    p->apply();
    p->updateNeighbors(parts);
}

VRTransformPtr VRMechanism::addChain(float w, vector<VRTransformPtr> geos, string dirs) {
    MChain* c = new MChain();
    for (uint i=0; i<geos.size(); i++) {
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
        rel->part2 = cache[g2];
        rel->dir = (dirs[i] == 'l') ? 1 : -1;
        rel->prev = cache[g1];
        rel->next = cache[g3];
        rel->segID = j;
        c->addNeighbor(cache[g2], rel);
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
        if (block) { // mechanism is blocked
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
    if (!geo) {
        geo = VRAnalyticGeometry::create();
        addChild(geo);
    }

    geo->clear();

    for (auto p : parts) {
        if (p->type != "gear") continue;
        VRGear* g = (VRGear*)p->prim;
        Vec3d n = Vec3d(p->reference[2]); n.normalize(); n *= g->radius();
        geo->addVector(Vec3d(p->reference[3]), n, Color3f(0.2,1,0.3));
    }

    for (auto p1 : parts) {
        auto pos1 = Vec3d(p1->reference[3]);
        for (auto p2 : p1->neighbors) {
            auto color = Color3f(0.4,0.6,1.0);
            if (p1->type == "chain" || p2.first->type == "chain") color = Color3f(1.0,0.6,0.4);
            auto pos2 = Vec3d(p2.first->reference[3]);
            geo->addVector(pos1, pos2-pos1, color);
        }
    }
}



