#include "VRRenderManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRLight.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRStage.h"
#include "core/objects/material/VRMaterial.h"
#include "VRDefShading.h"

#include <OpenSG/OSGRenderAction.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRRenderManager::VRRenderManager() {
    defShading = new VRDefShading();

    /*
                                     --- scene
    root_system --- root_def_shading --- root_ssao
    */

    root = VRObject::create("Root");
    root_def_shading = VRObject::create("Deffered shading root");
    root_ssao = VRObject::create("SSAO root");
    root_calib = VRObject::create("Calib root");
    root_system = VRObject::create("System root");

    root_system->addChild(root_calib);
    root_calib->addChild(root_def_shading);
    root_def_shading->addChild(root->getNode());
    root_def_shading->addChild(root_ssao);

    defShading->initDeferredShading(root_def_shading);
    defShading->initSSAO(root_ssao);
    initCalib(root_calib);
    setDefferedShading(false);
    setSSAO(false);

    update();

    store("frustum_culling", &frustumCulling);
    store("occlusion_culling", &occlusionCulling);
    store("two_sided", &twoSided);
    store("deferred_rendering", &deferredRendering);
    store("ssao", &ssao);
    store("ssao_kernel", &ssao_kernel);
    store("ssao_radius", &ssao_radius);
    store("ssao_noise", &ssao_noise);
}

VRRenderManager::~VRRenderManager() {
    delete defShading;
}

void VRRenderManager::initCalib(VRObjectPtr o) {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    auto plane = VRGeometry::create("calib_layer");
    o->addChild(plane);
    plane->setPrimitive("Plane", "2 2 1 1");

    float inf = std::numeric_limits<float>::max();
    BoxVolume &vol = plane->getNode()->editVolume(false);
    vol.setEmpty();
    vol.extendBy(Pnt3f(-inf,-inf,-inf));
    vol.extendBy(Pnt3f(inf,inf,inf));
    vol.setValid(true);
    vol.setStatic(true);

    auto mat = VRMaterial::create("calib");
    plane->setMaterial(mat);

    // ssao material pass
    mat->setLit(false);
    mat->readVertexShader(shdrDir + "Calib.vp.glsl");
    mat->readFragmentShader(shdrDir + "Calib.fp.glsl");
    mat->setShaderParameter<int>("grid", 64);
}

void VRRenderManager::update() {
    auto setup = VRSetupManager::getCurrent();
    if (!setup) return;
    RenderActionRefPtr ract = setup->getRenderAction();

    ract->setFrustumCulling(frustumCulling);
    ract->setOcclusionCulling(occlusionCulling);
    ract->setCorrectTwoSidedLighting(twoSided);
    ract->setZWriteTrans(true); // enables the zbuffer for transparent objects

    defShading->setDefferedShading(deferredRendering);
    root_ssao->setVisible(ssao);
    defShading->setSSAOparams(ssao_radius, ssao_kernel, ssao_noise);

    for (auto m : VRMaterial::materials) {
        auto mat = m.second.lock();
        if (!mat) continue;
        mat->setDeffered(ssao || deferredRendering);
    }

    root_calib->getChild(1)->setVisible(calib);
}

VRLightPtr VRRenderManager::addLight(string name) {
    VRLightPtr l = VRLight::create(name);
    light_map[l->getID()] = l;
    defShading->addDSLight(l);
    return l;
}

VRLightPtr VRRenderManager::getLight(int ID) { return light_map[ID]; }

void VRRenderManager::setFrustumCulling(bool b) { frustumCulling = b; update(); }
bool VRRenderManager::getFrustumCulling() { return frustumCulling; }

void VRRenderManager::setOcclusionCulling(bool b) { occlusionCulling = b; update(); }
bool VRRenderManager::getOcclusionCulling() { return occlusionCulling; }

void VRRenderManager::setTwoSided(bool b) { twoSided = b; update(); }
bool VRRenderManager::getTwoSided() { return twoSided; }

void VRRenderManager::setDefferedShading(bool b) { deferredRendering = b; update(); }
bool VRRenderManager::getDefferedShading() { return deferredRendering; }

void VRRenderManager::setDSCamera(VRCameraPtr cam) { defShading->setDSCamera(cam); }

void VRRenderManager::setSSAO(bool b) { ssao = b; update(); }
bool VRRenderManager::getSSAO() { return ssao; }
void VRRenderManager::setSSAOradius(float r) { ssao_radius = r; update(); }
void VRRenderManager::setSSAOkernel(int k) { ssao_kernel = k; update(); }
void VRRenderManager::setSSAOnoise(int k) { ssao_noise = k; update(); }

void VRRenderManager::setCalib(bool b) { calib = b; update(); }

OSG_END_NAMESPACE;
