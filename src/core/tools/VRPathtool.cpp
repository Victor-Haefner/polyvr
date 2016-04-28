#include "VRPathtool.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
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


VRPathtool::VRPathtool() {
    updatePtr = VRFunction<int>::create("path tool update", boost::bind(&VRPathtool::updateDevs, this) );
    VRSceneManager::getCurrent()->addUpdateFkt(updatePtr, 100);

    manip = new VRManipulator();

    lmat = VRMaterial::create("pline");
    lmat->setLit(false);
    lmat->setDiffuse(Vec3f(0.1,0.9,0.2));
    lmat->setLineWidth(3);
}

void VRPathtool::update() {
    for (auto h : handles) updateHandle(h.lock());
}

void VRPathtool::addPath(path* p, VRObjectPtr anchor) {
    entry* e = new entry();
    e->anchor = anchor;
    e->p = p;
    paths[e->p] = e;

    for (auto point : p->getPoints()) {
        VRGeometryPtr h = newHandle();
        entries[h.get()] = e;
        handles.push_back(h);
        e->points[h.get()] = e->points.size()-1;
        e->handles.push_back(h);
        e->anchor.lock()->addChild(h);
        h->setPose(point.p, point.n, point.u);
    }
}

path* VRPathtool::newPath(VRDevicePtr dev, VRObjectPtr anchor, int resolution) {
    entry* e = new entry();
    e->anchor = anchor;
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
    return h;
}

vector<path*> VRPathtool::getPaths() {
    vector<path*> res;
    for (auto p : paths) res.push_back(p.first);
    return res;
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

    cout << "get handles " << p << endl;
    for (auto h : res) cout << " " << h->getName() << endl;
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
    entries[h.get()] = e;
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

void VRPathtool::updateHandle(VRGeometryPtr handle) {
    if (!handle) return;
    auto key = handle.get();
    if (!entries.count(key)) return;
    entry* e = entries[key];
    Matrix m = handle->getWorldMatrix();
    e->p->setPoint(e->points[key], Vec3f(m[3]), Vec3f(m[2]), Vec3f(1,1,1), Vec3f(m[1]));

    int hN = e->handles.size();
    if (hN <= 0) return;

    e->p->compute(e->resolution);
    int pN = e->p->getPositions().size();
    if (pN <= 2) return;

    // update path line
    if (!e->line.lock()) {
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

void VRPathtool::updateDevs() {
    for (auto dev : VRSetupManager::getCurrent()->getDevices()) { // get dragged objects
        VRGeometryPtr obj = static_pointer_cast<VRGeometry>(dev.second->getDraggedObject());
        if (obj == 0) continue;
        if (entries.count(obj.get()) == 0) continue;
        updateHandle(obj);
    }
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
