#include "VRHMDDistortion.h"

#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGNode.h>

#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRHMDDistortion::VRHMDDistortion() : VRStage("hmdd") {}
VRHMDDistortion::~VRHMDDistortion() {}

void VRHMDDistortion::setHMDDparams(float eye) {
    if (!hmdd_mat) return;
    hmdd_mat->setShaderParameter<float>("eye", eye);
}

void VRHMDDistortion::initHMDD(VRMaterialPtr mat) {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    hmdd_mat = mat;

    hmdd_mat->setLit(false);
    hmdd_mat->readVertexShader(shdrDir + "HMDD.vp.glsl");
    hmdd_mat->readFragmentShader(shdrDir + "HMDD.fp.glsl");
    hmdd_mat->setShaderParameter<int>("texBufPos", 0);
    hmdd_mat->setShaderParameter<int>("texBufNorm", 1);
    hmdd_mat->setShaderParameter<int>("texBufDiff", 2);
    hmdd_mat->setShaderParameter<int>("texBufDiff2", 0);
    setHMDDparams(0.8);

    setTarget(hmdd_mat, 0);
}

void VRHMDDistortion::reload() {
    string shdrDir = VRSceneManager::get()->getOriginalWorkdir() + "/shader/DeferredShading/";
    hmdd_mat->readVertexShader(shdrDir + "HMDD.vp.glsl");
    hmdd_mat->readFragmentShader(shdrDir + "HMDD.fp.glsl");
}

OSG_END_NAMESPACE;
