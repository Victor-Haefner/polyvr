#include "VRPathtool.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/devices/VRDevice.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeoFunctions.h>

using namespace OSG;

VRPathtool::VRPathtool() {
    VRFunction<int>* fkt = new VRFunction<int>("path tool update", boost::bind(&VRPathtool::update, this) );
    VRSceneManager::getCurrent()->addUpdateFkt(fkt, 100);

    manip = new VRManipulator();
}

void VRPathtool::addPath(path* p, VRObject* anchor) {
    entry* e = new entry();
    e->anchor = anchor;
    e->p = p;
    paths[e->p] = e;

    for (auto point : p->getPoints()) {
        VRGeometry* h = newHandle();
        handles_dict[h] = e;
        e->handles[h] = e->handles.size()-1;
        e->anchor->addChild(h);
        h->setPose(point.p, point.n, point.u);
    }
}

path* VRPathtool::newPath(VRDevice* dev, VRObject* anchor) {
    entry* e = new entry();
    e->anchor = anchor;
    e->p = new path();
    paths[e->p] = e;

    extrude(0,e->p);
    extrude(dev,e->p);
    return e->p;
}

VRGeometry* VRPathtool::newHandle() {
    VRGeometry* h = new VRGeometry("handle");
    h->setPickable(true);
    h->setPrimitive("Torus", "0.04 0.15 3 4");
    //calcFaceNormals(h->getMesh()); // not working ??
    h->addAttachment("dynamicaly_generated", 0);
    return h;
}

vector<path*> VRPathtool::getPaths() {
    vector<path*> res;
    for (auto p : paths) res.push_back(p.first);
    return res;
}

vector<VRGeometry*> VRPathtool::getHandles(path* p) {
    vector<VRGeometry*> res;
    if (p == 0) for (auto h : handles_dict) res.push_back(h.first);
    else {
        entry* e = paths[p];
        for (auto h : e->handles) res.push_back(h.first);
    }
    return res;
}

void VRPathtool::extrude(VRDevice* dev, path* p) {
    if (paths.count(p) == 0) return;
    entry* e = paths[p];
    e->p->addPoint(Vec3f(0,0,-1), Vec3f(1,0,0), Vec3f(1,1,1));

    VRGeometry* h = newHandle();
    handles_dict[h] = e;
    e->handles[h] = e->handles.size()-1;
    e->anchor->addChild(h);
    if (dev) {
        dev->drag(h, dev->getBeacon());
        h->setPose(Vec3f(0,0,-1), Vec3f(0,0,-1), Vec3f(0,1,0));
    }

    updateHandle(h);
}

void VRPathtool::remPath(path* p) {
    if (paths.count(p) == 0) return;
    entry* e = paths[p];
    for (auto h : e->handles) delete h.first;
    delete e->line;
    delete e->p;
    delete e;
    paths.erase(p);
}

void VRPathtool::updateHandle(VRGeometry* handle) {
    Matrix m = handle->getWorldMatrix();
    entry* e = handles_dict[handle];
    e->p->setPoint(e->handles[handle], Vec3f(m[3]), Vec3f(m[2]), Vec3f(1,1,1), Vec3f(m[1]));

    int hN = e->handles.size();
    if (hN <= 0) return;

    e->p->compute(5*hN);
    int pN = e->p->getPositions().size();
    if (pN <= 2) return;

    // update path line
    if (e->line == 0) {
        e->line = new VRGeometry("path");
        VRMaterial* matl = new VRMaterial("pline");
        matl->setLit(false);
        matl->setDiffuse(Vec3f(0.1,0.9,0.2));
        matl->setLineWidth(3);
        e->anchor->addChild(e->line);
        e->line->setType(GL_LINE_STRIP);
        e->line->setMaterial(matl);
    }

    GeoUInt32PropertyRecPtr Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr Pos = GeoPnt3fProperty::create();
    Length->addValue(pN);
    for (Vec3f p : e->p->getPositions()) Pos->addValue(p);

    e->line->setPositions(Pos);
    e->line->setLengths(Length);
}

void VRPathtool::update() {
    for (auto dev : VRSetupManager::getCurrent()->getDevices()) { // get dragged objects
        VRGeometry* obj = (VRGeometry*)dev.second->getDraggedObject();
        if (obj == 0) continue;
        if (handles_dict.count(obj) == 0) continue;
        updateHandle(obj);
    }
}

void VRPathtool::setVisible(bool b) {
    for (auto p : paths) p.second->line->setVisible(b);
    for (auto h : handles_dict) h.first->setVisible(b);
}

void VRPathtool::select(VRGeometry* h) {
    manip->handle(h);

    if (handles_dict.count(h) == 0) return;

    manip->manipulate(h);
}
