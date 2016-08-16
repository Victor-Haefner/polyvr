#include "VRRenderManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRLight.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRStage.h"
#include "core/objects/material/VRMaterial.h"
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
    auto setup = VRSetupManager::getCurrent();
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
        }
    }
}

void VRRenderManager::addLight(VRLightPtr l) {
    auto setup = VRSetupManager::getCurrent();
    if (!setup) return;
    for (auto v : setup->getViews()) {
        auto rendering = v->getRenderingL();
        if (rendering) rendering->addLight(l);
        rendering = v->getRenderingR();
        if (rendering) rendering->addLight(l);
    }
}

void VRRenderManager::setDSCamera(VRCameraPtr cam) {
    auto setup = VRSetupManager::getCurrent();
    if (!setup) return;
    for (auto v : setup->getViews()) {
        auto rendering = v->getRenderingL();
        if (rendering) rendering->setCamera(cam);
        rendering = v->getRenderingR();
        if (rendering) rendering->setCamera(cam);
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

void VRRenderManager::setDefferedShading(bool b) { deferredRendering = b; update(); }
void VRRenderManager::setSSAO(bool b) { do_ssao = b; update(); }
void VRRenderManager::setSSAOradius(float r) { ssao_radius = r; update(); }
void VRRenderManager::setSSAOkernel(int k) { ssao_kernel = k; update(); }
void VRRenderManager::setSSAOnoise(int k) { ssao_noise = k; update(); }
void VRRenderManager::setCalib(bool b) { calib = b; update(); }
void VRRenderManager::setHMDD(bool b) { do_hmdd = b; update(); }

OSG_END_NAMESPACE;
