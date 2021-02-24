#include "VRCamera.h"
#include "OSGCamera.h"
#include "core/utils/toString.h"
#include "core/math/boundingbox.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/OSGMaterial.h"
#include "core/objects/OSGObject.h"
#include "core/scene/VRScene.h"
#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#endif
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGPerspectiveCamera.h>
#include <OpenSG/OSGOrthographicCamera.h>

using namespace OSG;


VRMaterialPtr getCamGeoMat() {
    VRMaterialPtr mat = VRMaterial::get("cam_geo_mat");
    mat->setDiffuse(Color3f(0.9, 0.9, 0.9));
    mat->setTransparency(0.3);
    mat->setLit(false);
    return mat;
}

VRCamera::VRCamera(string name) : VRTransform(name) {
    type = "Camera";
    fov = osgDegree2Rad(60);

    vrSetup = VRObject::create("DeviceBeacons");
    // probably not needed
    //vrSetup->setPersistency(0);

    setup(false);

    store("accept_root", &doAcceptRoot);
    store("near", &nearClipPlaneCoeff);
    store("far", &farClipPlaneCoeff);
    store("aspect", &aspect);
    store("fov", &fov);
    store("orthoSize", &orthoSize);
    store("type", &type);
    regStorageSetupFkt( VRStorageCb::create("camera_update", bind(&VRCamera::setup, this, true, _1)) );
}

VRCamera::~VRCamera() {
#ifndef WITHOUT_GTK
    if (registred) VRGuiManager::broadcast("camera_added");
#endif
}

VRCameraPtr VRCamera::ptr() { return static_pointer_cast<VRCamera>( shared_from_this() ); }

VRCameraPtr VRCamera::create(string name, bool reg) {
    auto p = VRCameraPtr(new VRCamera(name) );
    p->addChild(p->vrSetup);
    p->registred = reg;
    getAll().push_back( p );
#ifndef WITHOUT_GTK
    VRGuiManager::broadcast("camera_added");
#endif
    if (reg) VRScene::getCurrent()->setMActiveCamera(p->getName());
    return p;
}

VRObjectPtr VRCamera::copy(vector<VRObjectPtr> children) {
    VRCameraPtr t = VRCamera::create(getBaseName());
    t->setVisible(isVisible());
    t->setPickable(isPickable());
    t->setEntity(entity);
    t->setMatrix(getMatrix());

    t->setType(getType());
    t->setAspect(getAspect());
    t->setFov(getFov());
    t->setNear(getNear());
    t->setFar(getFar());
    t->setOrthoSize(getOrthoSize());
    return t;
}

void VRCamera::setCam(OSGCameraPtr c) { cam = c; } // warning: setup() will override this!

void VRCamera::setType(int type) { camType = type; setup(); }
int VRCamera::getType() { return camType; }

Matrix VRCamera::getProjectionMatrix(int w, int h) {
    Matrix res;
    cam->cam->getProjection(res, w, h);
    return res;
}

void VRCamera::updateOrthSize() {
    if (camType == ORTHOGRAPHIC) {
        orthoSize = (getAt()-getFrom()).length();
        setup();
    }
}

void VRCamera::setMatrix(Matrix4d m) { VRTransform::setMatrix(m); updateOrthSize(); }
void VRCamera::setAt(Vec3d m) { VRTransform::setAt(m); updateOrthSize(); }
void VRCamera::setFrom(Vec3d m) { VRTransform::setFrom(m); updateOrthSize(); }
VRObjectPtr VRCamera::getSetupNode() { return vrSetup; }

void VRCamera::setup(bool reg, VRStorageContextPtr context) {
    vrSetup->setPersistency(0);

    PerspectiveCameraMTRecPtr pcam;
    OrthographicCameraMTRecPtr ocam;
    if (cam) pcam = dynamic_pointer_cast<PerspectiveCamera>(cam->cam);
    if (cam) ocam = dynamic_pointer_cast<OrthographicCamera>(cam->cam);

    if (!pcam && camType == PERSPECTIVE) {
        //cout << " VRCamera::setup switch to perp, reg: " << reg << endl;
        pcam = PerspectiveCamera::create();
        cam = OSGCamera::create( pcam );
        pcam->setBeacon(getNode()->node);
        if (reg) VRScene::getCurrent()->setActiveCamera(getName());
    }

    if (!ocam && camType == ORTHOGRAPHIC) {
        //cout << " VRCamera::setup switch to orth, reg: " << reg << endl;
        ocam = OrthographicCamera::create();
        cam = OSGCamera::create( ocam );
        ocam->setBeacon(getNode()->node);
        if (reg) {
            VRScene::getCurrent()->setActiveCamera(getName());
            updateOrthSize();
        }
    }

    if (pcam) {
        pcam->setAspect(aspect);
        pcam->setFov(fov);
        pcam->setNear(parallaxD * nearClipPlaneCoeff);
        pcam->setFar(parallaxD * farClipPlaneCoeff);
    }

    if (ocam) {
        ocam->setAspect(aspect);
        ocam->setVerticalSize(orthoSize);
        ocam->setHorizontalSize(aspect*orthoSize);
        ocam->setNear(parallaxD * nearClipPlaneCoeff);
        ocam->setFar(parallaxD * farClipPlaneCoeff);
    }

    vrSetup->setPersistency(0);
}

void VRCamera::activate() {
    auto scene = VRScene::getCurrent();
    if (scene) scene->setActiveCamera(getName());
#ifndef WITHOUT_GTK
    VRGuiManager::broadcast("camera_changed");
#endif
}

void VRCamera::showCamGeo(bool b) {
    if (!camGeo) {
        TransformMTRecPtr trans = Transform::create();
        NodeMTRecPtr t = makeNodeFor(trans);
        trans->editMatrix().setTranslate(Vec3f(0,0,0.25));
        GeometryMTRecPtr camGeo_ = makeBoxGeo(0.2, 0.2, 0.25, 1, 1, 1); //
        GeometryMTRecPtr camGeo2_ = makeCylinderGeo(0.2, 0.07, 16, 1, 1, 1);
        camGeo = OSGObject::create( makeNodeFor(camGeo_) );
        NodeMTRecPtr camGeo2 = makeNodeFor(camGeo2_);
        camGeo->node->setTravMask(0);
        camGeo_->setMaterial(getCamGeoMat()->getMaterial()->mat);
        camGeo2_->setMaterial(getCamGeoMat()->getMaterial()->mat);
        addChild(OSGObject::create(t));
        t->addChild(camGeo->node);
        TransformMTRecPtr trans2 = Transform::create();
        NodeMTRecPtr t2 = makeNodeFor(trans2);
        trans2->editMatrix().setTranslate(Vec3f(0,0,-0.15));
        trans2->editMatrix().setRotate(Quaternion(Vec3f(1,0,0), Pi*0.5));
        camGeo->node->addChild(t2);
        t2->addChild(camGeo2);
    }

    if (b) camGeo->node->setTravMask(0xffffffff);
    else camGeo->node->setTravMask(0);
}

list<VRCameraWeakPtr>& VRCamera::getAll() {
    static list<VRCameraWeakPtr> objs;
    return objs;
}

OSGCameraPtr VRCamera::getCam() { return cam; }

void VRCamera::setAcceptRoot(bool b) { doAcceptRoot = b; }
bool VRCamera::getAcceptRoot() { return doAcceptRoot; }
float VRCamera::getAspect() { return aspect; }
float VRCamera::getFov() { return fov; }
float VRCamera::getNear() { return nearClipPlaneCoeff; }
float VRCamera::getFar() { return farClipPlaneCoeff; }
float VRCamera::getOrthoSize() { return orthoSize; }
void VRCamera::setAspect(float a) { aspect = a; setup(); }
void VRCamera::setFov(float f) { fov = f; setup(); }

void VRCamera::setNear(float a) {
    nearClipPlaneCoeff = a;
    setup();
#ifndef WITHOUT_GTK
    VRGuiManager::broadcast("camera_near_far_changed");
#endif
}

void VRCamera::setFar(float f) {
    farClipPlaneCoeff = f;
    setup();
#ifndef WITHOUT_GTK
    VRGuiManager::broadcast("camera_near_far_changed");
#endif
}

void VRCamera::setOrthoSize(float f) { orthoSize = f; setup(); }
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

void VRCamera::focusPoint(Vec3d p) { setAt(p); }

void VRCamera::focusObject(VRObjectPtr t) {
    auto bb = t->getBoundingbox();
    Vec3d c = bb->center();

    Vec3d d = getDir();
    focusPoint(c);

    Vec3d dp = getDir();
    if (dp.length() > 1e-4) d = dp; // only use new dir if it is valid
    d.normalize();
    //float r = max(bb->radius()*2, 0.1f);
    float r = bb->radius() / tan(fov*0.5);
    setFrom(c - d*r); // go back or forth to see whole node

    //cout << "VRCamera::focus " << t->getName() << " pos " << c << " size " << r << endl;
}

