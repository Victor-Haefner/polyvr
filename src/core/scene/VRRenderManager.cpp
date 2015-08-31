#include "VRRenderManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRLight.h"

#include <OpenSG/OSGRenderAction.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRRenderManager::VRRenderManager() {
    update();

    root = new VRObject("Root");
    root_def_shading = new VRObject("Deffered shading root");
    root_ssao = new VRObject("SSAO root");
    root_system = new VRObject("System root");

    root_system->addChild(root_ssao);
    root_ssao->addChild(root_def_shading);
    root_def_shading->addChild(root->getNode());

    initDeferredShading(root_def_shading);
    initSSAO(root_ssao);
    setDefferedShading(false);
    setSSAO(false);



    store("frustum_culling", &frustumCulling);
    store("occlusion_culling", &occlusionCulling);
    store("two_sided", &twoSided);
    store("deferred_rendering", &deferredRendering);
    store("ssao", &ssao);
}

VRRenderManager::~VRRenderManager() {}

void VRRenderManager::update() {
    RenderActionRefPtr ract = VRSetupManager::getCurrent()->getRenderAction();

    ract->setFrustumCulling(frustumCulling);
    ract->setOcclusionCulling(occlusionCulling);
    ract->setCorrectTwoSidedLighting(twoSided);
    ract->setZWriteTrans(true); // enables the zbuffer for transparent objects
}

VRLight* VRRenderManager::addLight(string name) {
    VRLight* l = new VRLight(name);
    light_map[l->getID()] = l;
    addDSLight(l);
    return l;
}

VRLight* VRRenderManager::getLight(int ID) {
    return light_map[ID];
}

//void VRRenderManager::setSSAO(bool b) { ssao = b; update(); }
//bool VRRenderManager::getSSAO() { return ssao; }

void VRRenderManager::setFrustumCulling(bool b) { frustumCulling = b; update(); }
bool VRRenderManager::getFrustumCulling() { return frustumCulling; }

void VRRenderManager::setOcclusionCulling(bool b) { occlusionCulling = b; update(); }
bool VRRenderManager::getOcclusionCulling() { return occlusionCulling; }

void VRRenderManager::setTwoSided(bool b) { twoSided = b; update(); }
bool VRRenderManager::getTwoSided() { return twoSided; }


OSG_END_NAMESPACE;
