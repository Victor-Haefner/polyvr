#include "VRRenderManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRLight.h"
#include "VRDefShading.h"

#include <OpenSG/OSGRenderAction.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRRenderManager::VRRenderManager() {
    defShading = new VRDefShading();

    update();

    root = VRObject::create("Root");
    root_def_shading = VRObject::create("Deffered shading root");
    root_ssao = VRObject::create("SSAO root");
    root_system = VRObject::create("System root");

    root_system->addChild(root_ssao);
    root_ssao->addChild(root_def_shading);
    root_def_shading->addChild(root->getNode());

    defShading->initDeferredShading(root_def_shading);
    defShading->initSSAO(root_ssao);
    setDefferedShading(false);
    setSSAO(false);



    store("frustum_culling", &frustumCulling);
    store("occlusion_culling", &occlusionCulling);
    store("two_sided", &twoSided);
    store("deferred_rendering", &deferredRendering);
    store("ssao", &ssao);
}

VRRenderManager::~VRRenderManager() {
    delete defShading;
}

void VRRenderManager::update() {
    RenderActionRefPtr ract = VRSetupManager::getCurrent()->getRenderAction();

    ract->setFrustumCulling(frustumCulling);
    ract->setOcclusionCulling(occlusionCulling);
    ract->setCorrectTwoSidedLighting(twoSided);
    ract->setZWriteTrans(true); // enables the zbuffer for transparent objects

    defShading->setDefferedShading(deferredRendering);
    defShading->setSSAO(ssao);
}

VRLightPtr VRRenderManager::addLight(string name) {
    VRLightPtr l = VRLight::create(name);
    light_map[l->getID()] = l;
    defShading->addDSLight(l);
    return l;
}

VRLightPtr VRRenderManager::getLight(int ID) {
    return light_map[ID];
}

void VRRenderManager::setFrustumCulling(bool b) { frustumCulling = b; update(); }
bool VRRenderManager::getFrustumCulling() { return frustumCulling; }

void VRRenderManager::setOcclusionCulling(bool b) { occlusionCulling = b; update(); }
bool VRRenderManager::getOcclusionCulling() { return occlusionCulling; }

void VRRenderManager::setTwoSided(bool b) { twoSided = b; update(); }
bool VRRenderManager::getTwoSided() { return twoSided; }

void VRRenderManager::setDSCamera(VRCameraPtr cam) { defShading->setDSCamera(cam); }
void VRRenderManager::setDefferedShading(bool b) { deferredRendering = b; update(); }
bool VRRenderManager::getDefferedShading() { return deferredRendering; }

void VRRenderManager::setSSAO(bool b) { ssao = b; update(); }
bool VRRenderManager::getSSAO() { return ssao; }


OSG_END_NAMESPACE;
