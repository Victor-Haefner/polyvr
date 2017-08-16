#include "VRDistrict.h"

#include "VRBuilding.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"
#include "core/objects/material/VRShader.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/toString.h"
#include "core/math/triangulator.h"
#include "core/scene/VRSceneManager.h"

using namespace OSG;

VRDistrict::VRDistrict() : VRObject("District") {}
VRDistrict::~VRDistrict() {}

VRDistrictPtr VRDistrict::create() {
    auto d = VRDistrictPtr( new VRDistrict() );
    d->init();
    return d;
}

void VRDistrict::init() {
    b_mat = VRMaterial::create("Buildings");
    b_mat->setTexture("world/textures/Buildings.png", false);
    b_mat->setAmbient(Color3f(0.7, 0.7, 0.7)); //light reflection in all directions
    b_mat->setDiffuse(Color3f(1.0, 1.0, 1.0)); //light from ambient (without lightsource)
    b_mat->setSpecular(Color3f(0.2, 0.2, 0.2)); //light reflection in camera direction

    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    b_mat->readVertexShader(wdir+"/shader/TexturePhong/phong.vp");
    b_mat->readFragmentShader(wdir+"/shader/TexturePhong/phong_building.fp"); //Fragment Shader
    b_mat->setMagMinFilter(GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, 0);

    facades = VRGeometry::create("facades");
    roofs = VRGeometry::create("roofs");
    addChild(facades);
    addChild(roofs);

    facades->setMaterial(b_mat);
    roofs->setMaterial(b_mat);
}

void VRDistrict::addBuilding( VRPolygon p ) {
    if (p.size() < 3) return;

    auto b = VRBuilding::create();
    auto walls = b->addFloor(p, 4);
    auto roof = b->addRoof(p);

    facades->merge(walls);
    roofs->merge(roof);
}

