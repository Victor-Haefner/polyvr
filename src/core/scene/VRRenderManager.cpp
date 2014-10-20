#include "VRRenderManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"

#include <OpenSG/OSGRenderAction.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


VRRenderManager::VRRenderManager() {
    frustumCulling = true;
    occlusionCulling = false;
    update();

    store("frustum_culling", &frustumCulling);
    store("occlusion_culling", &occlusionCulling);
}

VRRenderManager::~VRRenderManager() {}

void VRRenderManager::update() {
    RenderActionRefPtr ract = VRSetupManager::get()->getCurrent()->getRenderAction();
    ract->setFrustumCulling(frustumCulling);
    ract->setOcclusionCulling(occlusionCulling);
}

void VRRenderManager::setFrustumCulling(bool b) { frustumCulling = b; update(); }
bool VRRenderManager::getFrustumCulling() { return frustumCulling; }

void VRRenderManager::setOcclusionCulling(bool b) { occlusionCulling = b; update(); }
bool VRRenderManager::getOcclusionCulling() { return occlusionCulling; }


OSG_END_NAMESPACE;
