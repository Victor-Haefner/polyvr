#include "VRRenderStudio.h"
#include "core/setup/VRSetupManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/system/VRSystem.h"
#include "core/objects/VRLight.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRStage.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#include "VRDefShading.h"
#include "VRSSAO.h"
#include "VRHMDDistortion.h"
#include "VRFXAA.h"

#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGProjectionCameraDecorator.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

/**

Rendering stages

root_system - Group
|_____________________________________________________________________________________________________
|                                                          |                    |                     |
root_def_shading - Group / DSStage                         Calib                HMDD                  ...
|
layer_blur - Group / Geometry
|
root_blur - Group / DSStage
|
layer_ssao - Group / Geometry
|
root_scene / root_ssao - Group / DSStage
|
scene

*/

VRRenderStudio::VRRenderStudio(EYE e) {
    eye = e;
    root_system = VRObject::create("System root");
    addStage("shading");
    addStage("blurY", "shading");
    addStage("blurX", "blurY");
    addStage("ssao", "blurX");
    addStage("marker");
    addStage("calibration");
    addStage("hmdd");
    addStage("fxaa");
    root_scene = stages["ssao"]->getBottom();

    //addStage("texturing", "shading");
}

VRRenderStudio::~VRRenderStudio() {}
VRRenderStudioPtr VRRenderStudio::create(EYE e) { return VRRenderStudioPtr( new VRRenderStudio(e) ); }

void VRRenderStudio::addStage(string name, string parent) {
    if (stages.count(name)) return;
    auto s = shared_ptr<VRDeferredRenderStage>( new VRDeferredRenderStage(name) );
    stages[name] = s;
    if (!stages.count(parent)) s->getTop()->switchParent( root_system );
    else {
        auto pstage = stages[parent];
        pstage->insert(s);
        if (pstage->getBottom() == root_scene) root_scene = s->getBottom();
    }
    if (cam) setCamera(cam);
}

void VRRenderStudio::setStageActive(string name, bool da, bool la) {
    if (stages.count(name)) stages[name]->setActive(da, la);
    root_system->setVolumeCheck( false, true );
}

void VRRenderStudio::setStageShader(string name, string VPpath, string FPpath, bool doDeferred) {
    if (stages.count(name)) {
        auto mat = stages[name]->getMaterial();
        mat->setLit(false);

        if ( exists( VPpath ) ) mat->readVertexShader(VPpath);
        else mat->setVertexScript(VPpath);
        if ( exists( FPpath ) ) mat->readFragmentShader(FPpath, doDeferred);
        else mat->setFragmentScript(FPpath, doDeferred);

        if (doDeferred) {
            mat->setShaderParameter<int>("texBufPos", 0);
            mat->setShaderParameter<int>("texBufNorm", 1);
            mat->setShaderParameter<int>("texBufDiff", 2);
        }
    }
}

int VRRenderStudio::addStageBuffer(string name, int pformat, int ptype) {
    if (!stages.count(name)) return 0;
    return stages[name]->getRendering()->addBuffer(pformat, ptype);
}

void VRRenderStudio::setStageParameter(string name, string var, int val) {
    if (stages.count(name)) stages[name]->getMaterial()->setShaderParameter<int>(var, val);
}

void VRRenderStudio::setStageTexture(string name, VRTexturePtr tex, int unit, int mag, int min) {
    if (stages.count(name)) {
        stages[name]->getMaterial()->setTexture(tex, 0, unit);
        stages[name]->getMaterial()->setMagMinFilter(mag, min, unit);
    }
}

void VRRenderStudio::init(VRObjectPtr root) {
    ssao = shared_ptr<VRSSAO>( new VRSSAO() );
    hmdd = shared_ptr<VRHMDDistortion>( new VRHMDDistortion() );
    fxaa = shared_ptr<VRFXAA>( new VRFXAA() );

    root_system->addChild( fxaa );
    fxaa->addChild( hmdd );
    stages["shading"]->getTop()->switchParent( hmdd );

    ssao->initSSAO( stages["ssao"]->getMaterial() );
    ssao->initBlur( stages["blurX"]->getMaterial(), stages["blurY"]->getMaterial() );
    hmdd->initHMDD( stages["hmdd"]->getMaterial() );
    fxaa->initFXAA( stages["fxaa"]->getMaterial() );
    stages["hmdd"]->getMaterial()->setTexture( stages["shading"]->getRendering()->getTarget(), 0 );
    stages["fxaa"]->getMaterial()->setTexture( stages["shading"]->getRendering()->getTarget(), 0 );
    initCalib( stages["calibration"]->getMaterial() );
    initMarker( stages["marker"]->getMaterial() );

    if (root) setScene(root);
    update();
}

void VRRenderStudio::update() {
    stages["shading"]->setActive(deferredRendering, false);
    bool do_ssao = this->do_ssao && deferredRendering;

    stages["blurX"]->setActive(do_ssao, do_ssao);
    stages["blurY"]->setActive(do_ssao, do_ssao);
    stages["ssao"]->setActive(do_ssao, do_ssao);
    if (ssao) ssao->setSSAOparams(ssao_radius, ssao_kernel, ssao_noise);

    for (auto m : VRMaterial::materials) {
        if (auto mat = m.second.lock()) mat->setDeferred(deferredRendering);
    }

    // update shader code
    for (auto s : stages) s.second->getRendering()->reload();
    if (do_hmdd && hmdd) hmdd->reload();
    if (do_fxaa && fxaa) fxaa->reload();

    // update render layer visibility
    stages["calibration"]->setActive(false, calib);
    stages["hmdd"]->setActive(false, do_hmdd);
    stages["fxaa"]->setActive(false, do_fxaa);
    stages["marker"]->setActive(false, do_marker);
    if (hmdd) hmdd->setActive(do_hmdd);
    if (fxaa) fxaa->setActive(do_fxaa);
}

void VRRenderStudio::reset() {
    root_scene->clearLinks(); // clear links to current scene root node
    clearLights();
}

void VRRenderStudio::reloadStageShaders() {
    for (auto s : stages) s.second->getRendering()->reload();
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

void VRRenderStudio::initMarker(VRMaterialPtr mat) {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    mat->setLit(false);
    mat->enableTransparency(true);
    mat->readVertexShader(shdrDir + "Marker.vp.glsl");
    mat->readFragmentShader(shdrDir + "Marker.fp.glsl");
}

void VRRenderStudio::addLight(VRLightPtr l) {
    light_map[l->getID()] = l;
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->addDSLight(l);
}

VRLightPtr VRRenderStudio::getLight(int ID) { return light_map.count(ID) ? light_map[ID].lock() : 0; }

void VRRenderStudio::updateLight(VRLightPtr l) {
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->updateLight(l);
}

void VRRenderStudio::subLight(int ID) {
    if (light_map.count(ID)) light_map.erase(ID);
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->subLight(ID);
}

void VRRenderStudio::clearLights() {
    auto defShading = stages["shading"]->getRendering();
    for (auto li : light_map) {
        if (auto l = li.second.lock()) {
            if (defShading) defShading->subLight(l->getID());
        }
    }
    light_map.clear();
}

void VRRenderStudio::setEye(EYE e) {
    eye = e;
    if (auto s = stages["calibration"]) s->getMaterial()->setShaderParameter<int>("isRightEye", eye);
}

void VRRenderStudio::setCamera(OSGCameraPtr cam) {
    for (auto s : stages) s.second->getRendering()->setDSCamera(cam);
    if (hmdd) hmdd->setCamera(cam);
    if (fxaa) fxaa->setCamera(cam);
    this->cam = cam;
}

void VRRenderStudio::setBackground(BackgroundMTRecPtr bg) {
    //if (defShading) defShading->setBackground(bg);
    //if (defSSAO) defSSAO->setBackground(bg);
    //if (defBlur) defBlur->setBackground(bg);
    if (hmdd) hmdd->setBackground(bg);
    if (fxaa) fxaa->setBackground(bg);
}

void VRRenderStudio::setScene(VRObjectPtr r) {
    if (!root_scene || !r) return;
    root_scene->clearLinks(); // clear links to current scene root node
    root_scene->addLink( r );
}

void VRRenderStudio::resize(Vec2i s) {
    if (hmdd) hmdd->setSize(s);
    if (fxaa) fxaa->setSize(s);
}

VRObjectPtr VRRenderStudio::getRoot() { return root_system; }
bool VRRenderStudio::getSSAO() { return do_ssao; }
bool VRRenderStudio::getHMDD() { return do_hmdd; }
bool VRRenderStudio::getMarker() { return do_marker; }
bool VRRenderStudio::getFXAA() { return do_fxaa; }
bool VRRenderStudio::getDefferedShading() { return deferredRendering; }

void VRRenderStudio::setDeferredChannel(int c) {
    auto defShading = stages["shading"]->getRendering();
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
void VRRenderStudio::setFXAA(bool b) { do_fxaa = b; update(); }

void VRRenderStudio::setHMDDeye(float e) { hmdd->setHMDDparams(e); }

OSG_END_NAMESPACE;
