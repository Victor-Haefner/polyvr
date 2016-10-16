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
root_def_shading - Group / DSStage
|___________________________________________________________
|                   |                   |                   |
root - Group        Layer1              Layer2              ...
|
scene


*/

VRRenderStudio::VRRenderStudio(EYE e) {
    eye = e;

    root = VRObject::create("Root");
    root_system = VRObject::create("System root");
    root_post_processing = VRObject::create("Post processing root");
    root_def_shading = VRObject::create("Deffered shading root");
    root_system->addChild(root_post_processing);
    root_post_processing->addChild(root_def_shading);
    root_def_shading->addChild(root);
}

VRRenderStudio::~VRRenderStudio() {
    delete defShading;
    delete ssao;
}

VRRenderStudioPtr VRRenderStudio::create(EYE e) { return VRRenderStudioPtr( new VRRenderStudio(e) ); }

void VRRenderStudio::init(VRObjectPtr root) {
    ssao_mat = setupRenderLayer("ssao", root_def_shading);
    hmdd_mat = setupRenderLayer("hmdd", root_post_processing);
    calib_mat = setupRenderLayer("calibration", root_post_processing);
    marker_mat = setupRenderLayer("marker", root_post_processing);
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
    initMarker(marker_mat);

    if (root) setScene(root);
    update();
}

VRMaterialPtr VRRenderStudio::setupRenderLayer(string name, VRObjectPtr parent) {
    auto plane = VRGeometry::create(name+"_renderlayer");
    plane->setPrimitive("Plane", "2 2 1 1");
    plane->setVolume(false);
    plane->setMaterial( VRMaterial::create(name+"_mat") );
    plane->setVisible(false);
    parent->addChild(plane);
    renderLayer[name] = plane;
    return plane->getMaterial();
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
    if (!defShading) return;
    defShading->setDefferedShading(deferredRendering);
    if (ssao) ssao->setSSAOparams(ssao_radius, ssao_kernel, ssao_noise);

    for (auto m : VRMaterial::materials) {
        auto mat = m.second.lock();
        if (!mat) continue;
        bool b = ( (ssao && do_ssao) || (defShading && deferredRendering) );
        mat->setDeffered(b);
    }

    // update shader code
    defShading->reload();
    if (do_hmdd && hmdd) hmdd->reload();
    if (hmdd) hmdd->setActive(do_hmdd);

    // update render layer visibility
    if (renderLayer.count("ssao")) renderLayer["ssao"]->setVisible(do_ssao);
    if (renderLayer.count("calibration")) renderLayer["calibration"]->setVisible(calib);
    if (renderLayer.count("hmdd")) renderLayer["hmdd"]->setVisible(do_hmdd);
    if (renderLayer.count("marker")) renderLayer["marker"]->setVisible(do_marker);
}

void VRRenderStudio::addLight(VRLightPtr l) {
    light_map[l->getID()] = l;
    if (defShading) defShading->addDSLight(l);
}

VRLightPtr VRRenderStudio::getLight(int ID) { return light_map[ID]; }
void VRRenderStudio::setEye(EYE e) {
    eye = e;
    calib_mat->setShaderParameter<int>("isRightEye", eye);
}

void VRRenderStudio::setCamera(VRCameraPtr cam) {
    if (defShading) defShading->setDSCamera(cam);
    if (hmdd) hmdd->setCamera(cam);
}

void VRRenderStudio::setCamera(ProjectionCameraDecoratorRecPtr cam) {
    if (defShading) defShading->setDSCamera(cam);
    if (hmdd) hmdd->setCamera(cam);
}

void VRRenderStudio::setBackground(BackgroundRecPtr bg) {
    if (hmdd) hmdd->setBackground(bg);
}

void VRRenderStudio::setScene(VRObjectPtr r) {
    if (!root || !r) return;
    root->clearLinks();
    root->addLink( r );
}

void VRRenderStudio::resize(Vec2i s) {
    if (hmdd) hmdd->setSize(s);
}

VRObjectPtr VRRenderStudio::getRoot() { return root_system; }
bool VRRenderStudio::getSSAO() { return do_ssao; }
bool VRRenderStudio::getHMDD() { return do_hmdd; }
bool VRRenderStudio::getMarker() { return do_marker; }
bool VRRenderStudio::getDefferedShading() { return deferredRendering; }

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
