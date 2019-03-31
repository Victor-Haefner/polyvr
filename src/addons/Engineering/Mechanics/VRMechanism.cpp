#include "VRMechanism.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"

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
MGear::MGear() {}
MGear::~MGear() {}
MChain::MChain() {}
MChain::~MChain() {}
MThread::MThread() {}
MThread::~MThread() {}

bool MPart::changed() {
    if (geo == 0) return false;
    cout << "  part " << geo->getName() << " changed from timestamp " << timestamp << " (last change: " << geo->getLastChange() << ")";
    bool b = geo->changedSince(timestamp, true);
    cout << " to " << timestamp << ", -> " << b << endl;
    return b;
}

void MPart::setBack() { if (geo) geo->setWorldMatrix(reference); }
void MPart::apply() {
    if (geo) reference = geo->getWorldMatrix();
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
    cout << "      MPart::addNeighbor " << p->geo->getName() << endl;
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

bool MChange::same(MChange c) {
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
    if (change.time == c.time) {
        cout << " propagateMovement change times are the same?" << endl;
        return change.same(c);
    } // TODO: either it is the same change || another change in the same timestep..

    cout << " propagateMovement do move" << endl;
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
    //Matrix4d r1 = p1->geo->getWorldMatrix();
    //Matrix4d r2 = p2->geo->getWorldMatrix();

    VRGear* g1 = (VRGear*)p1->prim;
    VRGear* g2 = (VRGear*)p2->prim;
    float R = g1->radius() + g2->radius();
    Vec3d d = Vec3d(r1[3] - r2[3]);
    float D = d.length();
    float t = 0.5*g1->teeth_size;
    if (R+t < D || R-t > D) {
        //cout << "     checkGearGear failed! D: " << D << ", R1 " << R-t << " -> R2 " << R+t << endl;
        return 0; // too far apart
    }
    ;// TODO: check if intersection line of gear planes is at the edge of both gears
    ;// TODO: check if coplanar

    MGearGearRelation* rel = new MGearGearRelation();
    rel->part1 = p1;
    rel->part2 = p2;
    //cout << "     checkGearGear found between " << p1->geo->getName() << " and " << p2->geo->getName() << endl;
    return rel;
}

MRelation* checkGearThread(MPart* p1, MPart* p2) {
    //float R = g->radius() + t->radius;
    ; // TODO: check if line center distance is the gear + thread radius
    ; // TODO: check if thread && gear coplanar
    return 0;
}

MChainGearRelation* checkChainPart(MChain* c, MPart* p) {
    Vec3d pp = p->geo->getWorldPosition();
    if (p->prim->getType() != "Gear") return 0;
    float r = ((VRGear*)p->prim)->radius();
    Vec3d dir = p->geo->getWorldDirection();

    float eps = 0.0001;

    vector<pointPolySegment> psegs;
    for (auto ps : c->toVRPolygon(pp) ) if ( abs(ps.dist2 - r*r) < eps ) psegs.push_back(ps);
    //for (auto ps : c->toVRPolygon(pp) ) psegs.push_back(ps);
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

vector<pointPolySegment> MChain::toVRPolygon(Vec3d p) {
    vector<pointPolySegment> res;
    for (uint i=0; i<VRPolygon.size(); i+=2) {
        Vec3d p1 = VRPolygon[i];
        Vec3d p2 = VRPolygon[i+1];
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
void MGear::move() { cout << " gear move " << geo->getName() << endl ; trans->rotate(change.dx/gear()->radius(), Vec3d(0,0,1)); }
void MChain::move() { if (geo == 0) return; updateGeo(); }
void MThread::move() { trans->rotate(change.a, Vec3d(0,0,1)); }

void MPart::computeChange() {
    Matrix4d m = reference;
    m.invert();
    m.mult(geo->getWorldMatrix());

    change.t = Vec3d(m[3]);
    change.l = change.t.length();

    change.a = acos( (m[0][0] + m[1][1] + m[2][2] - 1)*0.5 );
    change.n = Vec3d(m[2][1] - m[1][2], m[0][2] - m[2][0], m[1][0] - m[0][1]);
    change.n.normalize();

    if (change.n[2] < 0) { change.n *= -1; change.a *= -1; }

    change.time = timestamp;
}

void MGear::computeChange() {
    MPart::computeChange();
    change.dx = change.a*gear()->radius();
}

void MGear::updateNeighbors(vector<MPart*> parts) {
    cout << "   MGear::updateNeighbors of " << geo->getName() << endl;
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

    // collect all VRPolygon points
    //printNeighbors();
    VRPolygon.clear();
    vector<MPart*> nbrs;
    map<int, MPart*> nbrs_m;
    for (auto n : neighbors) nbrs_m[((MChainGearRelation*)n.second)->segID] = n.first;
    for (auto n : nbrs_m) nbrs.push_back(n.second);

    int j=0;
    for (uint i=0; i<nbrs.size(); i++) {
        j = (i+1) % nbrs.size(); // next
        VRPrimitive* p1 = nbrs[i]->prim;
        VRPrimitive* p2 = nbrs[j]->prim;
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
        VRPolygon.push_back(t1);
        VRPolygon.push_back(t2);

        /*Vec3d check = t2-t1;
        cout << nbrs[i]->geo->getName() << " " << nbrs[j]->geo->getName();
        cout << " check " << check.dot(t1-c1) << " " << check.dot(t2-c2);
        cout << " r1 " << r1 << " r2 " << r2;
        cout << endl;*/
    }

    // draw VRPolygon
    j=0;
    for (uint i=0; i<VRPolygon.size(); i+=2) {
        Vec3d p1 = VRPolygon[i];
        Vec3d p2 = VRPolygon[i+1];
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

VRMechanism::VRMechanism() {;}
VRMechanism::~VRMechanism() { clear();}
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

void VRMechanism::addGear(VRTransformPtr trans, float width, float hole, float pitch, int N_teeth, float teeth_size, float bevel) {
    auto gPrim = VRPrimitive::create("Gear "+toString(width)+" "+toString(hole)+" "+toString(pitch)+" "+toString(N_teeth)+" "+toString(teeth_size)+" "+toString(bevel));
    auto p = new MGear();
    p->geo = trans;
    p->prim = gPrim;
    p->trans = trans;
    cache[trans] = p;
    parts.push_back(p);
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
    cout << "\nVRMechanism::update" << endl;

    vector<MPart*> changed_parts;
    for (auto& part : parts) if (part->changed()) changed_parts.push_back(part);

    for (auto& part : changed_parts) {
        part->updateNeighbors(parts);
        part->computeState();
        part->computeChange();
        part->printChange();
    }

    for (auto& part : changed_parts) {
        cout << " update changes " << part->geo->getName() << endl;
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

shared_ptr<VRMechanism> VRMechanism::create() { return shared_ptr<VRMechanism>(new VRMechanism()); }

