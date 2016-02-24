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

VRMaterialPtr getCamGeoMat() {
    VRMaterialPtr mat = VRMaterial::get("cam_geo_mat");
    mat->setDiffuse(Color3f(0.9, 0.9, 0.9));
    mat->setTransparency(0.3);
    mat->setLit(false);
    return mat;
}

VRCamera::VRCamera(string name) : VRTransform(name) {
    type = "Camera";
    cam_invert_z = true;

    cam = PerspectiveCamera::create();
    cam->setBeacon(getNode());
    setFov(osgDegree2Rad(60));

    store("accept_root", &doAcceptRoot);
    store("near", &nearClipPlaneCoeff);
    store("far", &farClipPlaneCoeff);
    store("aspect", &aspect);
    store("fov", &fov);
    regStorageUpdateFkt( VRFunction<int>::create("camera_update", boost::bind(&VRCamera::setup, this)) );

    // cam geo
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
}

VRCamera::~VRCamera() {
    VRGuiManager::broadcast("camera_added");
}

VRCameraPtr VRCamera::ptr() { return static_pointer_cast<VRCamera>( shared_from_this() ); }
VRCameraPtr VRCamera::create(string name) {
    auto p = shared_ptr<VRCamera>(new VRCamera(name) );
    getAll().push_back( p );
    VRGuiManager::broadcast("camera_added");
    return p;
}

void VRCamera::setup() {
    cout << "VRCamera::setup\n";
    cam->setAspect(aspect);
    cam->setFov(fov);
    cam->setNear(parallaxD * nearClipPlaneCoeff);
    cam->setFar(parallaxD * farClipPlaneCoeff);
}

void VRCamera::activate() {
    auto scene = VRSceneManager::getCurrent();
    if (scene) scene->setActiveCamera(getName());
    VRGuiManager::broadcast("camera_added");
}

void VRCamera::showCamGeo(bool b) {
    if (b) camGeo->setTravMask(0xffffffff);
    else camGeo->setTravMask(0);
}

list<VRCameraWeakPtr>& VRCamera::getAll() {
    static list<VRCameraWeakPtr> objs;
    return objs;
}

PerspectiveCameraRecPtr VRCamera::getCam() { return cam; }

void VRCamera::setAcceptRoot(bool b) { doAcceptRoot = b; }
bool VRCamera::getAcceptRoot() { return doAcceptRoot; }
float VRCamera::getAspect() { return aspect; }
float VRCamera::getFov() { return fov; }
float VRCamera::getNear() { return nearClipPlaneCoeff; }
float VRCamera::getFar() { return farClipPlaneCoeff; }
void VRCamera::setAspect(float a) { aspect = a; setup(); }
void VRCamera::setFov(float f) { fov = f; setup(); }
void VRCamera::setNear(float a) { nearClipPlaneCoeff = a; setup(); }
void VRCamera::setFar(float f) { farClipPlaneCoeff = f; setup(); }
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

void VRCamera::loadContent(xmlpp::Element* e) {
    VRTransform::loadContent(e);
    if (e->get_attribute("accept_root")) doAcceptRoot = toBool(e->get_attribute("accept_root")->get_value());
    if (e->get_attribute("near")) setNear( toFloat(e->get_attribute("near")->get_value()) );
    if (e->get_attribute("far")) setFar( toFloat(e->get_attribute("far")->get_value()) );
    if (e->get_attribute("aspect")) setAspect( toFloat(e->get_attribute("aspect")->get_value()) );
    if (e->get_attribute("fov")) setFov( toFloat(e->get_attribute("fov")->get_value()) );
}

void VRCamera::focus(Vec3f p) {
    setAt(p);
}

void VRCamera::focus(VRTransformPtr t) {
    Vec3f v1,v2,c;
    t->getBoundingBox(v1,v2);
    c = (v1+v2)*0.5;

    Vec3f d = getDir();
    //c = t->getWorldPosition();
    focus(c);

    Vec3f dp = getDir();
    if (dp.length() > 1e-4) d = dp;
    d.normalize();

    // go back or forth to see whole node
    float R = (v2 - v1).length();
    setFrom(c - d*max(R, 0.1f));
}

OSG_END_NAMESPACE;
