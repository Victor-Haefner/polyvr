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

bool MPart::propagateMovement() { // recursion
    bool res = true;
    for (auto n : neighbors) {
        res = n->propagateMovement(change) ? res : false;
    }
    return res;
}

bool MPart::propagateMovement(MChange c) { // change
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
    Vec3f pp = p->geo->getFrom();
    if (p->prim->getType() != "Gear") return false;
    float r = ((VRGear*)p->prim)->radius();
    Vec3f dir = p->geo->getDir();

    float eps = 0.0001;

    Vec3f pc, sd;
    c->toPolygon(pp, pc, sd); // point, vec to nearest seg, dir of seg
    bool res = ( abs(pc.squareLength() - r*r) < eps );

    if (res) {
        pc.normalize();
        sd.normalize();
        dir.normalize();
        float n = pc.cross(dir).dot(sd);
        if (abs(n) < (1-eps)) return false;

        char d = n > 0 ? 'r' : 'l';
        c->addDir(d);
    }

    return res;
}

/*bool checkThreadNut(VRThread* t, VRNut* n, Matrix r1, Matrix r2) {
    ; // TODO: check if nut center on thread line
    ; // TODO: check if nut and thread same orientation
    return true;
}*/

VRGear* MGear::gear() { return (VRGear*)prim; }
VRThread* MThread::thread() { return (VRThread*)prim; }

void MPart::move() {}
void MGear::move() { trans->rotate(change.dx/gear()->radius(), Vec3f(0,0,1)); }
void MChain::move() { /*if (geo == 0) return; updateGeo();*/ }
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
    dirs = "";
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

void MChain::setDirs(string dirs) { this->dirs = dirs; }
void MChain::addDir(char dir) { dirs.push_back(dir); }

void MChain::toPolygon(Vec3f p, Vec3f& ps, Vec3f& sd) {
    float l2min = 1e12;
    ps = Vec3f(sqrt(l2min),0,0);

    for (int i=0; i<polygon.size(); i+=2) {
        Vec3f p1 = polygon[i];
        Vec3f p2 = polygon[i+1];
        Vec3f d = p2-p1;
        Vec3f d1 = p-p1;
        Vec3f d2 = p-p2;

        Vec3f dx;
        float dist = 0;
        float l2 = d.squareLength();
        if (l2 == 0.0) dx = d1;
        else {
            float t = d1.dot(d)/l2;
            if (t < 0.0) dx = d1;
            else if (t > 1.0) dx = d2;
            else dx = p1 + t*d - p; // vector from p to p1p2
        }

        float l = dx.squareLength();
        if (l > l2min) continue;
        l2min = l;
        ps = dx;
        sd = d;
    }
}

void MChain::updateGeo() {
    //cout << "update chain\n";
    // update chain geometry
    GeoVec3fPropertyRecPtr pos = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
    GeoVec4fPropertyRecPtr cols = GeoVec4fProperty::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr lengths = GeoUInt32Property::create();

    // collect all polygon points
    printNeighbors();

    polygon.clear();
    int j=0;
    for (int i=0; i<neighbors.size(); i++) {
        j = (i+1) % neighbors.size(); // next
        VRPrimitive* p1 = neighbors[i]->prim;
        VRPrimitive* p2 = neighbors[j]->prim;
        if (p1->getType() != "Gear") continue;
        if (p2->getType() != "Gear") continue;
        VRGear* g1 = (VRGear*)p1;
        VRGear* g2 = (VRGear*)p2;

        int d1 = dirs[i] == 'r' ? -1 : 1;
        int d2 = dirs[j] == 'r' ? -1 : 1;
        int z1 = -1*d1*d2;

        Vec3f c1 = neighbors[i]->geo->getFrom();
        Vec3f c2 = neighbors[j]->geo->getFrom();
        Vec3f D = c2-c1;
        float d = D.length();

        float r1 = g1->radius();
        float r2 = g2->radius();
        int z2 = r2 > r1 ? z1 : 1;
        r2 *= z1;

        float x1 = 0;
        float x2 = 0;
        float y1 = r1;
        float y2 = z1*r2;

        if (abs(r1+r2) > 0.001 ) {
            Vec3f Ch = c1*r2/(r1+r2) + c2*r1/(r1+r2); // homothetic center
            float dCh1 = (Ch-c1).length();
            float dCh2 = (Ch-c2).length();
            float rCh1 = sqrt( dCh1*dCh1 - r1*r1 );
            float rCh2 = sqrt( dCh2*dCh2 - r2*r2 );
            x1 =    z2*r1*r1  /dCh1;
            x2 = z1*z2*r2*r2  /dCh2;
            y1 =    d1*r1*rCh1/dCh1;
            y2 = z1*d2*r2*rCh2/dCh2;
        }

        Vec3f dn = D/d;
        Vec3f t1 = dn.cross(neighbors[i]->geo->getDir());
        Vec3f t2 = dn.cross(neighbors[j]->geo->getDir());
        t1 = c1 + t1*y1 + dn*x1;
        t2 = c2 + t2*y2 - dn*x2;
        polygon.push_back(t1);
        polygon.push_back(t2);

        /*Vec3f check = t2-t1;
        cout << neighbors[i]->geo->getName() << " " << neighbors[j]->geo->getName();
        cout << " check " << check.dot(t1-c1) << " " << check.dot(t2-c2);
        cout << " r1 " << r1 << " r2 " << r2;
        cout << endl;*/
    }

    // draw polygon
    j=0;
    for (int i=0; i<polygon.size(); i+=2) {
        Vec3f p1 = polygon[i];
        Vec3f p2 = polygon[i+1];
        pos->addValue(p1);
        pos->addValue(p2);
        norms->addValue(Vec3f(0,1,0));
        norms->addValue(Vec3f(0,1,0));
        cols->addValue(Vec4f(0,1,0,1));
        cols->addValue(Vec4f(0,1,0,1));
        inds->addValue(j++);
        inds->addValue(j++);
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
    c->setDirs(dirs);
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
