#include "VRPathtool.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRGeometry.h"
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

VRGeometry* VRPathtool::newHandle(entry* e, VRDevice* dev) {
    VRGeometry* h = new VRGeometry("handle");
    h->setPickable(true);
    h->setPrimitive("Sphere", "0.1 2");
    h->addAttachment("dynamicaly_generated", 0);
    handles[h] = e;
    e->handles[h] = e->handles.size();
    e->anchor->addChild(h);
    if (dev) {
        dev->drag(h, dev->getBeacon());
        h->setPose(Vec3f(0,0,-1), Vec3f(0,0,-1), Vec3f(0,1,0));
    }
    return h;
}

int VRPathtool::newPath(VRDevice* dev, VRObject* anchor) {
    entry* e = new entry();
    e->anchor = anchor;

    e->p = new path();
    e->p->addPoint(Vec3f(0,0,-1), Vec3f(0,0,-1), Vec3f(1,1,1));
    e->p->addPoint(Vec3f(0,0,-1), Vec3f(0,0,-1), Vec3f(1,1,1));

    e->line = new VRGeometry("path");
    e->anchor->addChild(e->line);

    newHandle(e);
    newHandle(e, dev);

    int N = paths.size();
    paths[N] = e;
    return N;
}

void VRPathtool::extrude(VRDevice* dev, int i) {
    if (paths.count(i) == 0) return;
    entry* e = paths[i];
    e->p->addPoint(Vec3f(0,0,-1), Vec3f(0,0,1), Vec3f(1,1,1));
    newHandle(e, dev);
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

void VRPathtool::update() {
    for (auto dev : VRSetupManager::getCurrent()->getDevices()) { // get dragged objects
        VRGeometry* obj = (VRGeometry*)dev.second->getDraggedObject();
        if (obj == 0) continue;
        if (handles.count(obj) == 0) continue;

        Matrix m = obj->getWorldMatrix();
        Vec3f pos = Vec3f(m[3]);
        Vec3f dir = Vec3f(m[2]);
        Vec3f up = Vec3f(m[1]);
        entry* e = handles[obj];

        // update path
        e->p->setPoint(e->handles[obj], pos, dir, Vec3f(1,1,1), up);
        e->p->compute(20*handles.size());

        // update path line
        GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
        GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
        Length->addValue(e->p->getPositions().size());
        for (Vec3f p : e->p->getPositions()) Pos->addValue(p);
        e->line->setPositions(Pos);
        e->line->setLengths(Length);
        e->line->setType(GL_LINE_STRIP);
    }
}
