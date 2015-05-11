#include "VRAvatar.h"
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRTransform.h"
#include "core/objects/material/VRMaterial.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


VRObject* VRAvatar::initRay() {
    VRGeometry* ray = new VRGeometry("av_ray");

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

    VRMaterial* mat = VRMaterial::get("yellow_ray");
    mat->setLineWidth(6);
    mat->setDiffuse(Color3f(1,1,0));
    mat->setAmbient(Color3f(1,1,0));
    mat->setSpecular(Color3f(1,1,0));

    ray->create(GL_LINES, pos, norms, inds, texs);
    ray->setMaterial(mat);

    return ray;
}

VRObject* VRAvatar::initCone() {
    VRGeometry* cone = new VRGeometry("av_cone");
    cone->setMesh(makeConeGeo(0.3, 0.03, 32, true, true));
    cone->setFrom(Vec3f(0,0,-0.1));
    cone->setOrientation(Vec3f(1,0,-0.1), Vec3f(0,0,-1));

    return cone;
}

VRObject* VRAvatar::initBroadRay() {//path?
    VRGeometry* geo = new VRGeometry("av_broadray");
    //geo->setMesh(VRSceneLoader::get()->loadWRL("mod/flystick/fly2_w_ray.wrl"));

    return geo;
}

void VRAvatar::addAll() {
    map<string, VRObject*>::iterator itr = avatars.begin();
    for(;itr != avatars.end();itr++) deviceRoot->addChild(itr->second);
}

void VRAvatar::hideAll() {
    map<string, VRObject*>::iterator itr = avatars.begin();
    for(;itr != avatars.end();itr++) itr->second->hide();
}

void VRAvatar::addAvatar(VRObject* geo) {
    deviceRoot->addChild(geo);
}

VRAvatar::VRAvatar(string name) {
    deviceRoot = new VRTransform(name + "_beacon");
    deviceRoot->addAttachment("global", 0);
    tmpContainer = new VRTransform(name + "_tmp_beacon");
    tmpContainer->addAttachment("global", 0);

    avatars["ray"] = initRay();
    //avatars["cone"] = initCone();
    //avatars["broadray"] = initBroadRay();

    addAll();
    hideAll();
}

VRAvatar::~VRAvatar() {
    delete deviceRoot; // also deletes the avatars!
}

void VRAvatar::enableAvatar(string avatar) { if (avatars.count(avatar)) avatars[avatar]->show(); }
void VRAvatar::disableAvatar(string avatar) { if (avatars.count(avatar)) avatars[avatar]->hide(); }

VRTransform* VRAvatar::getBeacon() { return deviceRoot; }
VRTransform* VRAvatar::editBeacon() { return tmpContainer; }
void VRAvatar::setBeacon(VRTransform* b) {
    deviceRoot = b;
    for (auto a : avatars) a.second->switchParent(b);
}

void VRAvatar::updateBeacon() {
    deviceRoot->setMatrix(tmpContainer->getMatrix()); // TODO: may need world matrix here
}

OSG_END_NAMESPACE;
