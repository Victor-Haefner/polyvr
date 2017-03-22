#include "VRFXAA.h"

#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGNode.h>

#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRFXAA::VRFXAA() : VRStage("hmdd") {}
VRFXAA::~VRFXAA() {}

void VRFXAA::setFXAAparams(float eye) {
    if (!fxaa_mat) return;
    fxaa_mat->setShaderParameter<float>("eye", eye);
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
    fxaa_mat->setShaderParameter<int>("texBufDiff2", 0);
    setFXAAparams(0.8);

    setTarget(fxaa_mat, 0);
}

void VRFXAA::reload() {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    fxaa_mat->readVertexShader(shdrDir + "FXAA.vp.glsl");
    fxaa_mat->readFragmentShader(shdrDir + "FXAA.fp.glsl");
}

OSG_END_NAMESPACE;
