#include "VRLogistics.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/objects/VRTransform.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include <GL/glut.h>

#include <OpenSG/OSGMatrixUtility.h>

using namespace std;
using namespace OSG;


FID::FID() { static int i = 0; i++; ID = i; }
int FID::getID() { return ID; }


// --------------------------------------------------------------------- OBJECT

FObject::FObject() {}

FObject::~FObject() {
    ;
}

void FObject::setTransformation(VRTransform* t) {
    transform = t;
    t->set_orientation_mode(true);
    if (metaData) metaData->switchParent(t);
}

void FObject::setMetaData(string s) {
    return;
    if (metaData == 0) {
        metaData = new OSG::VRSprite("meta");
        metaData->setMaterial(new VRMaterial("metasprite"));
        metaData->switchParent(transform);
        metaData->setFrom(Vec3f(0,1,0));
        metaData->setDir(Vec3f(0,0,-1));
        metaData->setUp(Vec3f(0,1,0));
    }

    metaData->setLabel(s);
    float k = 0.2;
    metaData->setSize(0.5*k*s.size(), k);
}

VRTransform* FObject::getTransformation() {
    return transform;
}

void FObject::setType(FObject::Type t) { type = t; }
FObject::Type FObject::getType() { return type; }

bool FObject::move(OSG::path* p, float dx) {
    VRTransform* trans = getTransformation();
    if (trans == 0) return true;

    bool done = false;
    t += dx;
    if (t >= 1) { t = 1; done = true; }

    Matrix m;
    Vec3f dir, up, pos;
    pos = p->getPosition(t);
    p->getOrientation(t, dir, up);

    MatrixLookAt( m, pos, pos+dir, up );
    trans->setWorldMatrix(m);

    if (done) t = 0;
    return done;
}

// --------------------------------------------------------------------- NODE

FNode::FNode() : object(0), transporter(0), state(FREE) {}

FNode::~FNode() { ; }
FObject* FNode::get() { return object; }
void FNode::set(FObject* o) {
    object = o;
    if (o == 0) { setState(FREE); return; }
    if (o->getType() == FObject::CONTAINER) setState(CONTAINER);
    if (o->getType() == FObject::PRODUCT) setState(PRODUCT);
    if (o->getTransformation() == 0) return;

    auto t = o->getTransformation();
    Matrix wm;
    wm = t->getWorldMatrix();
    t->switchParent(getTransform());
    wm.setTranslate(getTransform()->getWorldPosition());
    t->setWorldMatrix(wm);
    t->update();
}

void FNode::connect(FNode* n) {
    if (n == 0) return;

    disconnect(n);
    out[n->getID()] = n;
    n->in[n->getID()] = this;
}

void FNode::disconnect(FNode* n) {
    int i = n->getID();
    int j = getID();
    if (out.count(i)) out.erase(i);
    if (n->out.count(j)) n->out.erase(j);
    if (in.count(i)) in.erase(i);
    if (n->in.count(j)) n->in.erase(j);
}

void FNode::isolate() {
    for (itr = out.begin(); itr != out.end(); itr++) disconnect(itr->second);
    for (itr = in.begin(); itr != in.end(); itr++) disconnect(itr->second);
    out.clear();
    in.clear();
}

void FNode::setState(State s) { state = s; }
FNode::State FNode::getState() { return state; }

map<int, FNode*>& FNode::getIncoming() { return in; }
map<int, FNode*>& FNode::getOutgoing() { return out; }

FNode* FNode::previous() { if (in.size() > 0) return in.begin()->second; else return 0; }
FNode* FNode::next() { if (out.size() > 0) return out.begin()->second; else return 0; }

void FNode::setTransform(OSG::VRTransform* t) { transform = t; }
VRTransform* FNode::getTransform() { return transform; }

Vec3f FNode::getTangent() {
    int Nout = out.size();
    int Nin = in.size();
    if (Nout == 0 && Nin == 0) return Vec3f(0,0,1);

    Vec3f t;
    //for (auto o : out) t += (o.second->transform->getFrom() - transform->getFrom());
    for (auto o : out) t += (o.second->transform->getWorldPosition() - transform->getWorldPosition());
    t.normalize();
    return t;
}


// --------------------------------------------------------------------- PATH

FPath::FPath() {}
std::vector<FNode*>& FPath::get() {return nodes; }

void FPath::set(FNode* n1, FNode* n2) { // TODO: A*
    nodes.clear();
    FNode* n = n1;
    nodes.push_back(n);
    while(n != n2 && n->next() > 0) {
        n = n->next(); // assumes linear networks
        nodes.push_back(n);
    }
    update();
}

void FPath::add(FNode* n) {
    nodes.push_back(n);
    update();
}

OSG::path* FPath::getPath(FNode* n) { return paths[n]; }

void FPath::update() {
    for (auto p : paths) delete p.second;
    paths.clear();

    for (unsigned int i=1; i<nodes.size(); i++) {
        FNode* n0 = nodes[i-1];
        FNode* n1 = nodes[i];
        path* p = new path();
        p->addPoint(n0->getTransform());
        p->addPoint(n1->getTransform());
        p->compute(12);
        paths[n1] = (p);
    }
}


// --------------------------------------------------------------------- CONTAINER

FContainer::FContainer() : capacity(100) {
    setType(CONTAINER);
}
FContainer::~FContainer() { ; }

void FContainer::setCapacity(int i) { capacity = i; }
int FContainer::getCapacity() { return capacity; }

void FContainer::add(FProduct* p) {
    auto t = p->getTransformation();
    t->hide();
    products.push_back(p);
    setMetaData("Nb: " + toString(products.size()));

    Matrix wm;
    wm = t->getWorldMatrix();
    t->switchParent(getTransformation());
    wm.setTranslate(getTransformation()->getWorldPosition());
    t->setWorldMatrix(wm);
    t->update();
}

FProduct* FContainer::pop() {
    FProduct* p = products.back();
    //p->getTransformation()->setMatrix(getTransformation()->getMatrix());
    products.pop_back();
    p->getTransformation()->show();
    setMetaData("Nb: " + toString(products.size()));
    //p->setMetaData("ID: " + toString(p->getID()));
    return p;
}

FProduct* FContainer::peek() {
    FProduct* p = products.back();
    return p;
}

void FContainer::clear() { products.clear(); }

bool FContainer::isFull() { return ((int)products.size() == capacity); }
bool FContainer::isEmpty() { return ((int)products.size() == 0); }
int FContainer::getCount() { return products.size(); }

// --------------------------------------------------------------------- TRANSPORTER

FTransporter::FTransporter() : speed(0.5) { ; }
void FTransporter::setPath(FPath* fpath) { this->fpath = fpath; }
void FTransporter::setTransportType(FTType type) { transport_type = type; }
void FTransporter::setSpeed(float s) { speed = s; }
float FTransporter::getSpeed() { return speed; }

void FTransporter::update(float dt) {
    vector<FNode*> nodes = fpath->get();

    bool changed = true; // TODO: chengeNow does not yet work!
    for (auto n : nodes) {
        if (n->getTransform()->changedNow()) {
            changed = true; break;
            cout << "detected change!\n";
        }
    }

    if (changed) fpath->update();

    vector<FNode*>::reverse_iterator itr;
    FNode *n1, *n2;
    FObject *o1, *o2;
    FProduct *p1, *p2;
    FContainer *c1, *c2;
    FNode::State s1, s2;
    FObject::Type t1, t2;
	s1 = FNode::FREE;

    n1=n2=0;
    o1=o2=0;
    p1=p2=0;
    c1=c2=0;

    //cout << "\n Transport id " << getID() << flush;
    int test_id = -1;

    // state machine
    for (itr = nodes.rbegin(); itr != nodes.rend(); itr++) {
        n2 = n1; n1 = *itr;
        s2 = s1; s1 = n1->getState();
        if (n2 == 0) continue; // ignore last node of path, this is where everything gets transported to

        o1 = n1->get();
        o2 = n2->get();
        o1 ? t1 = o1->getType() : t1 = FObject::NONE;
        o2 ? t2 = o2->getType() : t2 = FObject::NONE;
        (s1 == FNode::PRODUCT) ? p1 = (FProduct*)o1 : p1 = 0;
        (s2 == FNode::PRODUCT) ? p2 = (FProduct*)o2 : p2 = 0;
        (s1 == FNode::CONTAINER) ? c1 = (FContainer*)o1 : c1 = 0;
        (s2 == FNode::CONTAINER) ? c2 = (FContainer*)o2 : c2 = 0;

        if (o1 == 0) continue; /* nothing here to do */                                     if (getID() == test_id) cout << "\n Node content " << o1->getID() << " ,reserved?" << flush;
        if (s2 == FNode::RESERVED) continue; /* next node reserved*/                        if (getID() == test_id) cout << "\n Product there? " << flush;

        if (p2) continue; /* no place at next node */                                       if (getID() == test_id) cout << "\n Container there? " << flush;
        if (c2) if (c2->isFull()) continue; /* no place in container*/                      if (getID() == test_id) cout << "\n Next node has place for a product " << flush;

        switch(transport_type) {
            case PRODUCT:                                                                   if (getID() == test_id) cout << "\n Transport product, container? " << flush;
                if (t1 == FObject::CONTAINER) {                                             if (getID() == test_id) cout << "\n  yes, is container empty? " << flush;
                    if (c1->isEmpty()) continue; /* nothing to do, empty container */       if (getID() == test_id) cout << "\n  no, get object from container " << flush;
                    o1 = c1->pop();
                    c1 = 0;
                    p1 = (FProduct*)o1;
                    t1 = o1->getType();
                }

                if (t1 == FObject::PRODUCT) {                                               if (getID() == test_id) cout << "\n  a product, move it! " << flush;
                    if (n1->get() == o1) n1->set(0);
                    cargo[n2] = o1;
                    if(c2 == 0) n2->setState(FNode::RESERVED);
                    continue;
                }
                continue;

            case CONTAINER_FULL:                                                            if (getID() == test_id) cout << "\n Transport full container, container?" << flush;
                if (c1 == 0) continue;                                                      if (getID() == test_id) cout << "\n  found container, full? " << flush;
                if (!c1->isFull()) continue; /* wait until container is full*/              if (getID() == test_id) cout << "\n  yes, move it! " << flush;
                n1->set(0);
                n2->setState(FNode::RESERVED);
                cargo[n2] = c1;
                continue;

            case CONTAINER_EMPTY:                                                           if (getID() == test_id) cout << "\n Transport empty container, container?" << flush;
                if (c1 == 0) continue;                                                      if (getID() == test_id) cout << "\n  found container, empty? " << flush;
                if (!c1->isEmpty()) continue; /* wait until container is empty*/            if (getID() == test_id) cout << "\n  yes, move it! " << flush;
                n1->set(0);
                n2->setState(FNode::RESERVED);
                cargo[n2] = c1;
                continue;
        }
    }

    // objects in cargo are moved
    vector<FNode*> toErase;
    for (auto c : cargo) {
        FNode* n = c.first;
        FObject* o = c.second;
        if (n == 0) continue;
        FObject* no = n->get();

        OSG::path* p = fpath->getPath(n);
        float dx = speed*dt/p->getLength();
        if (o->move(p, dx)) {
            toErase.push_back(n);

            if (no == 0) { n->set(o); continue; } // no is not a container, just place the object there

            if (no->getType() == FObject::CONTAINER && o->getType() == FObject::PRODUCT) {
                FContainer* c = (FContainer*)no;
                FProduct* p = (FProduct*)o;
                c->add(p);
            }
        }
    }

    // delete cargo
    for (auto n : toErase) cargo.erase(n);
}


// --------------------------------------------------------------------- PRODUCT

FProduct::FProduct() { setType(PRODUCT); }
FProduct::~FProduct() { ; }


// --------------------------------------------------------------------- NETWORK

FNetwork::FNetwork() {;}
FNetwork::~FNetwork() {
    ;
}

FNode* FNetwork::addNode(FNode* parent) {
    FNode* n = new FNode();
    nodes[n->getID()] = n;
    if (parent) parent->connect(n);
    return n;
}

FNode* FNetwork::addNodeChain(int N, FNode* parent) {
    for (int i=0; i<N; i++) parent = addNode(parent);
    return parent;
}

vector<FNode*> FNetwork::getNodes() {
    vector<FNode*> res;
    for (auto n : nodes) res.push_back(n.second);
    return res;
}

VRStroke* FNetwork::stroke(Vec3f c, float k) {
    vector<path*> paths;
    for (itr = nodes.begin(); itr != nodes.end(); itr++) {
        FNode* n1 = itr->second;
        Vec3f t1 = n1->getTangent();
        Vec3f p1 = n1->getTransform()->getWorldPosition();
        map<int, FNode*>::iterator itr2;
        for (itr2 = n1->getOutgoing().begin(); itr2 != n1->getOutgoing().end(); itr2++) {
            FNode* n2 = itr2->second;
            Vec3f t2 = n2->getTangent();
            Vec3f p2 = n2->getTransform()->getWorldPosition();

            path* p = new path();
            p->addPoint(p1, t1, c);
            p->addPoint(p2, t2, c);
            p->compute(20);
            paths.push_back(p);
        }
    }


    VRStroke* stroke = new VRStroke("FNetwork_stroke");
    stroke->setPaths(paths);
    vector<Vec3f> profile;
    profile.push_back(Vec3f(-k,0,0));
    profile.push_back(Vec3f(-k*0.5,k,0));
    profile.push_back(Vec3f(k*0.5,k,0));
    profile.push_back(Vec3f(k,0,0));
    stroke->strokeProfile(profile, true);
    return stroke;
}


// --------------------------------------------------------------------- LOGISTICS


FLogistics::FLogistics() {}
FLogistics::~FLogistics() {
    for (t_ritr = transporter.rbegin(); t_ritr != transporter.rend(); t_ritr++) delete t_ritr->second;
    for (o_itr = objects.begin(); o_itr != objects.end(); o_itr++) delete o_itr->second;
    for (n_itr = networks.begin(); n_itr != networks.end(); n_itr++) delete n_itr->second;
    objects.clear();
    transporter.clear();
    networks.clear();
}

FProduct* FLogistics::addProduct(OSG::VRTransform* t) {
    FProduct* p = new FProduct();
    objects[p->getID()] = p;
    if (t) p->setTransformation(t);
    return p;
}

FNetwork* FLogistics::addNetwork() {
    FNetwork* n = new FNetwork();
    networks[n->getID()] = n;
    return n;
}

FTransporter* FLogistics::addTransporter(FTransporter::FTType type) {
    FTransporter* t = new FTransporter();
    t->setTransportType(type);
    transporter[t->getID()] = t;
    return t;
}

FPath* FLogistics::addPath() {
    FPath* p = new FPath();
    paths[p->getID()] = p;
    return p;
}

FContainer* FLogistics::addContainer(VRTransform* t) {
    if (t == 0) return 0;
    FContainer* c = new FContainer();
    t = (VRTransform*)t->duplicate(true);
    t->setVisible(true);
    t->setPersistency(0);
    c->setTransformation(t);
    objects[c->getID()] = c;
    return c;
}

void FLogistics::fillContainer(FContainer* c, int N, VRTransform* t) {
    for (int i=0; i<N; i++) {
        FProduct* p = addProduct();
        t = (VRTransform*)t->duplicate(true);
        t->setVisible(true);
        t->setPersistency(0);
        p->setTransformation(t);
        c->add( p );
    }
}

vector<FContainer*> FLogistics::getContainers() {
    vector<FContainer*> res;
    for (auto o : objects) if (o.second->getType() == FObject::CONTAINER) res.push_back((FContainer*)o.second);
    return res;
}

void FLogistics::update() {

    static float t2 = 0;
    float t1 = glutGet(GLUT_ELAPSED_TIME)/1000.0;//in seconds
    if (t2 == 0) { t2 = t1; return; } // first update
    float dt = t1 - t2;
    t2 = t1;

    for (t_ritr = transporter.rbegin(); t_ritr != transporter.rend(); t_ritr++) {
        t_ritr->second->update(dt);
    }
}
