#include "VRRenderManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRLight.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRStage.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#include "VRRenderStudio.h"

#include <OpenSG/OSGRenderAction.h>

using namespace OSG;


VRRenderManager::VRRenderManager() {
    root = VRObject::create("Root");

    update();

    setStorageType("Rendering");
    store("frustum_culling", &frustumCulling);
    store("occlusion_culling", &occlusionCulling);
    store("two_sided", &twoSided);
    store("deferred_rendering", &deferredRendering);
    store("ssao", &do_ssao);
    store("hmdd", &do_hmdd);
    store("marker", &do_marker);
    store("fxaa", &do_fxaa);
    store("ssao_kernel", &ssao_kernel);
    store("ssao_radius", &ssao_radius);
    store("ssao_noise", &ssao_noise);
    store("fogParams", &fogParams);
    store("fogColor", &fogColor);

    setMultisampling(true);
}

VRRenderManager::~VRRenderManager() {}

void VRRenderManager::setFogParams(Color4f fp, Color4f fc) {
    for (auto r : getRenderings()) r->setFogParams(fp, fc);
}

void VRRenderManager::update() {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    RenderActionRefPtr ract = setup->getRenderAction();

    ract->setFrustumCulling(frustumCulling);
    ract->setOcclusionCulling(occlusionCulling);
    ract->setCorrectTwoSidedLighting(twoSided);
    //ract->setSortTrans(true); // renders transparent objects from back to front
    ract->setZWriteTrans(true); // enables the zbuffer for transparent objects

    for (auto v : setup->getViews()) {
        auto rendering = v->getRenderingL();
        if (rendering) {
            rendering->setDefferedShading(deferredRendering);
            rendering->setSSAO(do_ssao);
            rendering->setSSAOradius(ssao_radius);
            rendering->setSSAOkernel(ssao_kernel);
            rendering->setSSAOnoise(ssao_noise);
            rendering->setCalib(calib);
            rendering->setHMDD(do_hmdd);
            rendering->setMarker(do_marker);
            rendering->setFXAA(do_fxaa);
        }

        rendering = v->getRenderingR();
        if (rendering) {
            rendering->setDefferedShading(deferredRendering);
            rendering->setSSAO(do_ssao);
            rendering->setSSAOradius(ssao_radius);
            rendering->setSSAOkernel(ssao_kernel);
            rendering->setSSAOnoise(ssao_noise);
            rendering->setCalib(calib);
            rendering->setHMDD(do_hmdd);
            rendering->setMarker(do_marker);
            rendering->setFXAA(do_fxaa);
        }
    }
}

void VRRenderManager::addLight(VRLightPtr l) {
    l->setDeferred(deferredRendering);
    for (auto r : getRenderings()) r->addLight(l);
}

void VRRenderManager::subLight(int ID) {
    for (auto r : getRenderings()) r->subLight(ID);
}

void VRRenderManager::updateLight(VRLightPtr l) {
    for (auto r : getRenderings()) r->updateLight(l);
}

void VRRenderManager::setDSCamera(VRCameraPtr cam) {
    auto setup = VRSetup::getCurrent();
    if (!setup) return;
    for (auto v : setup->getViews()) {
        auto rendering = v->getRenderingL();
        if (rendering) rendering->setCamera(cam->getCam());
        rendering = v->getRenderingR();
        if (rendering) rendering->setCamera(cam->getCam());
    }
}

void VRRenderManager::setFrustumCulling(bool b) { frustumCulling = b; update(); }
void VRRenderManager::setOcclusionCulling(bool b) { occlusionCulling = b; update(); }
void VRRenderManager::setTwoSided(bool b) { twoSided = b; update(); }
bool VRRenderManager::getFrustumCulling() { return frustumCulling; }
bool VRRenderManager::getOcclusionCulling() { return occlusionCulling; }
bool VRRenderManager::getTwoSided() { return twoSided; }

bool VRRenderManager::getDefferedShading() { return deferredRendering; }
bool VRRenderManager::getSSAO() { return do_ssao; }
bool VRRenderManager::getHMDD() { return do_hmdd; }
bool VRRenderManager::getMarker() { return do_marker; }
bool VRRenderManager::getCalib() { return calib; }
bool VRRenderManager::getFXAA() { return do_fxaa; }

vector<VRRenderStudioPtr> VRRenderManager::getRenderings() {
    vector<VRRenderStudioPtr> res;
    auto setup = VRSetup::getCurrent();
    if (!setup) return res;
    for (auto v : setup->getViews()) {
        if (auto r = v->getRenderingL()) res.push_back(r);
        if (auto r = v->getRenderingR()) res.push_back(r);
    }
    return res;
}

void VRRenderManager::setStageTexture(string name, VRTexturePtr tex, int unit, int mag, int min) {
    for (auto r : getRenderings()) r->setStageTexture(name, tex, unit, mag, min);
}

void VRRenderManager::addStage(string name, string parent) {
    for (auto r : getRenderings()) r->addStage(name, parent);
}

void VRRenderManager::setStageActive(string name, bool da, bool la) {
    for (auto r : getRenderings()) r->setStageActive(name, da, la);
}

void VRRenderManager::setStageShader(string name, string VPpath, string FPpath, bool doDeferred) {
    for (auto r : getRenderings()) r->setStageShader(name, VPpath, FPpath, doDeferred);
}

int VRRenderManager::addStageBuffer(string name, int pformat, int ptype) {
    int ID = 0;
    for (auto r : getRenderings()) ID = r->addStageBuffer(name, pformat, ptype);
    return ID;
}

void VRRenderManager::setStageParameter(string name, string var, int val) {
    for (auto r : getRenderings()) r->setStageParameter(name, var, val);
}

void VRRenderManager::setDeferredChannel(int c) {
    for (auto r : getRenderings()) r->setDeferredChannel(c);
}

void VRRenderManager::reloadStageShaders() {
    for (auto r : getRenderings()) r->reloadStageShaders();
}

bool VRRenderManager::getMultisampling() { return glMSAA; }
void VRRenderManager::setMultisampling(bool b) {
    glMSAA = b;
#ifdef GL_MULTISAMPLE_ARB
    cout << "VRRenderManager::setMultisampling " << b << endl;
    if (b) glEnable(GL_MULTISAMPLE_ARB);
    else glDisable(GL_MULTISAMPLE_ARB);
#endif
}

void VRRenderManager::setDeferredShading(bool b) { deferredRendering = b; update(); }
void VRRenderManager::setSSAO(bool b) { do_ssao = b; update(); }
void VRRenderManager::setSSAOradius(float r) { ssao_radius = r; update(); }
void VRRenderManager::setSSAOkernel(int k) { ssao_kernel = k; update(); }
void VRRenderManager::setSSAOnoise(int k) { ssao_noise = k; update(); }
void VRRenderManager::setCalib(bool b) { calib = b; update(); }
void VRRenderManager::setHMDD(bool b) { do_hmdd = b; update(); }
void VRRenderManager::setMarker(bool b) { do_marker = b; update(); }
void VRRenderManager::setFXAA(bool b) { do_fxaa = b; update(); }

string glParam(GLenum e) {
    const char* s = (const char*)glGetString(e);
    return s ? string(s) : "";
}

int VRRenderManager::getGLSLVersion() {
    string sV = glParam(GL_SHADING_LANGUAGE_VERSION);
    auto prts = splitString(sV, '.');
    if (prts.size() < 2) return 0;
    int i1 = toInt(prts[0]);
    int i2 = toInt(prts[1]);
    if (i2 < 10) i2 *= 10;
    //cout << "getGLSLVersion " << sV << "  ->  " << Vec2i(i1,i2) << "  -> " << i1*100 + i2 << endl;
    return i1*100 + i2;
}

string VRRenderManager::getGLVendor() { return glParam(GL_VENDOR); }
string VRRenderManager::getGLVersion() { return glParam(GL_VERSION); }
bool VRRenderManager::hasGeomShader() { int v = getGLSLVersion(); return (v >= 150); }
bool VRRenderManager::hasTessShader() { int v = getGLSLVersion(); return (v >= 400); }





