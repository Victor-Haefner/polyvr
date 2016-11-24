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

class VRDeferredRenderStage {
    private:
        VRObjectPtr top;
        VRObjectPtr bottom;
        VRGeometryPtr layer;
        VRMaterialPtr mat;
        VRDefShading* defRendering = 0;
        bool active = false;

        VRMaterialPtr setupRenderLayer(string name) {
            layer = VRGeometry::create(name+"_renderlayer");
            auto mat = VRMaterial::create(name+"_mat");
            string s = "2"; // TODO: check if layers are not culled in CAVE!
            layer->setPrimitive("Plane", s+" "+s+" 1 1");
            layer->setVolume(false);
            layer->setMaterial( mat );
            layer->setVisible(false);
            mat->setDepthTest(GL_ALWAYS);
            //mat->setSortKey(1000);
            top->addChild(layer);
            return mat;
        }

        void initDeferred() {
            defRendering = new VRDefShading();
            defRendering->initDeferredShading(bottom);
            defRendering->setDeferredShading(false);
        }

    public:
        VRDeferredRenderStage(string name) {
            top = VRObject::create(name+"_DRS_top");
            bottom = VRObject::create(name+"_DRS_bottom");
            top->addChild(bottom);
            mat = setupRenderLayer(name);
        }

        ~VRDeferredRenderStage() {
            delete defRendering;
        }

        VRObjectPtr getTop() { return top; }
        VRObjectPtr getBottom() { return bottom; }
        VRMaterialPtr getMaterial() { return mat; }
        VRGeometryPtr getLayer() { return layer; }
        VRDefShading* getRendering() {
            if (!defRendering) initDeferred();
            return defRendering;
        }
};

VRDeferredRenderStage* VRRenderStudio::addStage(string name, string parent) {
    auto s = new VRDeferredRenderStage(name);
    stages[name] = s;
    auto anchor = root_system;
    if (stages.count(parent)) anchor = stages[parent]->getBottom();
    s->getTop()->switchParent( anchor );
    return s;
}

VRRenderStudio::VRRenderStudio(EYE e) {
    eye = e;
    root_system = VRObject::create("System root");
    addStage("shading");
    addStage("blur", "shading");
    addStage("ssao", "blur");
    addStage("marker");
    addStage("calibration");
    addStage("hmdd");
    root_scene = stages["ssao"]->getBottom();
}

VRRenderStudio::~VRRenderStudio() {
    delete ssao;
}

VRRenderStudioPtr VRRenderStudio::create(EYE e) { return VRRenderStudioPtr( new VRRenderStudio(e) ); }

void VRRenderStudio::init(VRObjectPtr root) {
    ssao = new VRSSAO();
    hmdd = new VRHMDDistortion();
    auto hmddPtr = shared_ptr<VRHMDDistortion>(hmdd);

    root_system->addChild( hmddPtr );
    stages["shading"]->getTop()->switchParent( hmddPtr );

    ssao->initSSAO( stages["ssao"]->getMaterial() );
    ssao->initBlur( stages["blur"]->getMaterial() );
    hmdd->initHMDD( stages["hmdd"]->getMaterial() );
    stages["hmdd"]->getMaterial()->setTexture( stages["shading"]->getRendering()->getTarget(), 0 );
    initCalib( stages["calibration"]->getMaterial() );
    initMarker( stages["marker"]->getMaterial() );

    if (root) setScene(root);
    update();
}

void VRRenderStudio::reset() {
    root_scene->clearLinks(); // clear links to current scene root node
    clearLights();
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

void VRRenderStudio::update() {
    auto defShading = stages["shading"]->getRendering();
    auto defSSAO = stages["ssao"]->getRendering();
    auto defBlur = stages["blur"]->getRendering();

    if (!defShading || !defSSAO || !defBlur) return;
    defShading->setDeferredShading(deferredRendering);
    defBlur->setDeferredShading(do_ssao);
    defSSAO->setDeferredShading(do_ssao);
    if (ssao) ssao->setSSAOparams(ssao_radius, ssao_kernel, ssao_noise);

    bool do_deferred = ( (ssao && do_ssao) || (defShading && defBlur && defSSAO && deferredRendering) );
    for (auto m : VRMaterial::materials) {
        auto mat = m.second.lock();
        if (!mat) continue;
        mat->setDeffered(do_deferred);
    }

    // update shader code
    for (auto s : stages) s.second->getRendering()->reload();
    if (do_hmdd && hmdd) hmdd->reload();
    if (hmdd) hmdd->setActive(do_hmdd);

    // update render layer visibility
    if (stages.count("ssao")) stages["ssao"]->getLayer()->setVisible(do_ssao);
    if (stages.count("blur")) stages["blur"]->getLayer()->setVisible(do_ssao);
    if (stages.count("calibration")) stages["calibration"]->getLayer()->setVisible(calib);
    if (stages.count("hmdd")) stages["hmdd"]->getLayer()->setVisible(do_hmdd);
    if (stages.count("marker")) stages["marker"]->getLayer()->setVisible(do_marker);
}

void VRRenderStudio::addLight(VRLightPtr l) {
    light_map[l->getID()] = l;
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->addDSLight(l);
}

VRLightPtr VRRenderStudio::getLight(int ID) { return light_map.count(ID) ? light_map[ID] : 0; }

void VRRenderStudio::updateLight(VRLightPtr l) {
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->updateLight(l);
}

void VRRenderStudio::subLight(VRLightPtr l) {
    if (!l) return;
    if (light_map.count(l->getID())) light_map.erase(l->getID());
    auto defShading = stages["shading"]->getRendering();
    if (defShading) defShading->subLight(l);
}

void VRRenderStudio::clearLights() {
    auto defShading = stages["shading"]->getRendering();
    for (auto li : light_map) {
        if (auto l = li.second) {
            if (defShading) defShading->subLight(l);
        }
    }
    light_map.clear();
}

void VRRenderStudio::setEye(EYE e) {
    eye = e;
    stages["calibration"]->getMaterial()->setShaderParameter<int>("isRightEye", eye);
}

void VRRenderStudio::setCamera(VRCameraPtr cam) {
    auto defShading = stages["shading"]->getRendering();
    auto defSSAO = stages["ssao"]->getRendering();
    auto defBlur = stages["blur"]->getRendering();
    if (defShading) defShading->setDSCamera(cam);
    if (defSSAO) defSSAO->setDSCamera(cam);
    if (defBlur) defBlur->setDSCamera(cam);
    if (hmdd) hmdd->setCamera(cam);
}

void VRRenderStudio::setCamera(ProjectionCameraDecoratorRecPtr cam) {
    auto defShading = stages["shading"]->getRendering();
    auto defSSAO = stages["ssao"]->getRendering();
    auto defBlur = stages["blur"]->getRendering();
    if (defShading) defShading->setDSCamera(cam);
    if (defSSAO) defSSAO->setDSCamera(cam);
    if (defBlur) defBlur->setDSCamera(cam);
    if (hmdd) hmdd->setCamera(cam);
}

void VRRenderStudio::setBackground(BackgroundRecPtr bg) {
    //if (defShading) defShading->setBackground(bg);
    //if (defSSAO) defSSAO->setBackground(bg);
    //if (defBlur) defBlur->setBackground(bg);
    if (hmdd) hmdd->setBackground(bg);
}

void VRRenderStudio::setScene(VRObjectPtr r) {
    if (!root_scene || !r) return;
    root_scene->clearLinks(); // clear links to current scene root node
    root_scene->addLink( r );
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

void VRRenderStudio::setHMDDeye(float e) { hmdd->setHMDDparams(e); }

OSG_END_NAMESPACE;
