#include "VRMetaBalls.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRStage.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/VRCamera.h"
#include "core/objects/OSGCamera.h"
#include "core/scene/VRScene.h"

using namespace OSG;

VRMetaBalls::VRMetaBalls(string name) : VRObject(name) {
    type = "MetaBalls";
}

void VRMetaBalls::init() {
    stage = VRStage::create(name+"_stage");
    dstage = VRStage::create(name+"_depth_stage");
    VRObject::addChild(stage);
    VRObject::addChild(dstage);

    mat = VRMaterial::create(name+"_mat");
    auto plane = VRGeometry::create(name+"_renderlayer");
    plane->setPrimitive("Plane 2 2 1 1");
    plane->setVolumeCheck(false);
    plane->setMaterial( mat );
    VRObject::addChild(plane);

    depth_mat = VRMaterial::create(name+"_depth_mat");
    dstage->addChild(depth_mat);

    auto cam = VRScene::getCurrent()->getActiveCamera();
    stage->setTarget(mat, 0);
    dstage->setTarget(mat, 1);
    stage->setCamera(cam->getCam());
    dstage->setCamera(cam->getCam());

    mat->setShaderParameter<int>("texBufPos", 0);
    mat->setShaderParameter<int>("texBufNorm", 1);
    mat->setShaderParameter<int>("texBufDiff", 2);
    mat->setShaderParameter<int>("texWater", 0);
    mat->setShaderParameter<int>("texDepth", 1);
}

VRMetaBallsPtr VRMetaBalls::create(string name) {
    auto mb = VRMetaBallsPtr(new VRMetaBalls(name) );
    mb->init();
    return mb;
}

void VRMetaBalls::addChild(VRObjectPtr child, bool osg, int place) {
    stage->addChild(child, osg, place);
    depth_mat->addLink(child);
}

VRMaterialPtr VRMetaBalls::getMaterial() { return mat; }
VRMaterialPtr VRMetaBalls::getDepthMaterial() { return depth_mat; }



