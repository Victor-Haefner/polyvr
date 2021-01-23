#include "VRLogistics.h"
#include "core/objects/geometry/sprite/VRSprite.h"
#include "core/objects/VRTransform.h"
#include "core/math/path.h"
#include "core/utils/system/VRSystem.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/math/graph.h"
#include "addons/Algorithms/VRPathFinding.h"

#include <OpenSG/OSGMatrixUtility.h>

using namespace std;
using namespace OSG;

template<> string typeName(const FNode& m) { return "FNode"; }
template<> string typeName(const FObject& m) { return "FObject"; }
template<> string typeName(const FProduct& m) { return "FProduct"; }
template<> string typeName(const FContainer& m) { return "FContainer"; }
template<> string typeName(const FNetwork& m) { return "FNetwork"; }
template<> string typeName(const FPath& m) { return "FPath"; }
template<> string typeName(const FTransporter& m) { return "FTransporter"; }
template<> string typeName(const FLogistics& m) { return "FLogistics"; }


FID::FID() { static int i = 0; i++; ID = i; }
int FID::getID() { return ID; }
void FID::setID(int i) { ID = i; }


// --------------------------------------------------------------------- OBJECT

FObject::FObject() {}

FObject::~FObject() {
    ;
}

void FObject::setTransformation(VRTransformPtr t) {
    transform = t;
    t->set_orientation_mode(true);
    if (metaData) metaData->switchParent(t);
}

void FObject::setMetaData(string s) {
    return;
    if (metaData == 0) {
        metaData = OSG::VRSprite::create("meta");
        metaData->setMaterial(VRMaterial::create("metasprite"));
        metaData->switchParent(transform);
        metaData->setFrom(Vec3d(0,1,0));
        metaData->setDir(Vec3d(0,0,-1));
        metaData->setUp(Vec3d(0,1,0));
    }

    metaData->setText(s);
    float k = 0.2;
    metaData->setSize(0.5*k*s.size(), k);
}

VRTransformPtr FObject::getTransformation() {
    return transform;
}

void FObject::setType(FObject::Type t) { type = t; }
FObject::Type FObject::getType() { return type; }

bool FObject::move(OSG::PathPtr p, float dx) {
    VRTransformPtr trans = getTransformation();
    if (trans == 0) return true;

    bool done = false;
    t += dx;
    if (t >= 1) { t = 1; done = true; }

    Matrix4d m;
    Vec3d dir, up, pos;
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
FObjectPtr FNode::get() { return object; }

void FNode::set(FObjectPtr o) {
    object = o;
    if (o == 0) { setState(FREE); return; }
    if (o->getType() == FObject::CONTAINER) setState(CONTAINER);
    if (o->getType() == FObject::PRODUCT) setState(PRODUCT);

    auto t = o->getTransformation();
    auto t2 = getTransform();
    if (!t || !t2) return;

    Matrix4d wm;
    wm = t->getWorldMatrix();
    t->switchParent(t2);
    wm.setTranslate(t2->getWorldPosition());
    t->setWorldMatrix(wm);
    t->updateChange();
}

void FNode::setState(State s) { state = s; }
FNode::State FNode::getState() { return state; }

void FNode::setTransform(OSG::VRTransformPtr t) { transform = t; }
VRTransformPtr FNode::getTransform() { return transform; }


// --------------------------------------------------------------------- PATH

FPath::FPath() {}
void FPath::updatePath(GraphPtr graph) {
    path = Path::create();
    for (auto n : nodes) {
        auto p = graph->getPosition(n);
        path->addPoint(*p, Color3f());
    }
    path->compute(32);
}

int FPath::first() { return nodes[0]; }
int FPath::last() { return nodes[nodes.size()-1]; }

// --------------------------------------------------------------------- CONTAINER

FContainer::FContainer() : capacity(100) {
    setType(CONTAINER);
}
FContainer::~FContainer() { ; }

void FContainer::setCapacity(int i) { capacity = i; }
int FContainer::getCapacity() { return capacity; }

void FContainer::add(FProductPtr p) {
    auto t = p->getTransformation();
    t->hide();
    products.push_back(p);
    setMetaData("Nb: " + toString(products.size()));

    Matrix4d wm;
    wm = t->getWorldMatrix();
    t->switchParent(getTransformation());
    wm.setTranslate(getTransformation()->getWorldPosition());
    t->setWorldMatrix(wm);
    t->updateChange();
}

FProductPtr FContainer::pop() {
    FProductPtr p = products.back();
    //p->getTransformation()->setMatrix(getTransformation()->getMatrix());
    products.pop_back();
    p->getTransformation()->show();
    setMetaData("Nb: " + toString(products.size()));
    //p->setMetaData("ID: " + toString(p->getID()));
    return p;
}

FProductPtr FContainer::peek() {
    FProductPtr p = products.back();
    return p;
}

void FContainer::clear() { products.clear(); }

bool FContainer::isFull() { return ((int)products.size() == capacity); }
bool FContainer::isEmpty() { return ((int)products.size() == 0); }
int FContainer::getCount() { return products.size(); }

// --------------------------------------------------------------------- TRANSPORTER

FTransporter::FTransporter() : speed(0.5) {}
FTransporter::~FTransporter() {}
void FTransporter::setPath(FPathPtr fpath) { this->fpath = fpath; }
void FTransporter::setTransportType(FTType type) { transport_type = type; }
void FTransporter::setSpeed(float s) { speed = s; }
float FTransporter::getSpeed() { return speed; }

void FTransporter::update(float dt) {
    if (!fpath) return;
    auto net = network.lock();
    if (!net) return;
    vector<int>& nodes = fpath->nodes;
    if (!fpath->path) fpath->updatePath(net->getGraph());


    FNodePtr n1, n2;
    FObjectPtr o1, o2;
    FProductPtr p1, p2;
    FContainerPtr c1, c2;
    FNode::State s1, s2;
    FObject::Type t1, t2;
	s1 = FNode::FREE;

    //cout << "\n Transport id " << getID() << flush;
    //int test_id = -1;

    // state machine
    for (size_t i=0; i<nodes.size(); i++) {
        n2 = n1;
        int nID = nodes[nodes.size()-1-i];
        n1 = net->getNode(nID);
        s2 = s1; s1 = n1->getState();
        if (n2 == 0) continue; // ignore last node of path, this is where everything gets transported to

        o1 = n1->get();
        o2 = n2->get();
        o1 ? t1 = o1->getType() : t1 = FObject::NONE;
        o2 ? t2 = o2->getType() : t2 = FObject::NONE;
        (s1 == FNode::PRODUCT) ? p1 = dynamic_pointer_cast<FProduct>(o1) : p1 = 0;
        (s2 == FNode::PRODUCT) ? p2 = dynamic_pointer_cast<FProduct>(o2) : p2 = 0;
        (s1 == FNode::CONTAINER) ? c1 = dynamic_pointer_cast<FContainer>(o1) : c1 = 0;
        (s2 == FNode::CONTAINER) ? c2 = dynamic_pointer_cast<FContainer>(o2) : c2 = 0;

        if (o1 == 0) continue; /* nothing here to do */                                     //if (getID() == test_id) cout << "\n Node content " << o1->getID() << " ,reserved?" << flush;
        if (s2 == FNode::RESERVED) continue; /* next node reserved*/                        //if (getID() == test_id) cout << "\n Product there? " << flush;

        if (p2) continue; /* no place at next node */                                       //if (getID() == test_id) cout << "\n Container there? " << flush;
        if (c2) if (c2->isFull()) continue; /* no place in container*/                      //if (getID() == test_id) cout << "\n Next node has place for a product " << flush;

        switch(transport_type) {
            case PRODUCT:                                                                   //if (getID() == test_id) cout << "\n Transport product, container? " << flush;
                if (t1 == FObject::CONTAINER) {                                             //if (getID() == test_id) cout << "\n  yes, is container empty? " << flush;
                    if (c1->isEmpty()) continue; /* nothing to do, empty container */       //if (getID() == test_id) cout << "\n  no, get object from container " << flush;
                    o1 = c1->pop();
                    c1 = 0;
                    p1 = dynamic_pointer_cast<FProduct>(o1);
                    t1 = o1->getType();
                }

                if (t1 == FObject::PRODUCT) {                                               //if (getID() == test_id) cout << "\n  a product, move it! " << flush;
                    if (n1->get() == o1) n1->set(0);
                    cargo[n2] = o1;
                    if(c2 == 0) n2->setState(FNode::RESERVED);
                    continue;
                }
                continue;

            case CONTAINER_FULL:                                                            //if (getID() == test_id) cout << "\n Transport full container, container?" << flush;
                if (c1 == 0) continue;                                                      //if (getID() == test_id) cout << "\n  found container, full? " << flush;
                if (!c1->isFull()) continue; /* wait until container is full*/              //if (getID() == test_id) cout << "\n  yes, move it! " << flush;
                n1->set(0);
                n2->setState(FNode::RESERVED);
                cargo[n2] = c1;
                continue;

            case CONTAINER_EMPTY:                                                           //if (getID() == test_id) cout << "\n Transport empty container, container?" << flush;
                if (c1 == 0) continue;                                                      //if (getID() == test_id) cout << "\n  found container, empty? " << flush;
                if (!c1->isEmpty()) continue; /* wait until container is empty*/            //if (getID() == test_id) cout << "\n  yes, move it! " << flush;
                n1->set(0);
                n2->setState(FNode::RESERVED);
                cargo[n2] = c1;
                continue;
        }
    }

    // objects in cargo are moved
    vector<FNodePtr> toErase;
    for (auto c : cargo) {
        FNodePtr n = c.first;
        FObjectPtr o = c.second;
        if (n == 0) continue;
        FObjectPtr no = n->get();

        auto p = fpath->path;
        float dx = speed*dt/p->getLength();
        if (o->move(p, dx)) {
            toErase.push_back(n);

            if (no == 0) { n->set(o); continue; } // no is not a container, just place the object there

            if (no->getType() == FObject::CONTAINER && o->getType() == FObject::PRODUCT) {
                auto c = dynamic_pointer_cast<FContainer>(no);
                auto p = dynamic_pointer_cast<FProduct>(o);
                c->add(p);
            }
        }
    }

    // delete cargo
    for (auto n : toErase) cargo.erase(n);
}


// --------------------------------------------------------------------- PRODUCT

FProduct::FProduct() { setType(PRODUCT); }
FProduct::~FProduct() {}


// --------------------------------------------------------------------- NETWORK

FNetwork::FNetwork() {
    graph = Graph::create();
}

FNetwork::~FNetwork() {}

GraphPtr FNetwork::getGraph() { return graph; }
FNodePtr FNetwork::getNode(int ID) { return nodes[ID]; }

int FNetwork::addNode(PosePtr p) {
    auto nID = graph->addNode(p);
    nodes[nID] = FNodePtr(new FNode());
    nodes[nID]->setID(nID);
    return nID;
}

void FNetwork::connect(int n1, int n2) {
    graph->connect(n1, n2);
}

vector<FNodePtr> FNetwork::getNodes() {
    vector<FNodePtr> res;
    for (auto n : nodes) res.push_back(n.second);
    return res;
}

VRStrokePtr FNetwork::stroke(Color3f c, float k) {
    vector<PathPtr> paths;
    for (auto& e : graph->getEdges()) {
        auto& edge = e.second;
        PosePtr p1 = graph->getPosition(edge.from);
        PosePtr p2 = graph->getPosition(edge.to);
        PathPtr p = Path::create();
        p->addPoint( *p1, c );
        p->addPoint( *p2, c );
        p->compute(8);
        paths.push_back(p);
    }

    VRStrokePtr stroke = VRStroke::create("FNetwork_stroke");
    stroke->setPaths(paths);
    vector<Vec3d> profile;
    profile.push_back(Vec3d(-k,0,0));
    profile.push_back(Vec3d(-k*0.5,k,0));
    profile.push_back(Vec3d(k*0.5,k,0));
    profile.push_back(Vec3d(k,0,0));
    stroke->strokeProfile(profile, true, true);
    return stroke;
}


// --------------------------------------------------------------------- LOGISTICS


FLogistics::FLogistics() {
    network = FNetworkPtr(new FNetwork());
}

FLogistics::~FLogistics() {
    objects.clear();
    transporter.clear();
}

FProductPtr FLogistics::addProduct(OSG::VRTransformPtr t) {
    auto p = FProductPtr(new FProduct());
    objects[p->getID()] = p;
    if (t) p->setTransformation(t);
    return p;
}

FNetworkPtr FLogistics::addNetwork() {
    return network;
}

FTransporterPtr FLogistics::addTransporter(string stype) {
    FTransporter::FTType type = FTransporter::PRODUCT;
    if (stype == "CONTAINER_FULL") type = FTransporter::CONTAINER_FULL;
    if (stype == "CONTAINER_EMPTY") type = FTransporter::CONTAINER_EMPTY;
    auto t = FTransporterPtr(new FTransporter());
    t->network = network;
    t->setTransportType(type);
    transporter[t->getID()] = t;
    return t;
}

FContainerPtr FLogistics::addContainer(VRTransformPtr t) {
    if (t == 0) return 0;
    auto c = FContainerPtr(new FContainer());
    t = static_pointer_cast<VRTransform>(t->duplicate(true));
    t->setVisible(true);
    t->setPersistency(0);
    c->setTransformation(t);
    objects[c->getID()] = c;
    return c;
}

void FLogistics::fillContainer(FContainerPtr c, int N, VRTransformPtr t) {
    for (int i=0; i<N; i++) {
        auto p = addProduct();
        t = static_pointer_cast<VRTransform>(t->duplicate(true));
        t->setVisible(true);
        t->setPersistency(0);
        p->setTransformation(t);
        c->add( p );
    }
}

vector<FContainerPtr> FLogistics::getContainers() {
    vector<FContainerPtr> res;
    for (auto o : objects) if (o.second->getType() == FObject::CONTAINER) res.push_back(dynamic_pointer_cast<FContainer>(o.second));
    return res;
}

shared_ptr<FLogistics> FLogistics::create() { return shared_ptr<FLogistics>(new FLogistics()); }

void FLogistics::update() {
    static float t2 = 0;
    float t1 = getTime()*1e-6;//in seconds
    if (t2 == 0) { t2 = t1; return; } // first update
    float dt = t1 - t2;
    t2 = t1;

    for (auto& t : transporter) t.second->update(dt);
}

FPathPtr FLogistics::computeRoute(int n1, int n2) {
    auto nav = VRPathFinding::create();
    nav->setGraph(network->getGraph());
    auto route = nav->computePath(n1, n2);
    auto path = FPathPtr( new FPath() );
    for (auto& n : route) path->nodes.push_back(n.nID);
    return path;
}

void FLogistics::clear() { // TODO
    ;
}
