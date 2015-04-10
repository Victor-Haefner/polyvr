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

using namespace OSG;

VRPathtool::VRPathtool() {
    VRFunction<int>* fkt = new VRFunction<int>("path tool update", boost::bind(&VRPathtool::update, this) );
    VRSceneManager::getCurrent()->addUpdateFkt(fkt, 100);
}

int VRPathtool::newPath(VRDevice* dev, VRObject* anchor) {
    int ID = paths.size();

    entry* e = new entry();
    paths[ID] = e;
    e->anchor = anchor;
    e->p = new path();

    extrude(0,ID);
    extrude(dev,ID);
    return ID;
}

void VRPathtool::extrude(VRDevice* dev, int i) {
    if (paths.count(i) == 0) return;
    entry* e = paths[i];
    e->p->addPoint(Vec3f(0,0,-1), Vec3f(1,0,0), Vec3f(1,1,1));

    VRGeometry* h = new VRGeometry("handle");
    h->setPickable(true);
    h->setPrimitive("Sphere", "0.1 1");
    h->addAttachment("dynamicaly_generated", 0);
    handles_dict[h] = e;
    e->handles[h] = e->handles.size()-1;
    e->anchor->addChild(h);
    if (dev) {
        dev->drag(h, dev->getBeacon());
        h->setPose(Vec3f(0,0,-1), Vec3f(0,0,-1), Vec3f(0,1,0));
    }
}

void VRPathtool::remPath(int i) {
    if (paths.count(i) == 0) return;
    entry* e = paths[i];
    for (auto h : e->handles) delete h.first;
    delete e->line;
    delete e->p;
    delete e;
    paths.erase(i);
}

void VRPathtool::updatePath(entry* e) {
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

        Matrix m = obj->getWorldMatrix();
        entry* e = handles_dict[obj];
        e->p->setPoint(e->handles[obj], Vec3f(m[3]), Vec3f(m[2]), Vec3f(1,1,1), Vec3f(m[1]));
        updatePath(e);
    }
}
