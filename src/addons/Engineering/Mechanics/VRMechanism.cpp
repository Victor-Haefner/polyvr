#include "VRMechanism.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPrimitive.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

OSG_BEGIN_NAMESPACE;


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
    bool b = (timestamp != geo->getLastChange());
    timestamp = geo->getLastChange();
    return b;
}

void MPart::setBack() { if (geo) geo->setWorldMatrix(reference); }
void MPart::apply() { if (geo) reference = geo->getWorldMatrix(); }

MPart* MPart::make(VRGeometry* g, VRTransform* t) {
    string type = g->getPrimitive()->getType();
    MPart* p = 0;
    if (type == "Gear") p = new MGear();
    if (type == "Thread") p = new MThread();
    if (type == "Chain") p = new MChain();
    if (p) {
        p->geo = g;
        p->prim = g->getPrimitive();
        p->trans = t == 0 ? g : t;
    }
    return p;
}

void MPart::clearNeighbors() {
    for (auto n : neighbors) {
        //if (n->prim == 0) continue; // TODO: chain ?
        n->neighbors.erase( remove( begin(n->neighbors), end(n->neighbors), this ), end(n->neighbors) );
    }
    neighbors.clear();
}

void MPart::addNeighbor(MPart* p) {
    neighbors.push_back(p);
    p->neighbors.push_back(this);
}

bool MPart::hasNeighbor(MPart* p) {
    for (auto n : neighbors) if (n == p) return true;
    return false;
}

void MPart::computeState() {
    int N = neighbors.size();
    if (state == FREE and N > 0) state = ENGAGING;
    if (state == ENGAGING and N > 0) state = ENGAGED;
    if (state == ENGAGED and N == 0) state = DISENGAGING;
    if (state == DISENGAGING and N == 0) state = FREE;
}

void MChange::flip() {
    a *= -1;
    dx *= -1;
}

bool MChange::same(MChange c) {
    if (a*c.a < 0) return false;
    return true;
}

bool MPart::propagateMovement() {
    bool dop = (geo->getName() == "Gear.2");

    bool res = true;
    for (auto n : neighbors) {
        res = n->propagateMovement(change) ? res : false;
        //if (dop) cout << " d " << n->geo->getName() << endl;
    }
    return res;
}

bool MPart::propagateMovement(MChange c) {
    c.flip();
    if (change.time == c.time) {
        return change.same(c);
    } // TODO: either it is the same change or another change in the same timestep..

    change = c;
    move();

    return propagateMovement();
}

void MPart::printChange() {
    cout.precision(2);
    cout.setf( ios::fixed, ios::floatfield );
    cout << geo->getName();
    cout << " trans: " << change.l;
    cout << " (vec: " << change.t << ") ";
    cout << "and rot " << change.a << " around " << change.n;
    cout << endl;
}

void MPart::printNeighbors() {
    cout << geo->getName();
    cout << " neighbors: ";
    for (auto n : neighbors) cout << " " << n->geo->getName();
    cout << endl;
}


bool checkGearGear(VRGear* g1, VRGear* g2, Matrix r1, Matrix r2) {
    float R = g1->radius() + g2->radius();
    Vec3f d = Vec3f(r1[3] - r2[3]);
    float D = d.length();
    float t = 0.5*g1->teeth_size;
    if (R+t < D or R-t > D) return false; // too far apart
    ;// TODO: check if intersection line of gear planes is at the edge of both gears
    ;// TODO: check if coplanar
    return true;
}

bool checkGearThread(VRGear* g, VRThread* t, Matrix r1, Matrix r2) {
    //float R = g->radius() + t->radius;
    ; // TODO: check if line center distance is the gear + thread radius
    ; // TODO: check if thread and gear coplanar
    return true;
}

bool checkChainPart(MChain* c, MPart* p, Matrix r1, Matrix r2) {
    auto pos = c->geo->getMesh()->getPositions();
    Vec3f pp = p->geo->getFrom();
    for (int i=0; i<pos->size(); i++) {
        Vec3f cp = pos->getValue<Pnt3f>(i).subZero() - pp;
        if (cp.length() < 0.01) return true;
    }
    return false;
}

/*bool checkThreadNut(VRThread* t, VRNut* n, Matrix r1, Matrix r2) {
    ; // TODO: check if nut center on thread line
    ; // TODO: check if nut and thread same orientation
    return true;
}*/

VRGear* MGear::gear() { return (VRGear*)prim; }
VRThread* MThread::thread() { return (VRThread*)prim; }

void MPart::move() {}
void MGear::move() {
    trans->rotate(change.dx/gear()->radius(), Vec3f(0,0,1));
}
void MChain::move() {
    if (geo == 0) return;
    updateGeo();
}
void MThread::move() { trans->rotate(change.a, Vec3f(0,0,1)); }

void MPart::computeChange() {
    Matrix m = reference;
    m.invert();
    m.mult(geo->getWorldMatrix());

    change.t = Vec3f(m[3]);
    change.l = change.t.length();

    change.a = acos( (m[0][0] + m[1][1] + m[2][2] - 1)*0.5 );
    change.n = Vec3f(m[2][1] - m[1][2], m[0][2] - m[2][0], m[1][0] - m[0][1]);
    change.n.normalize();

    if (change.n[2] > 0) { change.n *= -1; change.a *= -1; }

    change.time = timestamp;
}

void MGear::computeChange() {
    MPart::computeChange();
    change.dx = change.a*gear()->radius();
}

void MGear::updateNeighbors(vector<MPart*> parts) {
    clearNeighbors();
    for (auto part : parts) {
        if (part == this) continue;
        VRPrimitive* p = part->prim;

        if (p == 0) { // chain
            bool b = checkChainPart((MChain*)part, this, reference, part->reference);
            if (b) addNeighbor(part);
            continue;
        }
        if (p->getType() == "Gear") {
            bool b = checkGearGear(gear(), (VRGear*)p, reference, part->reference);
            if (b) addNeighbor(part);
        }
        if (p->getType() == "Thread") {
            bool b = checkGearThread(gear(), (VRThread*)p, reference, part->reference);
            if (b) addNeighbor(part);
        }
    }
}

void MChain::updateNeighbors(vector<MPart*> parts) {
    clearNeighbors();
    for (auto p : parts) {
        if (p == this) continue;
        if (p->prim->getType() == "Gear") {
            if (checkChainPart(this, p, reference, p->reference) ) addNeighbor(p);
        }
    }
}

void MThread::updateNeighbors(vector<MPart*> parts) {
    clearNeighbors();
    VRThread* self = (VRThread*)geo->getPrimitive();
    for (auto part : parts) {
        if (part == this) continue;
        VRPrimitive* p = part->geo->getPrimitive();
        if (p->getType() == "Gear") {
            if (checkGearThread((VRGear*)p, self, reference, part->reference) ) addNeighbor(part);
        }
    }
}

void MChain::set(string dirs) {
    if (neighbors.size() > dirs.size()) return;
    this->dirs = dirs;
}

void MChain::updateGeo() {
    // update chain geometry
    GeoVec3fPropertyRecPtr pos = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
    GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();

    // collect all polygon points
    vector<Vec3f> pnts;
    for (int i=0; i<neighbors.size(); i++) {
        //VRPrimitive* p = n->prim;
        //if (p->getType() != "Gear") continue;
        //VRGear* g = (VRGear*)p;
        char dir = dirs[i]; // TODO: use dir for the right points
        pnts.push_back(neighbors[i]->geo->getFrom());
    }
    if (neighbors.size() > 0) pnts.push_back(neighbors[0]->geo->getFrom());

    // draw polygon
    Vec3f p0,p1;
    if (pnts.size() > 0) p0 = pnts[0];
    int j=0;
    for (int i=1; i<pnts.size(); i++) {
        p1 = pnts[i];
        pos->addValue(p0);
        pos->addValue(p1);
        norms->addValue(Vec3f(0,1,0));
        norms->addValue(Vec3f(0,1,0));
        cols->addValue(Vec4f(0,1,0,1));
        cols->addValue(Vec4f(0,1,0,1));
        inds->addValue(j++);
        inds->addValue(j++);
        p0 = p1;
    }
    lengths->addValue(j);

    geo->setPositions(pos);
    geo->setNormals(norms);
    geo->setColors(norms);
    geo->setIndices(inds);
    geo->setLengths(lengths);
}

VRGeometry* MChain::init() {
    geo = new VRGeometry("chain");
    VRMaterial* cm = new VRMaterial("chain_mat");
    cm->setLit(false);
    cm->setLineWidth(4);
    geo->setMaterial(cm);
    geo->setType(GL_LINES);
    updateGeo();
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

void VRMechanism::add(VRGeometry* part, VRTransform* trans) {
    MPart* p = MPart::make(part, trans);
    if (p == 0) return;
    cache[part] = p;
    parts.push_back(p);
}

VRGeometry* VRMechanism::addChain(float w, vector<VRGeometry*> geos, string dirs) {
    MChain* c = new MChain();
    for (auto g : geos) {
        if (cache.count(g) == 0) continue;
        c->addNeighbor(cache[g]);
    }
    c->set(dirs);
    parts.push_back(c);
    return c->init();
}

void VRMechanism::update() {
    vector<MPart*> changed_parts;
    for (auto& part : parts) if (part->changed()) changed_parts.push_back(part);

    for (auto& part : changed_parts) {
        part->updateNeighbors(parts);
        part->computeState();
        part->computeChange();
        //part->printChange();
    }

    for (auto& part : changed_parts) {
        bool block = !part->propagateMovement();
        if (block) { // mechanism is blocked
            for (auto part : changed_parts) {
                if (part->state == MPart::ENGAGED) part->setBack();
            }
            return;
        }
    }

    for (auto part : parts) part->apply();
}


OSG_END_NAMESPACE;
