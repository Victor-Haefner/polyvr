#include "VRPathtool.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/setup/devices/VRDevice.h"
#include "core/math/path.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeoFunctions.h>

using namespace OSG;

string toString(VRPathtool::option& n) {
    stringstream ss;
    ss << n.resolution << " " << n.useControlHandles;
    return ss.str();
}

template<> bool toValue(stringstream& ss, VRPathtool::option& n) {
    bool b = true;
    b = bool(ss >> n.resolution);
    b = bool(ss >> n.useControlHandles);
    return b;
}

#include "core/utils/VRStorage_template.h"

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

VRPathtool::option::option(int r, bool uch) : resolution(r), useControlHandles(uch) {}

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
    storeMap("paths", &paths, true);
    storeMap("options", options);
    //regStorageSetupBeforeFkt( VRFunction<int>::create("pathtool clear", boost::bind(&VRPathtool::clear, this)) );
    regStorageSetupBeforeFkt( VRFunction<int>::create("pathtool clear", boost::bind(&VRPathtool::setupBefore, this)) );
    regStorageSetupFkt( VRFunction<int>::create("pathtool setup", boost::bind(&VRPathtool::setup, this)) );
}

VRPathtoolPtr VRPathtool::create() { return VRPathtoolPtr( new VRPathtool() ); }

void VRPathtool::setupBefore() {
    clear();
}

void VRPathtool::setup() {
    setGraph(graph, false);
}

void VRPathtool::setProjectionGeometry(VRObjectPtr obj) { projObj = obj; }

void VRPathtool::setGraph(GraphPtr g, bool doClear) {
    if (!g) return;
    if (g == graph) graph = 0; // important, else the clear will also clear the graph!
    if (doClear) clear();
    graph = g;

    for (int i=0; i<g->size(); i++) setGraphNode(i);
    for (auto& n : g->getEdges()) {
        for (auto& e : n) {
            setGraphEdge(e);
        }
    }
}

int VRPathtool::addNode(posePtr p) {
    auto tmp = VRTransform::create("tmp");
    tmp->setPose(p);
    p = tmp->getRelativePose(ptr());

    if (!graph) setGraph( Graph::create() );
    int i = graph->addNode();
    graph->setPosition(i, p);
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
    for (auto e1 : handleToEntries[h1] ) {
        for (auto e2 : handleToEntries[h2] ) {
            if (e1 == e2) { // found entry between h1 and h2!
                e = e1;
                break;
            }
        }
        if (e) break;
    }

    if (e) {
        if (auto l = e->line.lock()) l->destroy(); // TODO: entry destructor?
        vecRemVal(handleToEntries[h1], e);
        vecRemVal(handleToEntries[h2], e);
        pathToEntry.erase(e->p.get());
    }

    vecRemVal(knots[i1].out, i2);
    vecRemVal(knots[i2].in, i1);
}

void VRPathtool::connect(int i1, int i2, bool handles) {
    if (i1 == i2) return;
    auto& nodes = graph->getNodes();
    Vec3f d = nodes[i2].p.pos() - nodes[i1].p.pos();
    d.normalize();
    connect(i1,i2,handles,d,d);
}

void VRPathtool::connect(int i1, int i2, bool handles, Vec3f n1, Vec3f n2) {
    if (i1 == i2) return;
    if (!graph) return;
    if (!graph->connected(i1,i2)) {
        int eID = graph->connect(i1,i2);
        auto& e = graph->getEdge(eID);
        setGraphEdge(e, handles, n1, n2);
    } else disconnect(i1,i2);
}

void VRPathtool::setGraphEdge(Graph::edge& e, bool handles) {
    auto& nodes = graph->getNodes();
    setGraphEdge(e, handles, nodes[e.from].p.dir(), nodes[e.to].p.dir());
}

void VRPathtool::setGraphEdge(Graph::edge& e, bool handles, Vec3f n1, Vec3f n2) {
    auto& nodes = graph->getNodes();
    if (!paths.count(e.ID)) {
        paths[e.ID] = path::create();
        paths[e.ID]->addPoint( pose(nodes[e.from].p.pos(), n1));
        paths[e.ID]->addPoint( pose(nodes[e.to].p.pos(), n2));
    }
    auto en = newEntry( paths[e.ID], option(10, handles), e.ID, 0 );
    setupHandles(en, knots[e.from].handle.lock(), knots[e.to].handle.lock());
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
    //h->setRelativePose( graph->getPosition(i), ptr() );
    h->setWorldPose( graph->getPosition(i) );
    return h;
}

void VRPathtool::update() { // call in script to have smooth knots
    if (graph) { // smooth knot transformations
        map<int, Vec3f> hPositions; // get handle positions
        for (uint i=0; i<knots.size(); i++)
            //if (auto h = knots[i].handle.lock()) hPositions[i] = h->getRelativePosition(ptr());
            if (auto h = knots[i].handle.lock()) hPositions[i] = h->getWorldPosition();

        for (auto& knot : knots) { // compute and set direction of handles
            auto h = knot.second.handle.lock();
            if (!h) continue;

            Vec3f pos = h->getRelativePosition(ptr());
            Vec3f dir;
            for (auto k : knot.second.out) if (hPositions.count(k)) dir += pos - hPositions[k];
            for (auto k : knot.second.in) if (hPositions.count(k)) dir += hPositions[k] - pos;
            dir.normalize();
            h->setRelativeDir(dir, ptr());
        }
    }

    for (auto wh : handles) { // apply handle transformation to path points
        auto handle = wh.lock();
        if (!handle) continue;
        auto key = handle.get();
        if (!handleToEntries.count(key)) continue;
        for (auto e : handleToEntries[key]) {
            auto po = handle->getPoseTo(e->anchor.lock());
            e->p->setPoint(e->points[key], *po);
        }
    }

    for (auto e : pathToEntry) updateEntry(e.second);
}

void VRPathtool::updateEntry(entryPtr e) { // update path representation
    if (!e) return;
    int hN = e->handles.size();
    if (hN <= 0) return;
    auto opt = options[e->edge];
    e->p->compute(opt.resolution);
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

    if (auto l = e->line.lock()) {
        l->update();
    }
}

void VRPathtool::updateHandle(VRGeometryPtr handle) { // update paths the handle belongs to
    if (!handle) return;
    auto p = handle->getPoseTo(ptr());
    auto key = handle.get();
    if (!handleToEntries.count(key)) return;

    if (handle->hasAttachment("handle")) {
        if (graph && handleToNode.count(key)) graph->setPosition( handleToNode[key], p );

        for (auto e : handleToEntries[key]) {
            auto op = e->p->getPoint(e->points[key]);
            e->p->setPoint( e->points[key], pose(p->pos(), op.dir(), p->up()));
            updateEntry(e);
        }

        for (auto c : handle->getChildrenWithAttachment("controlhandle")) {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(c);
            if (g) updateHandle(g);
        }

        if (handle->getChildrenWithAttachment("controlhandle").size() == 0) { // no control handles -> smooth knots curve
            map<int, Vec3f> hPositions; // get handle positions
            auto getPos = [&](int ID) {
                if (!hPositions.count(ID)) {
                    auto h = knots[ID].handle.lock();
                    hPositions[ID] = h ? h->getRelativePosition(ptr()) : Vec3f();
                }
                return hPositions[ID];
            };

            auto updateHandleDir = [&](int ID) {
                Vec3f pos = getPos(ID);
                Vec3f dir;
                for (auto k : knots[ID].in ) dir += pos - getPos(k);
                for (auto k : knots[ID].out) dir += getPos(k) - pos;
                dir.normalize();
                auto h = knots[ID].handle.lock();
                auto key = h.get();
                if (h) {
                    h->setRelativeDir(dir, ptr());
                    for (auto e : handleToEntries[key]) {
                        auto op = e->p->getPoint(e->points[key]);
                        e->p->setPoint( e->points[key], pose(op.pos(), dir, op.up()));
                        updateEntry(e);
                    }
                }
            };

            int nID = handleToNode[key];
            updateHandleDir( nID );
            for (auto k : knots[nID].out) updateHandleDir(k);
            for (auto k : knots[nID].in ) updateHandleDir(k);
        }
    }

    if (handle->hasAttachment("controlhandle")) {
        VRGeometryPtr pg = dynamic_pointer_cast<VRGeometry>(handle->getDragParent());
        if (!pg) pg = dynamic_pointer_cast<VRGeometry>(handle->getParent());
        if (!pg) return; // failed to get parent handle

        //Vec3f d = p->pos() - pg->getRelativePosition(ptr());
        Vec3f d = p->pos() - pg->getWorldPosition();
        auto e = handleToEntries[key][0]; // should only be one entry at most!
        auto op = e->p->getPoint(e->points[key]);
        e->p->setPoint( e->points[key], pose(op.pos(), d, op.up()));
        updateEntry(e);
    }
}

void VRPathtool::projectHandle(VRGeometryPtr handle, VRDevicePtr dev) { // project handle on projObj if set
    if (!projObj) return;
    OSG::VRIntersection ins = dev->intersect(projObj);
    if (ins.hit) {
        Vec3f d = handle->getWorldDirection();
        //Vec3f d = handle->getRelativeDirection(ptr());
        d -= d.dot(ins.normal)*ins.normal;
        //handle->setRelativePose( pose::create(Vec3f(ins.point), d, ins.normal), ptr() );
        handle->setWorldPose( pose::create(Vec3f(ins.point), d, ins.normal) );
    }
}

void VRPathtool::updateDevs() { // update when something is dragged
    for (auto dev : VRSetup::getCurrent()->getDevices()) { // get dragged objects
        VRGeometryPtr obj = static_pointer_cast<VRGeometry>(dev.second->getDraggedObject());
        if (obj == 0) continue;
        if (!obj->hasAttachment("handle") && !obj->hasAttachment("controlhandle")) continue;
        if (!handleToNode.count(obj.get()) && !handleToEntries.count(obj.get())) continue;
        projectHandle(obj, dev.second);
        updateHandle(obj);
    }
}

VRPathtool::entryPtr VRPathtool::newEntry(pathPtr p, option o, int eID, VRObjectPtr anchor) {
    auto e = entryPtr( new entry( anchor ? anchor : ptr() ) );
    e->p = p;
    e->edge = eID;
    pathToEntry[e->p.get()] = e;
    if (!options.count(e->edge)) options[e->edge] = o;
    return e;
}

void VRPathtool::setupHandles(entryPtr e, VRGeometryPtr ha, VRGeometryPtr he) {
    auto opt = options[e->edge];
    int N = e->p->size();
    for (int i=0; i<N; i++) {
        auto point = e->p->getPoint(i);
        VRGeometryPtr h, h1, h2;
        if (i == 0) h = ha;
        if (i == N-1) h = he;
        if (!h) h = newHandle();
        h->setPose( pose::create(point) );

        if (opt.useControlHandles && i > 0)   h1 = newControlHandle(h, point.pos() + point.dir());
        if (opt.useControlHandles && i < N-1) h2 = newControlHandle(h, point.pos() + point.dir());

        handleToEntries[h.get()].push_back(e);
        if (h1) handleToEntries[h1.get()].push_back(e);
        if (h2) handleToEntries[h2.get()].push_back(e);
        e->addHandle(h, i);
    }
}

void VRPathtool::addPath(pathPtr p, VRObjectPtr anchor, VRGeometryPtr ha, VRGeometryPtr he, bool handles) {
    auto e = newEntry( p, option(10, handles), -options.size(), anchor );
    setupHandles(e, ha, he);
}

VRPathtool::entry::entry(VRObjectPtr a) {
    anchor = a;
}

void VRPathtool::entry::addHandle(VRGeometryPtr h, int i) {
    points[h.get()] = i;
    for (auto c : h->getChildren()) points[(VRGeometry*)c.get()] = i;
    handles.push_back(h);
    if (auto a = anchor.lock()) a->addChild(h);
}

pathPtr VRPathtool::newPath( VRDevicePtr dev, VRObjectPtr anchor, int resolution, bool doCHandles ) { // deprecated?
    auto e = newEntry( path::create(), option(resolution, doCHandles), -options.size(), anchor );
    extrude(0,e->p);
    extrude(dev,e->p);
    return e->p;
}

VRGeometryPtr VRPathtool::newControlHandle(VRGeometryPtr handle, Vec3f p) {
    VRGeometryPtr h;
    h = VRGeometry::create("handle");
    h->setPrimitive("Sphere", "0.04 2");
    handle->addChild(h);

    h->setPickable(true);
    h->addAttachment("controlhandle", 0);
    h->setMaterial(VRMaterial::get("pathHandle"));
    h->getMaterial()->setDiffuse(Vec3f(0.5,0.5,0.9));
    h->setPersistency(0);
    //h->setRelativePosition(p, ptr());
    h->setWorldPosition(p);
    controlhandles.push_back(h);
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
    if (!h) for (auto p : pathToEntry) res.push_back(p.second->p);
    else if (handleToEntries.count(h.get())) {
        auto& ev = handleToEntries[h.get()];
        for (auto e : ev) res.push_back(e->p);
    }
    return res;
}

pathPtr VRPathtool::getPath(VRGeometryPtr h1, VRGeometryPtr h2) {
    if (!handleToEntries.count(h1.get()) || !handleToEntries.count(h2.get())) return 0;
    for (auto& e1 : handleToEntries[h1.get()]) {
        for (auto& e2 : handleToEntries[h2.get()]) {
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
        entryPtr e = pathToEntry[p.get()];
        for (auto h : e->handles) if (auto hl = h.lock()) sor[ e->points[hl.get()] ] = hl;
        for (auto h : sor) res.push_back(h.second);
    }
    return res;
}

VRStrokePtr VRPathtool::getStroke(pathPtr p) {
    if (pathToEntry.count(p.get()) == 0) return 0;
    return pathToEntry[p.get()]->line.lock();
}

VRGeometryPtr VRPathtool::extrude(VRDevicePtr dev, pathPtr p) {
    if (pathToEntry.count(p.get()) == 0) {
        cout << "Warning: VRPathtool::extrude, path " << p << " unknown\n";
        return 0;
    }

    auto e = pathToEntry[p.get()];
    e->p->addPoint( pose(Vec3f(0,0,-1), Vec3f(1,0,0)));

    VRGeometryPtr h = newHandle();
    handleToEntries[h.get()].push_back(e);
    handles.push_back(h);
    e->points[h.get()] = e->points.size()-1;
    e->handles.push_back(h);
    e->anchor.lock()->addChild(h);
    if (dev) {
        dev->drag(h, dev->getBeacon());
        h->setPose( pose::create(Vec3f(0,0,-1)) );
    }

    updateHandle(h);
    return h;
}

void VRPathtool::setHandleGeometry(VRGeometryPtr geo) {
    customHandle = static_pointer_cast<VRGeometry>( geo->duplicate() );
}

void VRPathtool::clear(pathPtr p) {
    if (!p) {
        pathToEntry.clear();
        paths.clear();
        options.clear();
        handleToEntries.clear();
        handles.clear();
        handleToNode.clear();
        if (graph) graph->clear();
        knots.clear();
        selectedPath.reset();
        clearChildren();
        return;
    }

    if (pathToEntry.count(p.get()) == 0) return;
    auto e = pathToEntry[p.get()];

    for (auto h : e->handles) {
        if (auto hl = h.lock()) {
            handleToEntries.erase(hl.get());
            hl->destroy();
        }
    }

    e->handles.clear();
    if (e->line.lock()) e->line.lock()->destroy();
    e->line.reset();
    p->clear();

    paths.erase(e->edge);
    options.erase(e->edge);
    pathToEntry.erase(p.get());
}

void VRPathtool::remPath(pathPtr p) {
    if (pathToEntry.count(p.get()) == 0) return;
    auto e = pathToEntry[p.get()];

    for (auto hw : e->handles) {
        if (auto h = hw.lock()) {
            handleToEntries.erase(h.get());
            h->destroy();
        }
    }

    if (auto l = e->line.lock()) l->destroy();
    pathToEntry.erase(p.get());
}

void VRPathtool::setVisuals(bool hvis, bool lines) {
    for (auto p : pathToEntry) if (p.second->line.lock()) p.second->line.lock()->setVisible(lines);
    for (auto h : handles) {
        if (auto hs = h.lock()) hs->setVisible(hvis);
    }
}

void VRPathtool::select(VRGeometryPtr h) {
    manip->handle(h);
    if (handleToEntries.count(h.get()) == 0) return;
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
