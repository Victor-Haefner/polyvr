#include "VRFXAA.h"

#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRFXAA::VRFXAA() : VRStage("hmdd") {}
VRFXAA::~VRFXAA() {}

void VRFXAA::setFXAAparams() {
    if (!fxaa_mat) return;
}

void VRFXAA::initFXAA(VRMaterialPtr mat) {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    fxaa_mat = mat;
    fxaa_mat->setLit(false);
    fxaa_mat->readVertexShader(shdrDir + "FXAA.vp.glsl");
    fxaa_mat->readFragmentShader(shdrDir + "FXAA.fp.glsl");
    fxaa_mat->setShaderParameter<int>("texBufPos", 0);
    fxaa_mat->setShaderParameter<int>("texBufNorm", 1);
    fxaa_mat->setShaderParameter<int>("texBufDiff", 2);
    fxaa_mat->setShaderParameter<int>("tex", 0);
    setTarget(fxaa_mat, 0);
}

void VRFXAA::reload() {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    fxaa_mat->readVertexShader(shdrDir + "FXAA.vp.glsl");
    fxaa_mat->readFragmentShader(shdrDir + "FXAA.fp.glsl");
}

void VRFXAA::setSize( Vec2i s ) { VRStage::setSize(s*2); } // TODO: pass parameter from gui!

OSG_END_NAMESPACE;
