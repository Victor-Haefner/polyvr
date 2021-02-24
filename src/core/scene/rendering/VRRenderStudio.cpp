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
#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#ifndef WITHOUT_DEFERRED_RENDERING
#include "VRDefShading.h"
#endif
#include "VRSSAO.h"
#include "VRHMDDistortion.h"
#include "VRFXAA.h"

#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGVisitSubTree.h>

using namespace OSG;
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
    addStage("fog", "shading");
    addStage("blurY", "fog");
    //addStage("blurY", "shading");
    addStage("blurX", "blurY");
    addStage("ssao", "blurX");
    addStage("marker");
    addStage("calibration");
    addStage("hmdd");
    addStage("fxaa");
    root_scene = stages["ssao"]->getBottom();
}

VRRenderStudio::~VRRenderStudio() {}
VRRenderStudioPtr VRRenderStudio::create(EYE e) { return VRRenderStudioPtr( new VRRenderStudio(e) ); }

void VRRenderStudio::addStage(string name, string parent) {
    if (stages.count(name)) return;
    auto s = VRDeferredRenderStage::create( name );
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
#ifndef WITHOUT_DEFERRED_RENDERING
    if (!stages.count(name)) return 0;
    return stages[name]->getRendering()->addBuffer(pformat, ptype);
#else
    return 0;
#endif
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
    stages["shading"]->initDeferred();
    stages["shading"]->getTop()->switchParent( hmdd );

    if (stages.count("fog")) {
        stages["fog"]->initDeferred();
        string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
        auto fogMat = stages["fog"]->getMaterial();
        fogMat->readVertexShader(shdrDir + "fog.vp.glsl");
        fogMat->readFragmentShader(shdrDir + "fog.fp.glsl", true);
        fogMat->setShaderParameter<int>("texBufPos", 0);
        fogMat->setShaderParameter<int>("texBufNorm", 1);
        fogMat->setShaderParameter<int>("texBufDiff", 2);
        fogMat->setShaderParameter<Color4f>("fogParams", fogParams);
        fogMat->setShaderParameter<Color4f>("fogColor", fogColor);
        stages["fog"]->setActive(false, false);
    }

    stages["blurX"]->initDeferred();
    stages["blurY"]->initDeferred();
    stages["ssao"]->initDeferred();
    ssao->initSSAO( stages["ssao"]->getMaterial() );
    ssao->initBlur( stages["blurX"]->getMaterial(), stages["blurY"]->getMaterial() );

    hmdd->initHMDD( stages["hmdd"]->getMaterial() );
    fxaa->initFXAA( stages["fxaa"]->getMaterial() );
#ifndef WITHOUT_DEFERRED_RENDERING
    stages["hmdd"]->getMaterial()->setTexture( stages["shading"]->getRendering()->getTarget(), 0 );
    stages["fxaa"]->getMaterial()->setTexture( stages["shading"]->getRendering()->getTarget(), 0 );
#endif
    initCalib( stages["calibration"]->getMaterial() );
    initMarker( stages["marker"]->getMaterial() );

    stages["calibration"]->getTop()->setVolumeCheck(false);
    stages["marker"]->getTop()->setVolumeCheck(false);

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

#ifndef WITHOUT_DEFERRED_RENDERING
    for (auto m : VRMaterial::materials) {
        if (auto mat = m.second.lock()) mat->setDeferred(deferredRendering);
    }

    // update shader code
    for (auto s : stages) if (auto r = s.second->getRendering()) r->reload();
#endif
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
#ifndef WITHOUT_DEFERRED_RENDERING
    for (auto s : stages) if (auto r = s.second->getRendering()) r->reload();
#endif
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
    stages["shading"]->addLight(l);
}

VRLightPtr VRRenderStudio::getLight(int ID) { return light_map.count(ID) ? light_map[ID].lock() : 0; }

void VRRenderStudio::updateLight(VRLightPtr l) {
#ifndef WITHOUT_DEFERRED_RENDERING
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->updateLight(l);
#endif
}

void VRRenderStudio::subLight(int ID) {
    if (light_map.count(ID)) light_map.erase(ID);
#ifndef WITHOUT_DEFERRED_RENDERING
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->subLight(ID);
#endif
}

void VRRenderStudio::clearLights() {
#ifndef WITHOUT_DEFERRED_RENDERING
    auto defShading = stages["shading"]->getRendering();
    for (auto li : light_map) {
        if (auto l = li.second.lock()) {
            if (defShading) defShading->subLight(l->getID());
        }
    }
#endif
    light_map.clear();
}

void VRRenderStudio::setEye(EYE e) {
    eye = e;
    if (auto s = stages["calibration"]) s->getMaterial()->setShaderParameter<int>("isRightEye", eye);
}

void VRRenderStudio::setFogParams(Color4f fogParams, Color4f fogColor) {
    if (stages.count("fog")) {
        auto fogMat = stages["fog"]->getMaterial();
        fogMat->setShaderParameter<Color4f>("fogParams", fogParams);
        fogMat->setShaderParameter<Color4f>("fogColor", fogColor);
        bool a = fogParams[0] > 0.5;
        stages["fog"]->setActive(a,a);
    }
}

void VRRenderStudio::setCamera(OSGCameraPtr cam) {
    for (auto s : stages) s.second->setCamera(cam);
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

/**
There seams to be an issue with clustering
and creating the link then there is no geometry on the scene
*/

void VRRenderStudio::updateSceneLink() {
    if (!root_scene) return;
    NodeMTRecPtr rsNode = root_scene->getNode()->node;
    if (rsNode->getNChildren() == 0) return;
    NodeMTRecPtr linkNode = rsNode->getChild(0);
    VisitSubTreeMTRecPtr link = dynamic_cast<VisitSubTree*>( linkNode->getCore() );
    if (!link) return;
    NodeMTRecPtr lRoot = link->getSubTreeRoot();
    if (lRoot) link->setSubTreeRoot(lRoot);
}

void VRRenderStudio::setScene(VRObjectPtr r) {
    if (!root_scene || !r) return;
    //cout << "VRRenderStudio::setScene " << this << " r " << r.get() << " Nlinks " << root_scene->getLinks().size() << " rsn: " << root_scene->getName() << endl;
    //cout << "   changelist: " << Thread::getCurrentChangeList() << endl;
    root_scene->clearLinks(); // clear links to current scene root node
    root_scene->addLink( r );
    //root_scene->addChild( r );
}

void VRRenderStudio::resize(Vec2i s) {
    if (hmdd) hmdd->setSize(s);
    if (fxaa) fxaa->setSize(s);
}

VRObjectPtr VRRenderStudio::getSceneRoot() { return root_scene; }
VRObjectPtr VRRenderStudio::getRoot() { return root_system; }
bool VRRenderStudio::getSSAO() { return do_ssao; }
bool VRRenderStudio::getHMDD() { return do_hmdd; }
bool VRRenderStudio::getMarker() { return do_marker; }
bool VRRenderStudio::getFXAA() { return do_fxaa; }
bool VRRenderStudio::getDefferedShading() { return deferredRendering; }

void VRRenderStudio::setDeferredChannel(int c) {
#ifndef WITHOUT_DEFERRED_RENDERING
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->setDeferredChannel(c);
#endif
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

