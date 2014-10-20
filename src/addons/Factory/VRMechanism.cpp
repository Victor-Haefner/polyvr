#include "VRMechanism.h"
#include "core/objects/geometry/VRPrimitive.h"

OSG_BEGIN_NAMESPACE;

void VRProfile::add(Vec2f v) { pnts.push_back(v); }

vector<Vec3f> VRProfile::get(Vec3f n, Vec3f u) {
    vector<Vec3f> res;
    Vec3f v;
    Vec3f x = u.cross(n);
    for (uint i=0; i<pnts.size(); i++) {
        v = x*pnts[i][0] + u*pnts[i][1];
        res.push_back(v);
    }
    return res;
}


MPart::MPart() {;}
MPart::~MPart() { ; }
void MPart::move(float dx) {}
bool MPart::changed() {
    if (timestamp == geo->getLastChange()) return false;
    else {
        timestamp = geo->getLastChange();
        return true;
    }
}

void MPart::setBack() { geo->setWorldMatrix(reference); }
void MPart::apply() { reference = geo->getWorldMatrix(); }

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

void MPart::computeChange() {
    Matrix M = reference;
    M.invert();
    M.mult(geo->getWorldMatrix());

    Vec3f t(M[3]);

}

void MPart::clearNeighbors() {
    for (auto n : neighbors) n->neighbors.erase( remove( begin(n->neighbors), end(n->neighbors), this ), end(n->neighbors) );
    neighbors.clear();
}

void MPart::addNeighbor(MPart* p) {
    neighbors.push_back(p);
    p->neighbors.push_back(this);
}

bool MPart::propagateMovement() {
    //for (uint i=0; i<neighbors.size(); i++) neighbors[i]->move(dx);
    return true;
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
    if ( find(c->neighbors.begin(), c->neighbors.end(), p)!=c->neighbors.end() ) return false;
    return true;
}

/*bool checkThreadNut(VRThread* t, VRNut* n, Matrix r1, Matrix r2) {
    ; // TODO: check if nut center on thread line
    ; // TODO: check if nut and thread same orientation
    return true;
}*/

MGear::MGear() {}
MGear::~MGear() {}
void MGear::move(float dx) {
    VRGear* g = (VRGear*)prim;
    dx *= -1; //gears invert direction of movement
    Vec3f n = Vec3f(0,0,1);
    float a = dx/g->radius();
    trans->rotate(a,n);
}
void MGear::updateNeighbors(vector<MPart*> parts) {
    clearNeighbors();
    VRGear* self = (VRGear*)prim;
    for (auto part : parts) {
        VRPrimitive* p = part->geo->getPrimitive();
        if (p->getType() == "Gear") {
            if (checkGearGear(self, (VRGear*)p, reference, part->reference) )
                addNeighbor(part);
        }
        if (p->getType() == "Thread") {
            if (checkGearThread(self, (VRThread*)p, reference, part->reference) )
                addNeighbor(part);
        }
        if (p->getType() == "Chain") {
            if (checkChainPart((MChain*)part, this, reference, part->reference) )
                addNeighbor(part);
        }
    }
}

MChain::MChain() {}
MChain::~MChain() {}
void MChain::move(float dx) {
    ;
}

void MChain::updateNeighbors(vector<MPart*> parts) {
    clearNeighbors();
    for (auto p : parts) {
        if (p->prim->getType() == "Gear") {
            if (checkChainPart(this, p, reference, p->reference) )
                addNeighbor(p);
        }
    }
}

MThread::MThread() {}
MThread::~MThread() {}
void MThread::move(float dx) {
    ;// thread is allways moved by rotation -> convert rotation to new dx
}

void MThread::updateNeighbors(vector<MPart*> parts) {
    clearNeighbors();
    VRThread* self = (VRThread*)geo->getPrimitive();
    for (auto part : parts) {
        VRPrimitive* p = part->geo->getPrimitive();
        if (p->getType() == "Gear") {
            if (checkGearThread((VRGear*)p, self, reference, part->reference) )
                addNeighbor(part);
        }
    }
}

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

void VRMechanism::addChain(float w, vector<VRGeometry*> geos) {
    MChain* c = new MChain();
    parts.push_back(c);
    for(auto g : geos) {
        if (cache.count(g) == 0) continue;
        c->addNeighbor(cache[g]);
    }
}

void VRMechanism::update() {
    vector<MPart*> changed_parts;
    for (auto& part : parts) if (part->changed()) changed_parts.push_back(part);

    for (auto& part : changed_parts) {
        part->updateNeighbors(parts);
        part->computeChange();
    }

    for (auto& part : changed_parts) {
        bool block = !part->propagateMovement();
        if (block) { // mechanism is blocked
            for (auto part : changed_parts) part->setBack();
            return;
        }
    }

    for (auto part : parts) part->apply();
}


OSG_END_NAMESPACE;
