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

#include <OpenSG/OSGRenderAction.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

/**

Rendering tree

root_system - Group
|
root_def_shading - Group / DSStage
|___________________________________________________________
|                   |                   |                   |
root - Group        Layer1              Layer2              ...
|
scene


*/

VRRenderManager::VRRenderManager() {
    root = VRObject::create("Root");

    update();

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
        v->setDefferedShading(deferredRendering);
        v->setSSAO(do_ssao);
        v->setSSAOradius(ssao_radius);
        v->setSSAOkernel(ssao_kernel);
        v->setSSAOnoise(ssao_noise);
        v->setCalib(calib);
        v->setHMDD(do_hmdd);
        v->update();
    }
}

void VRRenderManager::addLight(VRLightPtr l) {
    auto setup = VRSetupManager::getCurrent();
    if (!setup) return;
    for (auto v : setup->getViews()) v->addLight(l);
}

//VRLightPtr VRRenderManager::getLight(int ID) { return light_map[ID]; }

void VRRenderManager::setFrustumCulling(bool b) { frustumCulling = b; update(); }
bool VRRenderManager::getFrustumCulling() { return frustumCulling; }

void VRRenderManager::setOcclusionCulling(bool b) { occlusionCulling = b; update(); }
bool VRRenderManager::getOcclusionCulling() { return occlusionCulling; }

void VRRenderManager::setTwoSided(bool b) { twoSided = b; update(); }
bool VRRenderManager::getTwoSided() { return twoSided; }

void VRRenderManager::setDefferedShading(bool b) { deferredRendering = b; update(); }
bool VRRenderManager::getDefferedShading() { return deferredRendering; }

void VRRenderManager::setCamera(VRCameraPtr cam) {
    auto setup = VRSetupManager::getCurrent();
    if (!setup) return;
    for (auto v : setup->getViews()) v->setCamera(cam);
}

void VRRenderManager::setSSAO(bool b) { do_ssao = b; update(); }
bool VRRenderManager::getSSAO() { return do_ssao; }
void VRRenderManager::setSSAOradius(float r) { ssao_radius = r; update(); }
void VRRenderManager::setSSAOkernel(int k) { ssao_kernel = k; update(); }
void VRRenderManager::setSSAOnoise(int k) { ssao_noise = k; update(); }
void VRRenderManager::setCalib(bool b) { calib = b; update(); }
void VRRenderManager::setHMDD(bool b) { do_hmdd = b; update(); }
bool VRRenderManager::getHMDD() { return do_hmdd; }

OSG_END_NAMESPACE;
