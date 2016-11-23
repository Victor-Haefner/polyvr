#include "VRRenderStudio.h"
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
root_post_processing
|_____________________________________________________________________________________________________
|                                                          |                    |                     |
root_def_shading - Group / DSStage                         Calib                HMDD                  ...
|_________________________________________________________________________________
|                                         |                   |                   |
root_def_ssao - Group / DSStage           Layer1              Layer2              ...
|
scene


*/

VRRenderStudio::VRRenderStudio(EYE e) {
    eye = e;

    root_system = VRObject::create("System root");
    root_post_processing = VRObject::create("Post processing root");
    root_def_shading = VRObject::create("Deffered shading root");
    root_def_blur = VRObject::create("blur root");
    root_def_ssao = VRObject::create("ssao root");
    root_system->addChild(root_post_processing);
    root_post_processing->addChild(root_def_shading);
    root_def_shading->addChild(root_def_blur);
    root_def_blur->addChild(root_def_ssao);
}

VRRenderStudio::~VRRenderStudio() {
    delete defShading;
    delete defSSAO;
    delete defBlur;
    delete ssao;
}

VRRenderStudioPtr VRRenderStudio::create(EYE e) { return VRRenderStudioPtr( new VRRenderStudio(e) ); }

void VRRenderStudio::init(VRObjectPtr root) {
    dsProxy_mat1 = setupRenderLayer("dsproxy1", root_def_shading);
    dsProxy_mat2 = setupRenderLayer("dsproxy2", root_def_blur);
    ssao_mat = setupRenderLayer("ssao", root_def_shading);
    blur_mat = setupRenderLayer("blur", root_def_ssao);
    hmdd_mat = setupRenderLayer("hmdd", root_post_processing);
    calib_mat = setupRenderLayer("calibration", root_post_processing);
    marker_mat = setupRenderLayer("marker", root_post_processing);
    //auto metaball_mat = setupRenderLayer("metaball");

    defShading = new VRDefShading();
    defSSAO = new VRDefShading();
    defBlur = new VRDefShading();
    ssao = new VRSSAO();
    hmdd = new VRHMDDistortion();
    auto hmddPtr = shared_ptr<VRHMDDistortion>(hmdd);

    root_post_processing->addChild( hmddPtr );
    root_def_shading->switchParent( hmddPtr );

    defShading->initDeferredShading(root_def_shading);
    defSSAO->initDeferredShading(root_def_ssao);
    defBlur->initDeferredShading(root_def_blur);
    ssao->initSSAO(ssao_mat);
    //ssao->initBlur(blur_mat);
    hmdd->initHMDD(hmdd_mat);
    hmdd_mat->setTexture(defShading->getTarget(), 0);
    initDSProxy(dsProxy_mat1);
    initDSProxy(dsProxy_mat2);
    initCalib(calib_mat);
    initMarker(marker_mat);

    if (root) setScene(root);
    update();
}

void VRRenderStudio::reset() {
    root_def_ssao->clearLinks(); // clear links to current scene root node
    clearLights();
}

VRMaterialPtr VRRenderStudio::setupRenderLayer(string name, VRObjectPtr parent) {
    auto plane = VRGeometry::create(name+"_renderlayer");
    auto mat = VRMaterial::create(name+"_mat");
    string s = "2"; // TODO: check if layers are not culled in CAVE!
    plane->setPrimitive("Plane", s+" "+s+" 1 1");
    plane->setVolume(false);
    plane->setMaterial( mat );
    plane->setVisible(false);
    mat->setDepthTest(GL_ALWAYS);
    //mat->setSortKey(1000);
    parent->addChild(plane);
    renderLayer[name] = plane;
    return mat;
}

void VRRenderStudio::initDSProxy(VRMaterialPtr mat) {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    mat->setLit(false);
    mat->readVertexShader(shdrDir + "dsProxy.vp.glsl");
    mat->readFragmentShader(shdrDir + "dsProxy.fp.glsl", true);
    mat->setShaderParameter<int>("texBufPos", 0);
    mat->setShaderParameter<int>("texBufNorm", 1);
    mat->setShaderParameter<int>("texBufDiff", 2);
}

void VRRenderStudio::initCalib(VRMaterialPtr mat) {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    mat->setLit(false);
    mat->readVertexShader(shdrDir + "Calib.vp.glsl");
    mat->readFragmentShader(shdrDir + "Calib.fp.glsl");
    mat->setShaderParameter<int>("grid", 64);
    mat->setShaderParameter<int>("isRightEye", eye);
}

void VRRenderStudio::initMarker(VRMaterialPtr mat) { // TODO
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    mat->setLit(false);
    mat->enableTransparency(true);
    mat->readVertexShader(shdrDir + "Marker.vp.glsl");
    mat->readFragmentShader(shdrDir + "Marker.fp.glsl");
    //mat->setShaderParameter<int>("grid", 64);
    //mat->setShaderParameter<int>("isRightEye", eye);
}

void VRRenderStudio::update() {
    if (!defShading || !defSSAO) return;
    defShading->setDefferedShading(deferredRendering);
    defSSAO->setDefferedShading(deferredRendering);
    //defBlur->setDefferedShading(deferredRendering);
    if (ssao) ssao->setSSAOparams(ssao_radius, ssao_kernel, ssao_noise);

    bool do_deferred = ( (ssao && do_ssao) || (defShading && defSSAO && deferredRendering) );
    for (auto m : VRMaterial::materials) {
        auto mat = m.second.lock();
        if (!mat) continue;
        mat->setDeffered(do_deferred);
    }

    // update shader code
    defShading->reload();
    defSSAO->reload();
    //defBlur->reload();
    if (do_hmdd && hmdd) hmdd->reload();
    if (hmdd) hmdd->setActive(do_hmdd);

    // update render layer visibility
    if (renderLayer.count("ssao")) renderLayer["ssao"]->setVisible(do_ssao);
    if (renderLayer.count("blur")) renderLayer["blur"]->setVisible(do_ssao);
    if (renderLayer.count("dsproxy1")) renderLayer["dsproxy1"]->setVisible(do_deferred && !do_ssao);
    if (renderLayer.count("dsproxy2")) renderLayer["dsproxy2"]->setVisible(do_deferred && !do_ssao);
    if (renderLayer.count("calibration")) renderLayer["calibration"]->setVisible(calib);
    if (renderLayer.count("hmdd")) renderLayer["hmdd"]->setVisible(do_hmdd);
    if (renderLayer.count("marker")) renderLayer["marker"]->setVisible(do_marker);
}

void VRRenderStudio::addLight(VRLightPtr l) {
    light_map[l->getID()] = l;
    if (defShading) defShading->addDSLight(l);
}

VRLightPtr VRRenderStudio::getLight(int ID) { return light_map.count(ID) ? light_map[ID] : 0; }

void VRRenderStudio::updateLight(VRLightPtr l) {
    if (defShading) defShading->updateLight(l);
}

void VRRenderStudio::subLight(VRLightPtr l) {
    if (!l) return;
    if (light_map.count(l->getID())) light_map.erase(l->getID());
    if (defShading) defShading->subLight(l);
}

void VRRenderStudio::clearLights() {
    for (auto li : light_map) {
        if (auto l = li.second) {
            if (defShading) defShading->subLight(l);
        }
    }
    light_map.clear();
}

void VRRenderStudio::setEye(EYE e) {
    eye = e;
    calib_mat->setShaderParameter<int>("isRightEye", eye);
}

void VRRenderStudio::setCamera(VRCameraPtr cam) {
    if (defShading) defShading->setDSCamera(cam);
    if (defSSAO) defSSAO->setDSCamera(cam);
    if (hmdd) hmdd->setCamera(cam);
}

void VRRenderStudio::setCamera(ProjectionCameraDecoratorRecPtr cam) {
    if (defShading) defShading->setDSCamera(cam);
    if (defSSAO) defSSAO->setDSCamera(cam);
    if (hmdd) hmdd->setCamera(cam);
}

void VRRenderStudio::setBackground(BackgroundRecPtr bg) {
    if (hmdd) hmdd->setBackground(bg);
}

void VRRenderStudio::setScene(VRObjectPtr r) {
    if (!root_def_ssao || !r) return;
    root_def_ssao->clearLinks(); // clear links to current scene root node
    root_def_ssao->addLink( r );
}

void VRRenderStudio::resize(Vec2i s) {
    if (hmdd) hmdd->setSize(s);
}

VRObjectPtr VRRenderStudio::getRoot() { return root_system; }
bool VRRenderStudio::getSSAO() { return do_ssao; }
bool VRRenderStudio::getHMDD() { return do_hmdd; }
bool VRRenderStudio::getMarker() { return do_marker; }
bool VRRenderStudio::getDefferedShading() { return deferredRendering; }

void VRRenderStudio::setDeferredChannel(int c) {
    if (defShading) defShading->setDeferredChannel(c);
}

void VRRenderStudio::setDefferedShading(bool b) { deferredRendering = b; update(); }
void VRRenderStudio::setSSAO(bool b) { do_ssao = b; update(); }
void VRRenderStudio::setSSAOradius(float r) { ssao_radius = r; update(); }
void VRRenderStudio::setSSAOkernel(int k) { ssao_kernel = k; update(); }
void VRRenderStudio::setSSAOnoise(int k) { ssao_noise = k; update(); }
void VRRenderStudio::setCalib(bool b) { calib = b; update(); }
void VRRenderStudio::setHMDD(bool b) { do_hmdd = b; update(); }
void VRRenderStudio::setMarker(bool b) { do_marker = b; update(); }

void VRRenderStudio::setHMDDeye(float e) { hmdd->setHMDDparams(e); }

OSG_END_NAMESPACE;
