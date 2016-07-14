#include "VRHMDDistortion.h"

#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGNode.h>

#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRHMDDistortion::VRHMDDistortion() {}
VRHMDDistortion::~VRHMDDistortion() {}

void VRHMDDistortion::setHMDDparams(float radius) {
    if (!hmdd_mat) return;
    hmdd_mat->setShaderParameter<float>("uRadius", radius);
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
    setHMDDparams(0.8);
}

OSG_END_NAMESPACE;
