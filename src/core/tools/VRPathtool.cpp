#include "VRPathtool.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/setup/devices/VRDevice.h"
#include "core/math/path.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeoFunctions.h>

using namespace OSG;


template<> string toString(const VRPathtool::option& n) {
    stringstream ss;
    ss << n.resolution << " " << n.useControlHandles;
    return ss.str();
}

template<> int toValue(stringstream& ss, VRPathtool::option& n) {
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
    c->setActive(true);

    if (g == gTX || g == gTY || g == gTZ || g == gRX || g == gRY || g == gRZ) { // lock everything
        c->setTConstraint(Vec3d(0,0,0), VRConstraint::LINE);
        c->setRConstraint(Vec3d(0,0,0), VRConstraint::POINT);
    }

    if (g == gTX) c->setTConstraint(Vec3d(1,0,0), VRConstraint::LINE);
    if (g == gTY) c->setTConstraint(Vec3d(0,1,0), VRConstraint::LINE);
    if (g == gTZ) c->setTConstraint(Vec3d(0,0,1), VRConstraint::LINE);
    //if (g == gRX) sel->setRConstraint(Vec3i(0,1,1)); // TODO: fix rotation constraints!
    //if (g == gRY) sel->setRConstraint(Vec3i(1,0,1));
    //if (g == gRZ) sel->setRConstraint(Vec3i(1,1,0));
    if (g == gRX || g == gRY || g == gRZ) c->setRConstraint(Vec3d(0,0,0), VRConstraint::POINT);
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
    gTX->setPrimitive("Box 0.1 0.02 0.02 1 1 1");
    gTY->setPrimitive("Box 0.02 0.1 0.02 1 1 1");
    gTZ->setPrimitive("Box 0.02 0.02 0.1 1 1 1");
    gRX->setPrimitive("Torus 0.01 0.07 4 16");
    gRY->setPrimitive("Torus 0.01 0.07 4 16");
    gRZ->setPrimitive("Torus 0.01 0.07 4 16");
    gRX->setDir(Vec3d(1,0,0));
    gRY->setDir(Vec3d(0,1,0));
    gRY->setUp(Vec3d(1,0,0));
    anchor->addChild(gTX);
    anchor->addChild(gTY);
    anchor->addChild(gTZ);
    anchor->addChild(gRX);
    anchor->addChild(gRY);
    anchor->addChild(gRZ);
}

VRPathtool::option::option(int r, bool uch) : resolution(r), useControlHandles(uch) {}

VRPathtool::VRPathtool() : VRTransform("Pathtool") {
    updatePtr = VRUpdateCb::create("path tool update", bind(&VRPathtool::updateDevs, this) );
    VRScene::getCurrent()->addUpdateFkt(updatePtr, 100);

    manip = new VRManipulator();

    lmat = VRMaterial::create("pline");
    lmat->setLit(false);
    lmat->setDiffuse(Color3f(0.1,0.9,0.2));
    lmat->setLineWidth(3);

    amat = VRMaterial::create("pline");
    amat->setDiffuse(Color3f(0.9,0.9,0.2));

    lsmat = VRMaterial::create("spline");
    lsmat->setLit(false);
    lsmat->setDiffuse(Color3f(0.9,0.1,0.2));
    lsmat->setLineWidth(3);

    arrowTemplate = VRGeometry::create("arrow");
    arrowTemplate->setPrimitive("Arrow 2 3 3 1.5 1.2");
    arrowTemplate->setMaterial(amat);

    storeObj("handle", customHandle);
    storeObj("graph", graph);
    storeMap("paths", &paths, true);
    storeMap("options", options);
    //regStorageSetupBeforeFkt( VRUpdateCb::create("pathtool clear", bind(&VRPathtool::clear, this)) );
    regStorageSetupBeforeFkt( VRStorageCb::create("pathtool clear", bind(&VRPathtool::setupBefore, this, _1)) );
    regStorageSetupFkt( VRStorageCb::create("pathtool setup", bind(&VRPathtool::setup, this, _1)) );
}

VRPathtool::~VRPathtool() {
    if (manip) delete manip;
    // destroy the handles to make sure, they may have been moved in the SG
    for (auto hw : handles       ) if (auto h = hw.lock()) h->destroy();
    for (auto hw : controlHandles) if (auto h = hw.lock()) h->destroy();
}

VRPathtoolPtr VRPathtool::create() { return VRPathtoolPtr( new VRPathtool() ); }

void VRPathtool::setupBefore(VRStorageContextPtr context) {
    bool onlyReload = false;
    if (context) onlyReload = context->onlyReload;
    if (!onlyReload) clear();
}

void VRPathtool::setup(VRStorageContextPtr context) {
    /*bool onlyReload = false;
    if (context) onlyReload = context->onlyReload;
    if (!onlyReload)*/
    setGraph(graph, false);
}

void VRPathtool::setProjectionGeometry(VRObjectPtr obj) { projObj = obj; }

GraphPtr VRPathtool::getGraph() { return graph; }

void VRPathtool::setGraph(GraphPtr g, bool doClear, bool handles, bool doArrows) {
    if (!g) return;
    if (g == graph) graph = 0; // important, else the clear will also clear the graph!
    if (doClear) clear();
    graph = g;

    for (auto& n : g->getNodes()) setGraphNode(n.first);
    for (auto& e : g->getEdges()) setGraphEdge(e.second, handles, doArrows);
}

int VRPathtool::addNode(PosePtr p) {
    if (!graph) setGraph( Graph::create() );
    int i = graph->addNode();
    addHandle(i,p);
    return i;
}

VRGeometryPtr VRPathtool::addHandle(int nID, PosePtr p) {
    if (!graph) return 0;
    auto h = setGraphNode(nID);
    setHandlePose(nID, p);
    return h;
}

void VRPathtool::setHandlePose(int nID, PosePtr p) {
    auto h = getHandle(nID);
    if (h) h->setPose(p);
    if (graph) graph->setPosition(nID, p);
}

void VRPathtool::removeNode(int i) {
    if (!graph) return;
    graph->remNode(i);
    if (knots.count(i)) {
        if (auto h = knots[i].handle.lock()) {
            h->destroy();
            handleToNode.erase(h.get());
        }

        auto nIn = knots[i].in;
        auto nOut = knots[i].out;
        for (auto e : nIn) disconnect(e,i);
        for (auto e : nOut) disconnect(i,e);
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
        if (auto a = e->arrow.lock()) a->destroy();
        vecRemVal(handleToEntries[h1], e);
        vecRemVal(handleToEntries[h2], e);
        pathToEntry.erase(e->p.get());
    }

    vecRemVal(knots[i1].out, i2);
    vecRemVal(knots[i2].in, i1);
}

void VRPathtool::connect(int i1, int i2, Vec3d n1, Vec3d n2, bool handles, bool doArrow) {
    if (i1 == i2) return;
    if (!graph) return;

    if (n1.length() < 1e-4) {
        auto& nodes = graph->getNodes();
        Vec3d d = nodes[i2].p.pos() - nodes[i1].p.pos();
        d.normalize();
        n1 = d;
        n2 = d;
    }

    if (!graph->connected(i1,i2)) {
        int eID = graph->connect(i1,i2);
        cout << "VRPathtool::connect " << i1 << " with " << i2 << ", eID" << eID << endl;
        auto& e = graph->getEdge(eID);
        setGraphEdge(e, handles, doArrow, n1, n2);
    } else {
        cout << "VRPathtool::disconnect " << i1 << " with " << i2 << "!" << endl;
        disconnect(i1,i2);
    }
}

VRMaterialPtr VRPathtool::getArrowMaterial() { return amat; }

void VRPathtool::setArrowSize(float s) {
    auto p = Pose::create();
    float S = s/arrowScale;
    p->setScale(Vec3d(S,S,S));
    arrowTemplate->applyTransformation(p);
    arrowScale = s;
}

void VRPathtool::setGraphEdge(Graph::edge& e, bool handles, bool doArrow) {
    auto& nodes = graph->getNodes();
    setGraphEdge(e, handles, doArrow, nodes[e.from].p.dir(), nodes[e.to].p.dir());
}

void VRPathtool::setGraphEdge(Graph::edge& e, bool handles, bool doArrow, Vec3d n1, Vec3d n2) {
    auto& nodes = graph->getNodes();
    if (!paths.count(e.ID)) {
        paths[e.ID] = Path::create();
        paths[e.ID]->addPoint( Pose(nodes[e.from].p.pos(), n1));
        paths[e.ID]->addPoint( Pose(nodes[e.to].p.pos(), n2));
    }
    auto en = newEntry( paths[e.ID], option(10, handles), e.ID, 0 );
    setupHandles(en, knots[e.from].handle.lock(), knots[e.to].handle.lock());
    knots[e.from].out.push_back(e.to);
    knots[e.to].in.push_back(e.from);
    updateHandle( knots[e.from].handle.lock() );
    updateHandle( knots[e.to  ].handle.lock() );

    if (doArrow) {
        auto a = arrowTemplate->duplicate();
        addChild(a);
        auto ag = dynamic_pointer_cast<VRGeometry>(a);
        ag->setScale(Vec3d(arrowScale, arrowScale, arrowScale));
        en->arrow = ag;
    }
}

VRGeometryPtr VRPathtool::setGraphNode(int i) {
    auto h = newHandle();
    knots[i] = knot();
    knots[i].ID = i;
    knots[i].handle = h;
    handleToNode[h.get()] = i;
    addChild(h);
    //h->setRelativePose( graph->getPosition(i), ptr() );
    h->setWorldPose( graph->getPosition(i) );
    return h;
}

void VRPathtool::updateHandlePose(knot& knot, map<int, Vec3d>& hPositions, bool doUpdateEntry) {
    auto h = knot.handle.lock();
    if (!h) return;

    auto getPose = [&](int ID) {
        if (!hPositions.count(ID)) {
            auto h = knots[ID].handle.lock();
            hPositions[ID] = h ? h->getRelativePosition(ptr()) : Vec3d();
        }
        return hPositions[ID];
    };

    bool didChange = false;
    for (auto k : knot.in ) if (auto h = knots[k].handle.lock()) if (h->changedNow()) { didChange = true; break; }
    for (auto k : knot.out) if (auto h = knots[k].handle.lock()) if (h->changedNow()) { didChange = true; break; }
    if (!didChange) return;

    Vec3d pos = getPose(knot.ID);
    Vec3d dir;

    for (auto k : knot.in) dir += pos - getPose(k);
    for (auto k : knot.out) dir += getPose(k) - pos;

    if (dir.squareLength() > 1e-6) {
        dir.normalize();
        auto key = h.get();
        h->setRelativeDir(dir, ptr());
        h->apply_constraints(true);

        if (doUpdateEntry && handleToEntries.count(key)) {
            for (auto e : handleToEntries[key]) {
                auto opt = options[e->edge];
                int i = e->points[key];
                auto po = e->p->getPoint(i);
                if (!opt.doSmoothGraphNodes) {
                    Vec3d p1 = po.pos();
                    Vec3d p0 = p1;
                    Vec3d p2 = p1;
                    if (i > 0) p0 = e->p->getPoint(i-1).pos();
                    if (i < e->p->size()-1) p2 = e->p->getPoint(i+1).pos();
                    dir = p2-p0;
                    dir.normalize();
                }
                po.setDir(dir - opt.bulge*(2.0*i/(e->p->size()-1)-1));
                e->p->setPoint( e->points[key], po );
                e->doUpdate = true;
                updateEntry(e);
            }
        }
    }
}

void VRPathtool::update() { // call in script to have smooth knots
    if (graph) { // smooth knot transformations
        map<int, Vec3d> hPositions; // get handle positions
        for (auto& knot : knots) updateHandlePose(knot.second, hPositions, false); // compute and set direction of handles
    }

    for (auto wh : handles) { // apply handle transformation to path points
        auto handle = wh.lock();
        if (!handle) continue;
        if (!handle->changedNow()) continue;
        auto key = handle.get();
        if (!handleToEntries.count(key)) continue;
        for (auto e : handleToEntries[key]) {
            auto opt = options[e->edge];
            int i = e->points[key];
            auto po = *e->anchor.lock()->getPoseTo(handle);
            if (!opt.doSmoothGraphNodes) {
                Vec3d p1 = po.pos();
                Vec3d p0 = p1;
                Vec3d p2 = p1;
                if (i > 0) p0 = e->p->getPoint(i-1).pos();
                if (i < e->p->size()-1) p2 = e->p->getPoint(i+1).pos();
                Vec3d dir = p2-p0;
                dir.normalize();
                po.setDir(dir);
            }
            po.setDir( po.dir() - opt.bulge*(2.0*i/(e->p->size()-1)-1) );
            e->p->setPoint( i, po );
            e->doUpdate = true;
        }
    }

    for (auto e : pathToEntry) updateEntry(e.second);
    updateBezierVisuals();
}

void VRPathtool::updateEntry(entryPtr e) { // update path representation
    if (!e) return;
    if (!e->doUpdate) return;
    int hN = e->handles.size();
    if (hN <= 0) return;
    auto opt = options[e->edge];

    for (int i=0; i<e->p->size(); i++) { // apply options
        /*if (graph) { // TODO
            Vec3d n = e->handles[i].lock()->getPose()->dir();
            n -= opt.bulge*(2.0*i/(e->p->size()-1)-1);
            e->p->getPoint(i).setDir(n);
        }*/

        if (opt.useColors) {
            Color3f c = opt.color1 + (opt.color2-opt.color1)*float(i)/(e->p->size()-1);
            e->p->setPointColor(i, c);
        }
    }

    e->p->compute(opt.resolution);
    int pN = e->p->getPositions().size();
    if (pN < 2) return;

    if (!e->anchor.lock()) e->anchor = ptr();
    if (!e->line.lock() && e->anchor.lock()) { // update path line
        auto line = VRStroke::create("path");
        e->line = line;
        line->setPersistency(0);
        line->setMaterial(lmat);
        e->anchor.lock()->addChild(line);
        vector<Vec3d> profile;
        profile.push_back(Vec3d());
        line->addPath(e->p);
        line->strokeProfile(profile, 0, 0, opt.useColors);
    }

    if (auto l = e->line.lock()) {
        l->setDoColor( opt.useColors );
        l->update();
        l->setVisible(opt.isVisible);
    }

    if (auto a = e->arrow.lock()) {
        a->setPose(e->p->getPose(0.5));
        a->setVisible(opt.isVisible);
    }

    e->doUpdate = false;
}

void VRPathtool::updateHandle(VRGeometryPtr handle) { // update paths the handle belongs to
    if (!handle) return;
    auto p = getPoseTo(handle);
    auto key = handle.get();
    if (!handleToEntries.count(key)) return;

    if (handle->hasTag("handle")) {
        if (graph && handleToNode.count(key)) graph->setPosition( handleToNode[key], p );

        for (auto e : handleToEntries[key]) {
            if (!e || !e->p || e->points.count(key) == 0) continue;
            auto pnt = e->points[key];
            if (e->p->size() <= pnt) continue;
            auto op = e->p->getPoint(pnt);
            e->p->setPoint( e->points[key], Pose(p->pos(), op.dir(), p->up()));
            updateEntry(e);
        }

        for (auto c : handle->getChildrenWithTag("controlhandle")) {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(c);
            if (g) updateHandle(g);
        }

        if (handle->getChildrenWithTag("controlhandle").size() == 0) { // no control handles -> smooth knots curve
            map<int, Vec3d> hPositions; // get handle positions

            int nID = handleToNode[key];
            updateHandlePose( knots[nID], hPositions );
            for (auto k : knots[nID].out) updateHandlePose( knots[k], hPositions );
            for (auto k : knots[nID].in ) updateHandlePose( knots[k], hPositions );
        }
    }

    if (handle->hasTag("controlhandle")) {
        VRGeometryPtr pg = dynamic_pointer_cast<VRGeometry>(handle->getDragParent());
        if (!pg) pg = dynamic_pointer_cast<VRGeometry>(handle->getParent());
        if (!pg) return; // failed to get parent handle

        //Vec3d d = p->pos() - pg->getRelativePosition(ptr());
        Vec3d d = p->pos() - pg->getWorldPosition();
        auto e = handleToEntries[key][0]; // should only be one entry at most!
        auto op = e->p->getPoint(e->points[key]);
        e->p->setPoint( e->points[key], Pose(op.pos(), d, op.up()));
        updateEntry(e);
    }
}

void VRPathtool::projectHandle(VRGeometryPtr handle, VRDevicePtr dev) { // project handle on projObj if set
    if (!projObj) return;
    OSG::VRIntersection ins = dev->intersect(projObj);
    if (ins.hit) {
        Vec3d d = handle->getWorldDirection();
        //Vec3d d = handle->getRelativeDirection(ptr());
        d -= d.dot(ins.normal)*ins.normal;
        //handle->setRelativePose( pose::create(Vec3d(ins.point), d, ins.normal), ptr() );
        handle->setWorldPose( Pose::create(Vec3d(ins.point), d, ins.normal) );
    }
}

void VRPathtool::updateDevs() { // update when something is dragged
    for (auto dev : VRSetup::getCurrent()->getDevices()) { // get dragged objects
        VRGeometryPtr obj = static_pointer_cast<VRGeometry>(dev.second->getDraggedObject());
        if (obj == 0) continue;
        if (!obj->hasTag("handle") && !obj->hasTag("controlhandle")) continue;
        if (!handleToNode.count(obj.get()) && !handleToEntries.count(obj.get())) continue;
        projectHandle(obj, dev.second);
        updateHandle(obj);
        updateBezierVisuals();
    }
}

VRPathtool::entryPtr VRPathtool::newEntry(PathPtr p, option o, int eID, VRObjectPtr anchor) {
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
        h->setPose( Pose::create(point) );

        if (opt.useControlHandles && i > 0)   h1 = newControlHandle(h, point.pos() + point.dir());
        if (opt.useControlHandles && i < N-1) h2 = newControlHandle(h, point.pos() + point.dir());

        handleToEntries[h.get()].push_back(e);
        if (h1) handleToEntries[h1.get()].push_back(e);
        if (h2) handleToEntries[h2.get()].push_back(e);
        e->addHandle(h, i);
    }
}

void VRPathtool::addPath(PathPtr p, VRObjectPtr anchor, VRGeometryPtr ha, VRGeometryPtr he, bool handles) {
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

PathPtr VRPathtool::newPath( VRDevicePtr dev, VRObjectPtr anchor, int resolution, bool doCHandles ) { // deprecated?
    auto e = newEntry( Path::create(), option(resolution, doCHandles), -options.size(), anchor );
    extrude(0,e->p);
    extrude(dev,e->p);
    return e->p;
}

void VRPathtool::setEdgeResolution(int eID, int resolution) { options[eID].resolution = resolution; }
void VRPathtool::setEdgeColor(int eID, Color3f color1, Color3f color2) { options[eID].color1 = color1; options[eID].color2 = color2; options[eID].useColors = true; }
void VRPathtool::setEdgeBulge(int eID, Vec3d bulge) { options[eID].bulge = bulge; }
void VRPathtool::setEdgeSmoothGraphNodes(int eID, bool b) { options[eID].doSmoothGraphNodes = b; }
void VRPathtool::setEdgeVisibility(int eID, bool b) { options[eID].isVisible = b; }

VRGeometryPtr VRPathtool::newControlHandle(VRGeometryPtr handle, Vec3d p) {
    VRGeometryPtr h;
    h = VRGeometry::create("handle");
    h->setPrimitive("Sphere 0.04 2");
    handle->addChild(h);

    h->setPickable(true);
    h->addAttachment("controlhandle", 0);
    h->setMaterial(VRMaterial::get("pathHandle"));
    h->getMaterial()->setDiffuse(Color3f(0.5,0.5,0.9));
    h->setPersistency(0);
    //h->setRelativePosition(p, ptr());
    h->setWorldPosition(p);
    controlHandles.push_back(h);
    return h;
}

VRGeometryPtr VRPathtool::newHandle() {
    VRGeometryPtr h;

    if (customHandle) {
        h = static_pointer_cast<VRGeometry>( customHandle->duplicate() );
    } else {
        h = VRGeometry::create("handle");
        h->setPrimitive("Torus 0.04 0.15 3 4");
    }

    h->setPickable(true);
    h->addAttachment("handle", 0);
    h->setMaterial(VRMaterial::get("pathHandle"));
    h->getMaterial()->setDiffuse(Color3f(0.5,0.5,0.9));
    //calcFaceNormals(h->getMesh()); // not working ??
    h->setPersistency(0);
    handles.push_back(h);
    return h;
}

VRGeometryPtr VRPathtool::getHandle(int ID) {
    //cout << "VRPathtool::getHandle ID: " << ID << " knots:" << endl;
    //for (auto k : knots) cout << " knot ID: " << k.first << " knot handle: " << k.second.handle.lock() << endl;
    return knots[ID].handle.lock();
}

vector<PathPtr> VRPathtool::getPaths(VRGeometryPtr h) {
    vector<PathPtr> res;
    if (!h) for (auto p : pathToEntry) res.push_back(p.second->p);
    else if (handleToEntries.count(h.get())) {
        auto& ev = handleToEntries[h.get()];
        for (auto e : ev) res.push_back(e->p);
    }
    return res;
}

PathPtr VRPathtool::getPath(VRGeometryPtr h1, VRGeometryPtr h2) {
    if (!handleToEntries.count(h1.get()) || !handleToEntries.count(h2.get())) return 0;
    for (auto& e1 : handleToEntries[h1.get()]) {
        for (auto& e2 : handleToEntries[h2.get()]) {
            if (e1->p == e2->p) return e1->p;
        }
    }
    return 0;
}

vector<VRGeometryPtr> VRPathtool::getHandles(PathPtr p) {
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

VRStrokePtr VRPathtool::getStroke(PathPtr p) {
    if (pathToEntry.count(p.get()) == 0) return 0;
    return pathToEntry[p.get()]->line.lock();
}

VRGeometryPtr VRPathtool::extrude(VRDevicePtr dev, PathPtr p) {
    if (pathToEntry.count(p.get()) == 0) {
        cout << "Warning: VRPathtool::extrude, path " << p << " unknown\n";
        return 0;
    }

    auto e = pathToEntry[p.get()];
    e->p->addPoint( Pose(Vec3d(0,0,-1), Vec3d(1,0,0)));

    VRGeometryPtr h = newHandle();
    handleToEntries[h.get()].push_back(e);
    handles.push_back(h);
    e->points[h.get()] = e->p->size()-1;
    e->handles.push_back(h);
    e->anchor.lock()->addChild(h);

    if (dev) {
        dev->drag(h);
        h->setPose( Pose::create(Vec3d(0,0,-1)) );
    }

    updateHandle(h);
    return h;
}

void VRPathtool::setHandleGeometry(VRGeometryPtr geo) {
    customHandle = static_pointer_cast<VRGeometry>( geo->duplicate() );
}

void VRPathtool::clear(PathPtr p) {
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

void VRPathtool::remPath(PathPtr p) {
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

void VRPathtool::setBezierVisuals(bool cpvis, bool hvis) {
    showBControlPoints = cpvis;
    showBHulls = hvis;
    updateBezierVisuals();
}

void VRPathtool::updateBezierVisuals() {
    auto cps = controlPoints.lock();
    auto bhs = bezierHulls.lock();

    if (!showBControlPoints && cps) cps->destroy();
    if (!showBHulls && bhs) bhs->destroy();

    if (showBControlPoints) {
        if (!cps) {
            cps = VRGeometry::create("BezierControlPoints");
            addChild(cps);
            auto m = VRMaterial::create("BezierControlPointsMat");
            m->setPointSize(5);
            m->setWireFrame(true);
            m->setLit(false);
            m->setDiffuse(Color3f(0,0,1));
            cps->setMaterial(m);
            cps->hide("SHADOW");
            controlPoints = cps;
        }
        VRGeoData data;
        for (auto pE : pathToEntry) {
            if (auto path = pE.second->p) {
                for (auto cP : path->getControlPoints()) {
                    data.pushVert(cP);
                    data.pushPoint();
                }
                for (auto P : path->getPoints()) {
                    data.pushVert(P.pos());
                    data.pushPoint();
                }
            }
        }
        data.apply(cps);
    }

    if (showBHulls) {
        if (!bhs) {
            bhs = VRGeometry::create("BezierHulls");
            addChild(bhs);
            auto m = VRMaterial::create("BezierHullsMat");
            m->setPointSize(5);
            m->setWireFrame(true);
            m->setLit(false);
            m->setDiffuse(Color3f(0,0,1));
            bhs->setMaterial(m);
            bhs->hide("SHADOW");
            bezierHulls = bhs;
        }
        VRGeoData data;
        for (auto pE : pathToEntry) {
            if (auto path = pE.second->p) {
                auto cPs = path->getControlPoints();
                auto Ps = path->getPoints();
                if ((Ps.size()-1)*2 != cPs.size()) continue;
                for (unsigned int i=0; i<Ps.size()-1; i++) {
                    int p1 = data.pushVert(Ps[i].pos());
                    int p2 = data.pushVert(cPs[i*2]);
                    int p3 = data.pushVert(cPs[i*2+1]);
                    int p4 = data.pushVert(Ps[i+1].pos());
                    data.pushTri(p1, p4, p2);
                    data.pushTri(p1, p3, p4);
                    data.pushTri(p1, p2, p3);
                    data.pushTri(p2, p4, p3);
                }
            }
        }
        data.apply(bhs);
    }
}


void VRPathtool::select(VRGeometryPtr h) {
    manip->handle(h);
    if (handleToEntries.count(h.get()) == 0) return;
    manip->manipulate(h);
}

void VRPathtool::selectPath(PathPtr p) {
    if (selectedPath) getStroke(selectedPath)->setMaterial(lmat);
    selectedPath = p;
    getStroke(selectedPath)->setMaterial(lsmat);
}

void VRPathtool::deselect() {
    if (selectedPath) getStroke(selectedPath)->setMaterial(lmat);
}

VRMaterialPtr VRPathtool::getPathMaterial() { return lmat; }
