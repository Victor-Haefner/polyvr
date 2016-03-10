#include "VRAvatar.h"
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRTransform.h"
#include "core/objects/material/VRMaterial.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRObjectPtr VRAvatar::initRay() {
    VRGeometryPtr ray = VRGeometry::create("av_ray");

    vector<Vec3f> pos, norms;
    vector<Vec2f> texs;
    vector<int> inds;

    //pos.push_back(Vec3f(0,0,-5));
    //pos.push_back(Vec3f(0,0,50));
    pos.push_back(Vec3f(0,0,0));
    pos.push_back(Vec3f(0,0,-50));

    for (int i=0;i<2;i++) {
        norms.push_back(Vec3f(0,0,-1));
        inds.push_back(i);
        texs.push_back(Vec2f(0,0));
    }

    VRMaterialPtr mat = VRMaterial::get("yellow_ray");
    mat->setLineWidth(6);
    mat->setDiffuse(Color3f(1,1,0));
    mat->setAmbient(Color3f(1,1,0));
    mat->setSpecular(Color3f(1,1,0));

    ray->create(GL_LINES, pos, norms, inds, texs);
    ray->setMaterial(mat);

    return ray;
}

VRObjectPtr VRAvatar::initCone() {
    VRGeometryPtr cone = VRGeometry::create("av_cone");
    cone->setMesh(makeConeGeo(0.3, 0.03, 32, true, true));
    cone->setFrom(Vec3f(0,0,-0.1));
    cone->setOrientation(Vec3f(1,0,-0.1), Vec3f(0,0,-1));

    return cone;
}

VRObjectPtr VRAvatar::initBroadRay() {//path?
    VRGeometryPtr geo = VRGeometry::create("av_broadray");
    //geo->setMesh(VRSceneLoader::get()->loadWRL("mod/flystick/fly2_w_ray.wrl"));

    return geo;
}

void VRAvatar::addAll() {
    map<string, VRObjectPtr>::iterator itr = avatars.begin();
    for(;itr != avatars.end();itr++) deviceRoot->addChild(itr->second);
}

void VRAvatar::hideAll() {
    map<string, VRObjectPtr>::iterator itr = avatars.begin();
    for(;itr != avatars.end();itr++) itr->second->hide();
}

void VRAvatar::addAvatar(VRObjectPtr geo) {
    deviceRoot->addChild(geo);
}

VRAvatar::VRAvatar(string name) {
    deviceRoot = VRTransform::create(name + "_beacon");
    deviceRoot->setPersistency(0);
    tmpContainer = VRTransform::create(name + "_tmp_beacon");
    tmpContainer->setPersistency(0);

    avatars["ray"] = initRay();
    //avatars["cone"] = initCone();
    //avatars["broadray"] = initBroadRay();

    addAll();
    hideAll();
}

VRAvatar::~VRAvatar() {}

void VRAvatar::enableAvatar(string avatar) { if (avatars.count(avatar)) avatars[avatar]->show(); }
void VRAvatar::disableAvatar(string avatar) { if (avatars.count(avatar)) avatars[avatar]->hide(); }

VRTransformPtr VRAvatar::getBeacon() { return deviceRoot; }
VRTransformPtr VRAvatar::editBeacon() { return tmpContainer; }
void VRAvatar::setBeacon(VRTransformPtr b) {
    deviceRoot = b;
    for (auto a : avatars) a.second->switchParent(b);
}

void VRAvatar::updateBeacon() {
    deviceRoot->setMatrix(tmpContainer->getMatrix()); // TODO: may need world matrix here
}

OSG_END_NAMESPACE;
