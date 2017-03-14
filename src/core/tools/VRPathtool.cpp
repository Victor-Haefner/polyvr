#include "VRPathtool.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/setup/devices/VRDevice.h"
#include "core/math/path.h"
#include "core/utils/VRStorage_template.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeoFunctions.h>

using namespace OSG;

VRManipulator::VRManipulator() { setup(); }

void VRManipulator::handle(VRGeometryPtr g) {
    if (sel == 0) return;
    auto c = sel->getConstraint();
    c->setActive(true, sel);

    if (g == gTX || g == gTY || g == gTZ || g == gRX || g == gRY || g == gRZ) { // lock everything
        c->setTConstraint(Vec3f(0,0,0), VRConstraint::LINE);
        c->setRConstraint(Vec3f(0,0,0), VRConstraint::POINT);
    }

    if (g == gTX) c->setTConstraint(Vec3f(1,0,0), VRConstraint::LINE);
    if (g == gTY) c->setTConstraint(Vec3f(0,1,0), VRConstraint::LINE);
    if (g == gTZ) c->setTConstraint(Vec3f(0,0,1), VRConstraint::LINE);
    //if (g == gRX) sel->setRConstraint(Vec3i(0,1,1)); // TODO: fix rotation constraints!
    //if (g == gRY) sel->setRConstraint(Vec3i(1,0,1));
    //if (g == gRZ) sel->setRConstraint(Vec3i(1,1,0));
    if (g == gRX || g == gRY || g == gRZ) c->setRConstraint(Vec3f(0,0,0), VRConstraint::POINT);
}

void VRManipulator::manipulate(VRTransformPtr t) {
    t->addChild(anchor);
    sel = t;
}

void VRManipulator::setup() {
    anchor = VRObject::create("manipulator");
    gTX = VRGeometry::create("gTX");
    gTY = VRGeometry::create("gTY");
    gTZ = VRGeometry::create("gTZ");
    gRX = VRGeometry::create("gRX");
    gRY = VRGeometry::create("gRY");
    gRZ = VRGeometry::create("gRZ");
    gTX->setPrimitive("Box", "0.1 0.02 0.02 1 1 1");
    gTY->setPrimitive("Box", "0.02 0.1 0.02 1 1 1");
    gTZ->setPrimitive("Box", "0.02 0.02 0.1 1 1 1");
    gRX->setPrimitive("Torus", "0.01 0.07 4 16");
    gRY->setPrimitive("Torus", "0.01 0.07 4 16");
    gRZ->setPrimitive("Torus", "0.01 0.07 4 16");
    gRX->setDir(Vec3f(1,0,0));
    gRY->setDir(Vec3f(0,1,0));
    gRY->setUp(Vec3f(1,0,0));
    anchor->addChild(gTX);
    anchor->addChild(gTY);
    anchor->addChild(gTZ);
    anchor->addChild(gRX);
    anchor->addChild(gRY);
    anchor->addChild(gRZ);
}


VRPathtool::VRPathtool() : VRObject("Pathtool") {
    updatePtr = VRFunction<int>::create("path tool update", boost::bind(&VRPathtool::updateDevs, this) );
    VRScene::getCurrent()->addUpdateFkt(updatePtr, 100);

    manip = new VRManipulator();

    lmat = VRMaterial::create("pline");
    lmat->setLit(false);
    lmat->setDiffuse(Vec3f(0.1,0.9,0.2));
    lmat->setLineWidth(3);

    lsmat = VRMaterial::create("spline");
    lsmat->setLit(false);
    lsmat->setDiffuse(Vec3f(0.9,0.1,0.2));
    lsmat->setLineWidth(3);

    storeObj("handle", customHandle);
    storeObj("graph", graph);
    regStorageSetupFkt( VRFunction<int>::create("pathtool setup", boost::bind(&VRPathtool::setup, this)) );
}

VRPathtoolPtr VRPathtool::create() { return VRPathtoolPtr( new VRPathtool() ); }

void VRPathtool::setup() {
    setGraph(graph);
}

void VRPathtool::setProjectionGeometry(VRObjectPtr obj) { projObj = obj; }

void VRPathtool::setGraph(GraphPtr g) {
    if (!g) return;
    if (g == graph) graph = 0; // important, else the clear will also clear the graph!
    clear();
    graph = g;

    for (uint i=0; i<g->size(); i++) setGraphNode(i);
    for (auto& n : g->getEdges()) {
        for (auto& e : n) {
            setGraphEdge(e);
        }
    }
}

int VRPathtool::addNode(posePtr p) {
    if (!graph) setGraph( Graph::create() );
    int i = graph->addNode();
    graph->setPosition(i, p->pos());
    auto h = setGraphNode(i);
    h->setPose(p);
    return i;
}

void VRPathtool::remNode(int i) {
    if (!graph) return;
    graph->remNode(i);
    if (knots.count(i)) {
        if (auto h = knots[i].handle.lock()) {
            h->destroy();
            handleToNode.erase(h.get());
        }
        knots.erase(i);
    }
}

int VRPathtool::getNodeID(VRObjectPtr o) {
    auto k = (VRGeometry*)o.get();
    if (!handleToNode.count(k)) return -1;
    return handleToNode[k];
}

template <class T, class V>
void vecRemVal(V& vec, T& val) {
    vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
};

void VRPathtool::disconnect(int i1, int i2) {
    if (i1 == i2) return;
    if (!graph) return;
    graph->disconnect(i1,i2);

    auto h1 = knots[i1].handle.lock().get();
    auto h2 = knots[i2].handle.lock().get();
    entryPtr e = 0;
    for (auto e1 : entries[h1] ) {
        for (auto e2 : entries[h2] ) {
            if (e1 == e2) { // found entry between h1 and h2!
                e = e1;
                break;
            }
        }
        if (e) break;
    }

    if (e) {
        if (auto l = e->line.lock()) l->destroy(); // TODO: entry destructor?
        vecRemVal(entries[h1], e);
        vecRemVal(entries[h2], e);
        paths.erase(e->p.get());
    }

    vecRemVal(knots[i1].out, i2);
    vecRemVal(knots[i2].in, i1);
}

void VRPathtool::connect(int i1, int i2) {
    if (i1 == i2) return;
    if (!graph) return;
    if (!graph->connected(i1,i2)) {
        auto& e = graph->connect(i1,i2);
        setGraphEdge(e);
    } else disconnect(i1,i2);
}

void VRPathtool::setGraphEdge(Graph::edge& e) {
    auto& nodes = graph->getNodes();
    pathPtr p = path::create();
    p->addPoint(nodes[e.from].box.center());
    p->addPoint(nodes[e.to].box.center());
    addPath(p, 0, knots[e.from].handle.lock(), knots[e.to].handle.lock());
    knots[e.from].out.push_back(e.to);
    knots[e.to].in.push_back(e.from);
    updateHandle( knots[e.from].handle.lock() );
    updateHandle( knots[e.to  ].handle.lock() );
}

VRGeometryPtr VRPathtool::setGraphNode(int i) {
    auto h = newHandle();
    knots[i] = knot();
    knots[i].handle = h;
    handleToNode[h.get()] = i;
    addChild(h);
    h->setFrom( graph->getPosition(i) );
    return h;
}

void VRPathtool::update() {
    if (graph) { // smooth knot transformations
        map<int, Vec3f> hPositions; // get handle positions
        for (uint i=0; i<knots.size(); i++)
            if (auto h = knots[i].handle.lock()) hPositions[i] = h->getWorldPosition();

        for (auto& knot : knots) { // compute and set direction of handles
            auto h = knot.second.handle.lock();
            if (!h) continue;

            Vec3f pos = h->getWorldPosition();
            Vec3f dir;
            for (auto k : knot.second.out) if (hPositions.count(k)) dir += pos - hPositions[k];
            for (auto k : knot.second.in) if (hPositions.count(k)) dir += hPositions[k] - pos;
            dir.normalize();
            h->setDir(dir);
        }
    }

    for (auto wh : handles) { // apply handle transformation to path points
        auto handle = wh.lock();
        if (!handle) continue;
        auto key = handle.get();
        if (!entries.count(key)) continue;
        for (auto e : entries[key]) {
            Matrix m = handle->getMatrixTo(e->anchor.lock());
            e->p->setPoint(e->points[key], Vec3f(m[3]), Vec3f(m[2]), Vec3f(1,1,1), Vec3f(m[1]));
        }
    }

    for (auto e : paths) updateEntry(e.second);
}

void VRPathtool::updateEntry(entryPtr e) { // update path representation
    if (!e) return;
    int hN = e->handles.size();
    if (hN <= 0) return;
    e->p->compute(e->resolution);
    int pN = e->p->getPositions().size();
    if (pN <= 2) return;

    if (!e->anchor.lock()) e->anchor = ptr();
    if (!e->line.lock() && e->anchor.lock()) { // update path line
        auto line = VRStroke::create("path");
        e->line = line;
        line->setPersistency(0);
        line->setMaterial(lmat);
        e->anchor.lock()->addChild(line);
        vector<Vec3f> profile;
        profile.push_back(Vec3f());
        line->addPath(e->p);
        line->strokeProfile(profile, 0, 0);
    }

    if (auto l = e->line.lock()) l->update();
}

void VRPathtool::updateHandle(VRGeometryPtr handle) { // update paths the handle belongs to
    if (!handle) return;
    auto key = handle.get();
    if (!entries.count(key)) return;
    Matrix m = handle->getWorldMatrix();
    for (auto e : entries[key]) {
        e->p->setPoint(e->points[key], Vec3f(m[3]), Vec3f(m[2]), Vec3f(1,1,1), Vec3f(m[1]));
        updateEntry(e);
    }
}

void VRPathtool::projectHandle(VRGeometryPtr handle, VRDevicePtr dev) {
    if (!projObj) return;
    OSG::VRIntersection ins = dev->intersect(projObj);
    if (ins.hit) {
        Vec3f d = handle->getWorldDirection();
        d -= d.dot(ins.normal)*ins.normal;
        handle->setWorldPose( pose::create(Vec3f(ins.point), -d, ins.normal) );
    }
}

void VRPathtool::updateDevs() { // update when something is dragged
    for (auto dev : VRSetup::getCurrent()->getDevices()) { // get dragged objects
        VRGeometryPtr obj = static_pointer_cast<VRGeometry>(dev.second->getDraggedObject());
        if (obj == 0) continue;
        if (entries.count(obj.get()) == 0) continue;
        projectHandle(obj, dev.second);
        updateHandle(obj);
    }
}

void VRPathtool::addPath(pathPtr p, VRObjectPtr anchor, VRGeometryPtr ha, VRGeometryPtr he) {
    auto e = entryPtr( new entry( anchor ? anchor : ptr() ) );
    e->p = p;
    paths[e->p.get()] = e;

    for (int i=0; i<p->size(); i++) {
        auto point = p->getPoint(i);
        VRGeometryPtr h;
        if (i == 0) h = ha;
        if (i == p->size()-1) h = he;
        if (!h) h = newHandle();

        entries[h.get()].push_back(e);
        h->setPose(point.p, point.n, point.u);
        e->addHandle(h);
    }
}

VRPathtool::entry::entry(VRObjectPtr a) {
    anchor = a;
}

void VRPathtool::entry::addHandle(VRGeometryPtr h) {
    points[h.get()] = points.size()-1;
    handles.push_back(h);
    if (auto a = anchor.lock()) a->addChild(h);
}

pathPtr VRPathtool::newPath(VRDevicePtr dev, VRObjectPtr anchor, int resolution) {
    auto e = entryPtr( new entry( anchor ? anchor : ptr() ) );
    e->p = path::create();
    e->resolution = resolution;
    paths[e->p.get()] = e;

    extrude(0,e->p);
    extrude(dev,e->p);
    return e->p;
}

VRGeometryPtr VRPathtool::newControlHandle(VRGeometryPtr handle) {
    VRGeometryPtr h;
    h = VRGeometry::create("handle");
    h->setPrimitive("Sphere", "0.04 2");

    h->setPickable(true);
    h->addAttachment("controlhandle", 0);
    h->setMaterial(VRMaterial::get("pathHandle"));
    h->getMaterial()->setDiffuse(Vec3f(0.5,0.5,0.9));
    //calcFaceNormals(h->getMesh()); // not working ??
    h->setPersistency(0);
    controlhandles.push_back(h);
    handle->addChild(h);
    return h;
}

VRGeometryPtr VRPathtool::newHandle() {
    VRGeometryPtr h;

    if (customHandle) {
        h = static_pointer_cast<VRGeometry>( customHandle->duplicate() );
    } else {
        h = VRGeometry::create("handle");
        h->setPrimitive("Torus", "0.04 0.15 3 4");
    }

    h->setPickable(true);
    h->addAttachment("handle", 0);
    h->setMaterial(VRMaterial::get("pathHandle"));
    h->getMaterial()->setDiffuse(Vec3f(0.5,0.5,0.9));
    //calcFaceNormals(h->getMesh()); // not working ??
    h->setPersistency(0);
    handles.push_back(h);
    return h;
}

VRGeometryPtr VRPathtool::getHandle(int ID) {
    return knots[ID].handle.lock();
}

vector<pathPtr> VRPathtool::getPaths(VRGeometryPtr h) {
    vector<pathPtr> res;
    if (!h) for (auto p : paths) res.push_back(p.second->p);
    else if (entries.count(h.get())) {
        auto& ev = entries[h.get()];
        for (auto e : ev) res.push_back(e->p);
    }
    return res;
}

pathPtr VRPathtool::getPath(VRGeometryPtr h1, VRGeometryPtr h2) {
    if (!entries.count(h1.get()) || !entries.count(h2.get())) return 0;
    for (auto& e1 : entries[h1.get()]) {
        for (auto& e2 : entries[h2.get()]) {
            if (e1->p == e2->p) return e1->p;
        }
    }
    return 0;
}

vector<VRGeometryPtr> VRPathtool::getHandles(pathPtr p) {
    vector<VRGeometryPtr> res;
    if (p == 0) for (auto h : handles) res.push_back(h.lock());
    else {
        map<int, VRGeometryPtr> sor;
        entryPtr e = paths[p.get()];
        for (auto h : e->handles) if (auto hl = h.lock()) sor[ e->points[hl.get()] ] = hl;
        for (auto h : sor) res.push_back(h.second);
    }
    return res;
}

VRStrokePtr VRPathtool::getStroke(pathPtr p) {
    if (paths.count(p.get()) == 0) return 0;
    return paths[p.get()]->line.lock();
}

VRGeometryPtr VRPathtool::extrude(VRDevicePtr dev, pathPtr p) {
    if (paths.count(p.get()) == 0) {
        cout << "Warning: VRPathtool::extrude, path " << p << " unknown\n";
        return 0;
    }

    auto e = paths[p.get()];
    e->p->addPoint(Vec3f(0,0,-1), Vec3f(1,0,0), Vec3f(1,1,1));

    VRGeometryPtr h = newHandle();
    entries[h.get()].push_back(e);
    handles.push_back(h);
    e->points[h.get()] = e->points.size()-1;
    e->handles.push_back(h);
    e->anchor.lock()->addChild(h);
    if (dev) {
        dev->drag(h, dev->getBeacon());
        h->setPose(Vec3f(0,0,-1), Vec3f(0,0,-1), Vec3f(0,1,0));
    }

    updateHandle(h);
    return h;
}

void VRPathtool::setHandleGeometry(VRGeometryPtr geo) {
    customHandle = static_pointer_cast<VRGeometry>( geo->duplicate() );
}

void VRPathtool::clear(pathPtr p) {
    if (!p) {
        paths.clear();
        entries.clear();
        handles.clear();
        handleToNode.clear();
        if (graph) graph->clear();
        knots.clear();
        selectedPath.reset();
        clearChildren();
        return;
    }

    if (paths.count(p.get()) == 0) return;
    auto e = paths[p.get()];

    for (auto h : e->handles) {
        if (auto hl = h.lock()) {
            entries.erase(hl.get());
            hl->destroy();
        }
    }

    e->handles.clear();
    if (e->line.lock()) e->line.lock()->destroy();
    e->line.reset();
    p->clear();
}

void VRPathtool::remPath(pathPtr p) {
    if (paths.count(p.get()) == 0) return;
    auto e = paths[p.get()];

    for (auto hw : e->handles) {
        if (auto h = hw.lock()) {
            entries.erase(h.get());
            h->destroy();
        }
    }

    if (auto l = e->line.lock()) l->destroy();
    paths.erase(p.get());
}

void VRPathtool::setVisible(bool hvis, bool lines) {
    for (auto p : paths) if (p.second->line.lock()) p.second->line.lock()->setVisible(lines);
    for (auto h : handles) {
        if (auto hs = h.lock()) hs->setVisible(hvis);
    }
}

void VRPathtool::select(VRGeometryPtr h) {
    manip->handle(h);
    if (entries.count(h.get()) == 0) return;
    manip->manipulate(h);
}

void VRPathtool::select(pathPtr p) {
    if (selectedPath) getStroke(selectedPath)->setMaterial(lmat);
    selectedPath = p;
    getStroke(selectedPath)->setMaterial(lsmat);
}

void VRPathtool::deselect() {
    if (selectedPath) getStroke(selectedPath)->setMaterial(lmat);
}

VRMaterialPtr VRPathtool::getPathMaterial() { return lmat; }
