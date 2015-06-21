#include "VRCamera.h"
#include "core/utils/toString.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/gui/VRGuiManager.h"
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGMultiPassMaterial.h>
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRMaterial* getCamGeoMat() {
    VRMaterial* mat = VRMaterial::get("cam_geo_mat");
    mat->setDiffuse(Color3f(0.9, 0.9, 0.9));
    mat->setTransparency(0.3);
    mat->setLit(false);
    return mat;
}

VRCamera::VRCamera(string name) : VRTransform(name) {
    parallaxD = 2;
    nearClipPlaneCoeff = 0.1;
    //farClipPlaneCoeff = 250000;
    farClipPlaneCoeff = 250;
    cam_invert_z = true;

    cam = PerspectiveCamera::create();
    cam->setBeacon(getNode());
    cam->setFov(osgDegree2Rad(60));
    cam->setNear(parallaxD* nearClipPlaneCoeff);
    cam->setFar(parallaxD* farClipPlaneCoeff);

    type = "Camera";
    doAcceptRoot = true;
    camGeo = 0;

    TransformRecPtr trans = Transform::create();
    NodeRecPtr t = makeNodeFor(trans);
    trans->editMatrix().setTranslate(Vec3f(0,0,0.25));
    GeometryRecPtr camGeo_ = makeBoxGeo(0.2, 0.2, 0.25, 1, 1, 1);
    GeometryRecPtr camGeo2_ = makeCylinderGeo(0.2, 0.07, 16, 1, 1, 1);
    camGeo = makeNodeFor(camGeo_);
    NodeRecPtr camGeo2 = makeNodeFor(camGeo2_);
    camGeo->setTravMask(0);
    camGeo_->setMaterial(getCamGeoMat()->getMaterial());
    camGeo2_->setMaterial(getCamGeoMat()->getMaterial());
    addChild(t);
    t->addChild(camGeo);
    TransformRecPtr trans2 = Transform::create();
    NodeRecPtr t2 = makeNodeFor(trans2);
    trans2->editMatrix().setTranslate(Vec3f(0,0,-0.15));
    trans2->editMatrix().setRotate(Quaternion(Vec3f(1,0,0), Pi*0.5));
    camGeo->addChild(t2);
    t2->addChild(camGeo2);

    getAll().push_back(this);
    VRGuiManager::broadcast("camera_added");
}

VRCamera::~VRCamera() {
    getAll().remove(this);
    VRGuiManager::broadcast("camera_added");
    cam = 0;
}

void VRCamera::activate() {
    cout << "VRCamera::activate " << camID << endl;
    auto scene = VRSceneManager::getCurrent();
    if (scene) scene->setActiveCamera(getName());
    VRGuiManager::broadcast("camera_added");
}

void VRCamera::showCamGeo(bool b) {
    if (b) camGeo->setTravMask(0xffffffff);
    else camGeo->setTravMask(0);
}

list<VRCamera*>& VRCamera::getAll() {
    static list<VRCamera*> objs;
    return objs;
}

PerspectiveCameraRecPtr VRCamera::getCam() { return cam; }

void VRCamera::setAcceptRoot(bool b) { doAcceptRoot = b; }
bool VRCamera::getAcceptRoot() { return doAcceptRoot; }
float VRCamera::getAspect() { return cam->getAspect(); }
float VRCamera::getFov() { return cam->getFov(); }
float VRCamera::getNear() { return nearClipPlaneCoeff; }
float VRCamera::getFar() { return farClipPlaneCoeff; }
void VRCamera::setAspect(float a) { cam->setAspect(a); }
void VRCamera::setFov(float f) { cam->setFov(f); }
void VRCamera::setNear(float a) { nearClipPlaneCoeff = a; cam->setNear(a); }
void VRCamera::setFar(float f) { farClipPlaneCoeff = f; cam->setFar(f); }
void VRCamera::setProjection(string p) {
    if (p == "perspective"); // TODO
    if (p == "orthographic"); // TODO
}

vector<string> VRCamera::getProjectionTypes() {
    vector<string> proj;
    proj.push_back("perspective");
    proj.push_back("orthographic");
    return proj;
}

void VRCamera::saveContent(xmlpp::Element* e) {
    VRTransform::saveContent(e);
    e->set_attribute("accept_root", toString(doAcceptRoot));
    e->set_attribute("near", toString(nearClipPlaneCoeff));
    e->set_attribute("far", toString(farClipPlaneCoeff));
    e->set_attribute("aspect", toString(getAspect()));
    e->set_attribute("fov", toString(getFov()));
}

void VRCamera::loadContent(xmlpp::Element* e) {
    VRTransform::loadContent(e);
    if (e->get_attribute("accept_root")) doAcceptRoot = toBool(e->get_attribute("accept_root")->get_value());
    if (e->get_attribute("near")) setNear( toFloat(e->get_attribute("near")->get_value()) );
    if (e->get_attribute("far")) setFar( toFloat(e->get_attribute("far")->get_value()) );
    if (e->get_attribute("aspect")) setAspect( toFloat(e->get_attribute("aspect")->get_value()) );
    if (e->get_attribute("fov")) setFov( toFloat(e->get_attribute("fov")->get_value()) );
}

OSG_END_NAMESPACE;
