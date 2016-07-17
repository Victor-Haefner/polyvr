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
#include "VRDefShading.h"
#include "VRSSAO.h"
#include "VRHMDDistortion.h"

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
    root_system = VRObject::create("System root");
    root_post_processing = VRObject::create("Post processing root");
    root_def_shading = VRObject::create("Deffered shading root");

    root_system->addChild(root_post_processing);
    root_post_processing->addChild(root_def_shading);
    root_def_shading->addChild(root->getNode());

    auto ssao_mat = setupRenderLayer("ssao", root_def_shading);
    auto calib_mat = setupRenderLayer("calibration", root_post_processing);
    auto hmdd_mat = setupRenderLayer("hmdd", root_post_processing);
    //auto metaball_mat = setupRenderLayer("metaball");

    defShading = new VRDefShading();
    ssao = new VRSSAO();
    hmdd = new VRHMDDistortion();
    auto hmddPtr = shared_ptr<VRHMDDistortion>(hmdd);

    root_post_processing->addChild( hmddPtr );
    root_def_shading->switchParent( hmddPtr );

    defShading->initDeferredShading(root_def_shading);
    ssao->initSSAO(ssao_mat);
    hmdd->initHMDD(hmdd_mat);
    hmdd_mat->setTexture(defShading->getTarget(), 0);
    initCalib(calib_mat);
    setDefferedShading(false);
    setSSAO(false);
    setHMDD(false);

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

VRRenderManager::~VRRenderManager() {
    delete defShading;
    delete ssao;
}

VRMaterialPtr VRRenderManager::setupRenderLayer(string name, VRObjectPtr parent) {
    auto plane = VRGeometry::create(name+"_renderlayer");
    plane->setPrimitive("Plane", "2 2 1 1");
    plane->setVolume(false);
    plane->setMaterial( VRMaterial::create(name+"_mat") );
    parent->addChild(plane);
    renderLayer[name] = plane;
    return plane->getMaterial();
}

void VRRenderManager::initCalib(VRMaterialPtr mat) {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
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

    if (defShading) defShading->setDefferedShading(deferredRendering);
    if (ssao) ssao->setSSAOparams(ssao_radius, ssao_kernel, ssao_noise);

    for (auto m : VRMaterial::materials) {
        auto mat = m.second.lock();
        if (!mat) continue;
        bool b = ( (ssao && do_ssao) || (defShading && deferredRendering) );
        mat->setDeffered(b);
    }

    // update shader code
    if (defShading) defShading->reload();
    if (do_hmdd && hmdd) hmdd->reload();
    if (hmdd) hmdd->setActive(do_hmdd);

    // update render layer visibility
    if (renderLayer.count("ssao")) renderLayer["ssao"]->setVisible(do_ssao);
    if (renderLayer.count("calibration")) renderLayer["calibration"]->setVisible(calib);
    if (renderLayer.count("hmdd")) renderLayer["hmdd"]->setVisible(do_hmdd);

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
    light_map[l->getID()] = l;
    if (defShading) defShading->addDSLight(l);

    auto setup = VRSetupManager::getCurrent();
    if (!setup) return;
    for (auto v : setup->getViews()) v->addLight(l);
}

VRLightPtr VRRenderManager::getLight(int ID) { return light_map[ID]; }

void VRRenderManager::setFrustumCulling(bool b) { frustumCulling = b; update(); }
bool VRRenderManager::getFrustumCulling() { return frustumCulling; }

void VRRenderManager::setOcclusionCulling(bool b) { occlusionCulling = b; update(); }
bool VRRenderManager::getOcclusionCulling() { return occlusionCulling; }

void VRRenderManager::setTwoSided(bool b) { twoSided = b; update(); }
bool VRRenderManager::getTwoSided() { return twoSided; }

void VRRenderManager::setDefferedShading(bool b) { deferredRendering = b; update(); }
//bool VRRenderManager::getDefferedShading() { return (deferredRendering && defShading); }
bool VRRenderManager::getDefferedShading() { return deferredRendering; }

void VRRenderManager::setDSCamera(VRCameraPtr cam) {
    if (defShading) defShading->setDSCamera(cam);
    if (hmdd) hmdd->setCamera(cam);

    auto setup = VRSetupManager::getCurrent();
    if (!setup) return;
    for (auto v : setup->getViews()) v->setCamera(cam);
}

void VRRenderManager::setSSAO(bool b) { do_ssao = b; update(); }
//bool VRRenderManager::getSSAO() { return (do_ssao && ssao); }
bool VRRenderManager::getSSAO() { return do_ssao; }
void VRRenderManager::setSSAOradius(float r) { ssao_radius = r; update(); }
void VRRenderManager::setSSAOkernel(int k) { ssao_kernel = k; update(); }
void VRRenderManager::setSSAOnoise(int k) { ssao_noise = k; update(); }
void VRRenderManager::setCalib(bool b) { calib = b; update(); }
void VRRenderManager::setHMDD(bool b) { do_hmdd = b; update(); }
bool VRRenderManager::getHMDD() { return do_hmdd; }

OSG_END_NAMESPACE;
