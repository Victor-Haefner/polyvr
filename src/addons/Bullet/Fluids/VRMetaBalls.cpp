#include "VRMetaBalls.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRStage.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;

VRMetaBalls::VRMetaBalls(string name) : VRObject(name) {
    stage = VRStage::create(name+"_stage");
    VRObject::addChild(stage);

    auto plane = VRGeometry::create(name+"_renderlayer");
    plane->setPrimitive("Plane", "2 2 1 1");
    plane->setVolume(false);
    auto mat = VRMaterial::create(name+"_mat");
    plane->setMaterial( mat );
    VRObject::addChild(plane);

    auto cam = VRSceneManager::getCurrent()->getActiveCamera();
    stage->setTarget(mat);
    stage->setCamera(cam);
}

VRMetaBallsPtr VRMetaBalls::create(string name) { return VRMetaBallsPtr(new VRMetaBalls(name) ); }

void VRMetaBalls::addChild(VRObjectPtr child, bool osg, int place) { stage->addChild(child, osg, place); }
