#include "VRPathtool.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/setup/devices/VRDevice.h"

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
}

VRPathtoolPtr VRPathtool::create() { return VRPathtoolPtr( new VRPathtool() ); }

void VRPathtool::setGraph(graph_basePtr g) {
    if (!g) return;
    graph = g;
    clear();
    knots.clear();
    auto& nodes = g->getNodes();
    auto& edges = g->getEdges();

    for (int i=0; i<nodes.size(); i++) {
        knots[i] = knot();
        auto h = newHandle();
        knots[i].handle = h;
        addChild(h);
    }

    for (auto& n : edges) {
        for (auto& e : n) {
            path* p = new path();
            p->addPoint(nodes[e.from].box.center());
            p->addPoint(nodes[e.to].box.center());
            addPath(p, 0, knots[e.from].handle.lock(), knots[e.to].handle.lock());
            knots[e.from].out.push_back(e.to);
            knots[e.to].in.push_back(e.from);
        }
    }
}

void VRPathtool::update() {
    if (graph) { // smooth knot transformations
        auto& nodes = graph->getNodes();
        auto& edges = graph->getEdges();

        // get handle positions
        map<int, Vec3f> hPositions;
        for (int i=0; i<knots.size(); i++)
            if (auto h = knots[i].handle.lock()) hPositions[i] = h->getWorldPosition();

        for (int i=0; i<knots.size(); i++) {
            auto& knot = knots[i];
            auto h = knot.handle.lock();
            if (!h) continue;

            Vec3f pos = h->getWorldPosition();
            Vec3f dir;
            for (auto k : knot.out) if (hPositions.count(k)) dir += pos - hPositions[k];
            for (auto k : knot.in) if (hPositions.count(k)) dir += hPositions[k] - pos;
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

void VRPathtool::updateEntry(entry* e) {
    if (!e) return;
    int hN = e->handles.size();
    if (hN <= 0) return;
    e->p->compute(e->resolution);
    int pN = e->p->getPositions().size();
    if (pN <= 2) return;

    if (!e->line.lock()) { // update path line
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

    e->line.lock()->update();
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

void VRPathtool::updateDevs() { // update when something is dragged
    for (auto dev : VRSetup::getCurrent()->getDevices()) { // get dragged objects
        VRGeometryPtr obj = static_pointer_cast<VRGeometry>(dev.second->getDraggedObject());
        if (obj == 0) continue;
        if (entries.count(obj.get()) == 0) continue;
        updateHandle(obj);
    }
}

void VRPathtool::addPath(path* p, VRObjectPtr anchor, VRGeometryPtr ha, VRGeometryPtr he) {
    entry* e = new entry();
    e->anchor = anchor ? anchor : ptr();
    e->p = p;
    paths[e->p] = e;

    for (int i=0; i<p->size(); i++) {
        auto point = p->getPoint(i);
        VRGeometryPtr h;
        if (i == 0) h = ha;
        if (i == p->size()-1) h = he;
        if (!h) h = newHandle();

        entries[h.get()].push_back(e);
        e->addHandle(h);
        h->setPose(point.p, point.n, point.u);
    }
}

void VRPathtool::entry::addHandle(VRGeometryPtr h) {
    points[h.get()] = points.size()-1;
    handles.push_back(h);
    anchor.lock()->addChild(h);
}

path* VRPathtool::newPath(VRDevicePtr dev, VRObjectPtr anchor, int resolution) {
    entry* e = new entry();
    e->anchor = anchor ? anchor : ptr();
    e->p = new path();
    e->resolution = resolution;
    paths[e->p] = e;

    extrude(0,e->p);
    extrude(dev,e->p);
    return e->p;
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

vector<path*> VRPathtool::getPaths() {
    vector<path*> res;
    for (auto p : paths) res.push_back(p.first);
    return res;
}

path* VRPathtool::getPath(VRGeometryPtr h1, VRGeometryPtr h2) {
    if (!entries.count(h1.get()) || !entries.count(h2.get())) return 0;
    for (auto& e1 : entries[h1.get()]) {
        for (auto& e2 : entries[h2.get()]) {
            if (e1->p == e2->p) return e1->p;
        }
    }
    return 0;
}

vector<VRGeometryPtr> VRPathtool::getHandles(path* p) {
    vector<VRGeometryPtr> res;
    if (p == 0) for (auto h : handles) res.push_back(h.lock());
    else {
        map<int, VRGeometryPtr> sor;
        entry* e = paths[p];
        for (auto h : e->handles) if (auto hl = h.lock()) sor[ e->points[hl.get()] ] = hl;
        for (auto h : sor) res.push_back(h.second);
    }
    return res;
}

VRStrokePtr VRPathtool::getStroke(path* p) {
    if (paths.count(p) == 0) return 0;
    return paths[p]->line.lock();
}

VRGeometryPtr VRPathtool::extrude(VRDevicePtr dev, path* p) {
    if (paths.count(p) == 0) {
        cout << "Warning: VRPathtool::extrude, path " << p << " unknown\n";
        return 0;
    }

    entry* e = paths[p];
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

void VRPathtool::clear(path* p) {
    if (p == 0) {
        auto tmp = paths;
        for (auto path : tmp) clear(path.first);
        return;
    }

    if (paths.count(p) == 0) return;
    entry* e = paths[p];

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

void VRPathtool::remPath(path* p) {
    if (paths.count(p) == 0) return;
    entry* e = paths[p];

    for (auto hw : e->handles) {
        if (auto h = hw.lock()) {
            entries.erase(h.get());
            h->destroy();
        }
    }

    if (auto l = e->line.lock()) l->destroy();
    delete e->p;
    delete e;
    paths.erase(p);
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

VRMaterialPtr VRPathtool::getPathMaterial() { return lmat; }
