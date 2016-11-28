#include "VRRenderManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRLight.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRStage.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#include "VRRenderStudio.h"

#include <OpenSG/OSGRenderAction.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRRenderManager::VRRenderManager() {
    root = VRObject::create("Root");

    update();

    setStorageType("Rendering");
    store("frustum_culling", &frustumCulling);
    store("occlusion_culling", &occlusionCulling);
    store("two_sided", &twoSided);
    store("deferred_rendering", &deferredRendering);
    store("ssao", &do_ssao);
    store("hmdd", &do_hmdd);
    store("ssao_kernel", &ssao_kernel);
    store("ssao_radius", &ssao_radius);
    store("ssao_noise", &ssao_noise);
}

VRRenderManager::~VRRenderManager() {}

void VRRenderManager::update() {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    RenderActionRefPtr ract = setup->getRenderAction();

    ract->setFrustumCulling(frustumCulling);
    ract->setOcclusionCulling(occlusionCulling);
    ract->setCorrectTwoSidedLighting(twoSided);
    ract->setZWriteTrans(true); // enables the zbuffer for transparent objects

    for (auto v : setup->getViews()) {
        auto rendering = v->getRenderingL();
        if (rendering) {
            rendering->setDefferedShading(deferredRendering);
            rendering->setSSAO(do_ssao);
            rendering->setSSAOradius(ssao_radius);
            rendering->setSSAOkernel(ssao_kernel);
            rendering->setSSAOnoise(ssao_noise);
            rendering->setCalib(calib);
            rendering->setHMDD(do_hmdd);
            rendering->setMarker(do_marker);
        }

        rendering = v->getRenderingR();
        if (rendering) {
            rendering->setDefferedShading(deferredRendering);
            rendering->setSSAO(do_ssao);
            rendering->setSSAOradius(ssao_radius);
            rendering->setSSAOkernel(ssao_kernel);
            rendering->setSSAOnoise(ssao_noise);
            rendering->setCalib(calib);
            rendering->setHMDD(do_hmdd);
            rendering->setMarker(do_marker);
        }
    }
}

void VRRenderManager::addLight(VRLightPtr l) {
    for (auto r : getRenderings()) r->addLight(l);
}

void VRRenderManager::updateLight(VRLightPtr l) {
    for (auto r : getRenderings()) r->updateLight(l);
}

void VRRenderManager::setDSCamera(VRCameraPtr cam) {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    for (auto v : setup->getViews()) {
        auto rendering = v->getRenderingL();
        if (rendering) rendering->setCamera(cam->getCam());
        rendering = v->getRenderingR();
        if (rendering) rendering->setCamera(cam->getCam());
    }
}

void VRRenderManager::setFrustumCulling(bool b) { frustumCulling = b; update(); }
void VRRenderManager::setOcclusionCulling(bool b) { occlusionCulling = b; update(); }
void VRRenderManager::setTwoSided(bool b) { twoSided = b; update(); }
bool VRRenderManager::getFrustumCulling() { return frustumCulling; }
bool VRRenderManager::getOcclusionCulling() { return occlusionCulling; }
bool VRRenderManager::getTwoSided() { return twoSided; }

bool VRRenderManager::getDefferedShading() { return deferredRendering; }
bool VRRenderManager::getSSAO() { return do_ssao; }
bool VRRenderManager::getHMDD() { return do_hmdd; }
bool VRRenderManager::getMarker() { return do_marker; }
bool VRRenderManager::getCalib() { return calib; }

vector<VRRenderStudioPtr> VRRenderManager::getRenderings() {
    vector<VRRenderStudioPtr> res;
    auto setup = VRSetup::getCurrent();
    if (!setup) return res;
    for (auto v : setup->getViews()) {
        if (auto r = v->getRenderingL()) res.push_back(r);
        if (auto r = v->getRenderingR()) res.push_back(r);
    }
    return res;
}

void VRRenderManager::addStage(string name, string parent) {
    for (auto r : getRenderings()) r->addStage(name, parent);
}

void VRRenderManager::setStageActive(string name, bool da, bool la) {
    for (auto r : getRenderings()) r->setStageActive(name, da, la);
}

void VRRenderManager::setStageShader(string name, string VPpath, string FPpath, bool doDeferred) {
    for (auto r : getRenderings()) r->setStageShader(name, VPpath, FPpath, doDeferred);
}

int VRRenderManager::addStageBuffer(string name, int pformat, int ptype) {
    int ID = 0;
    for (auto r : getRenderings()) ID = r->addStageBuffer(name, pformat, ptype);
    return ID;
}

void VRRenderManager::setStageParameter(string name, string var, int val) {
    for (auto r : getRenderings()) r->setStageParameter(name, var, val);
}

void VRRenderManager::setDeferredChannel(int c) {
    for (auto r : getRenderings()) r->setDeferredChannel(c);
}

void VRRenderManager::setDeferredShading(bool b) { deferredRendering = b; update(); }
void VRRenderManager::setSSAO(bool b) { do_ssao = b; update(); }
void VRRenderManager::setSSAOradius(float r) { ssao_radius = r; update(); }
void VRRenderManager::setSSAOkernel(int k) { ssao_kernel = k; update(); }
void VRRenderManager::setSSAOnoise(int k) { ssao_noise = k; update(); }
void VRRenderManager::setCalib(bool b) { calib = b; update(); }
void VRRenderManager::setHMDD(bool b) { do_hmdd = b; update(); }
void VRRenderManager::setMarker(bool b) { do_marker = b; update(); }

OSG_END_NAMESPACE;
