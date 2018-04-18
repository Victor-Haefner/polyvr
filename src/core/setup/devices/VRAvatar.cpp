#include "VRAvatar.h"
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/VRTransform.h"
#include "core/objects/material/VRMaterial.h"
#include "core/setup/VRSetup.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRObjectPtr VRAvatar::initRay() {
    VRGeoData rayData;
    int N = 100;
    float L = 50;
    float d = L/N;
    for (int i=1; i<N; i++) {
        Color3f c = Color3f(1-0.3*((i+1)%2),1-0.3*(i%2),0);
        rayData.pushVert(Vec3d(0,0,-(i-1)*d), Vec3d(0,0,-1), c);
        rayData.pushVert(Vec3d(0,0,-    i*d), Vec3d(0,0,-1), c);
        rayData.pushLine();
    }

    VRMaterialPtr mat = VRMaterial::get("yellow_ray");
    mat->setLineWidth(6);
    mat->setLit(0);

    VRGeometryPtr ray = rayData.asGeometry(this->deviceName + "_av_ray");
    ray->setMaterial(mat);
    return ray;
}

VRObjectPtr VRAvatar::initCone() {
    VRGeometryPtr cone = VRGeometry::create("av_cone");
    cone->setMesh( OSGGeometry::create(makeConeGeo(0.3, 0.03, 32, true, true)) );
    cone->setFrom(Vec3d(0,0,-0.1));
    cone->setOrientation(Vec3d(1,0,-0.1), Vec3d(0,0,-1));

    return cone;
}

VRObjectPtr VRAvatar::initBroadRay() {//path?
    VRGeometryPtr geo = VRGeometry::create("av_broadray");
    //geo->setMesh(VRSceneLoader::get()->loadWRL("mod/flystick/fly2_w_ray.wrl"));

    return geo;
}

void VRAvatar::addAll(int i) {
    map<string, VRObjectPtr>::iterator itr = beacons[i].avatars.begin();
    for(;itr != beacons[i].avatars.end();itr++) {
      beacons[i].beacon->addChild(itr->second);
    }
}

void VRAvatar::hideAll(int i) {
    map<string, VRObjectPtr>::iterator itr = beacons[i].avatars.begin();
    for(;itr != beacons[i].avatars.end();itr++) itr->second->hide();
}

void VRAvatar::addAvatar(VRObjectPtr geo, int i) {
    beacons[i].beacon->addChild(geo);
}

VRAvatar::VRAvatar(string name) {
    this->deviceName = name;
    cout << "Device Name: " << this->deviceName << endl;
    deviceRoot = VRTransform::create(name + "_beacons");
    deviceRoot->setPersistency(0);

    addBeacon();
}

VRAvatar::~VRAvatar() {}

void VRAvatar::enableAvatar(string avatar, int i) { if (beacons[i].avatars.count(avatar)) beacons[i].avatars[avatar]->show(); }
void VRAvatar::enableAvatar(string avatar) { enableAvatar(avatar, 0); }
void VRAvatar::disableAvatar(string avatar, int i) { if (beacons[i].avatars.count(avatar)) beacons[i].avatars[avatar]->hide(); }
void VRAvatar::disableAvatar(string avatar) { disableAvatar(avatar, 0); }


void VRAvatar::addBeacon() {
    map<string, VRObjectPtr> m;
    Beacon newB{0, 0, m};
    beacons.push_back(newB);

    int id = beacons.size()-1;

    beacons[id].beacon = VRTransform::create(this->deviceName + "_beacon_" + to_string(id));
    beacons[id].beacon->setPersistency(0);
    beacons[id].tmpContainer = VRTransform::create(this->deviceName + "_tmp_beacon_" + to_string(id));
    beacons[id].tmpContainer->setPersistency(0);

    beacons[id].avatars["ray"] = initRay();
    //newB.avatars["cone"] = initCone();
    //newB.avatars["broadray"] = initBroadRay();

    addAll(id);
    hideAll(id);

    deviceRoot->addChild(beacons[id].beacon);

    //return beacons.back();
}

VRTransformPtr VRAvatar::getBeaconRoot() {
    return deviceRoot;
}

VRTransformPtr VRAvatar::getBeacon(int i) { return beacons[i].beacon; }
VRTransformPtr VRAvatar::getBeacon() { return beacons[0].beacon; }

VRTransformPtr VRAvatar::editBeacon(int i) { return beacons[i].tmpContainer?beacons[i].tmpContainer:beacons[i].beacon; }
VRTransformPtr VRAvatar::editBeacon() { return beacons[0].tmpContainer?beacons[0].tmpContainer:beacons[0].beacon; }

void VRAvatar::setBeacon(VRTransformPtr b, int i) {
    beacons[i].beacon = b;
    beacons[i].tmpContainer = 0;
    for (auto a : beacons[i].avatars) a.second->switchParent(b);
}
void VRAvatar::setBeacon(VRTransformPtr b) { setBeacon(b, 0);}

void VRAvatar::updateBeacons() {
    for (auto b : beacons) b.beacon->updateTransform(b.tmpContainer);
}

OSG_END_NAMESPACE;
