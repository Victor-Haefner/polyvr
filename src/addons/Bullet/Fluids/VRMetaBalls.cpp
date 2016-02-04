#include "VRMetaBalls.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRStage.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;

VRMetaBalls::VRMetaBalls(string name) : VRObject(name) {
    type = "MetaBalls";
}

void VRMetaBalls::init() {
    stage = VRStage::create(name+"_stage");
    VRObject::addChild(stage);

    mat = VRMaterial::create(name+"_mat");
    auto plane = VRGeometry::create(name+"_renderlayer");
    plane->setPrimitive("Plane", "2 2 1 1");
    plane->setVolume(false);
    plane->setMaterial( mat );
    VRObject::addChild(plane);

    auto cam = VRSceneManager::getCurrent()->getActiveCamera();
    stage->setTarget(mat, 3);
    stage->setCamera(cam);

    mat->setShaderParameter<int>("texBufPos", 0);
    mat->setShaderParameter<int>("texBufNorm", 1);
    mat->setShaderParameter<int>("texBufDiff", 2);
    mat->setShaderParameter<int>("texWater", 3);
}

VRMetaBallsPtr VRMetaBalls::create(string name) {
    auto mb = VRMetaBallsPtr(new VRMetaBalls(name) );
    mb->init();
    return mb;
}

void VRMetaBalls::addChild(VRObjectPtr child, bool osg, int place) { stage->addChild(child, osg, place); }

VRMaterialPtr VRMetaBalls::getMaterial() { return mat; }
