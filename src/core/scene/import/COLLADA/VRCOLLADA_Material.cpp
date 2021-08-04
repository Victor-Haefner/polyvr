#include "VRCOLLADA_Material.h"
#include "core/objects/material/VRMaterial.h"

using namespace OSG;

VRCOLLADA_Material::VRCOLLADA_Material() {}
VRCOLLADA_Material::~VRCOLLADA_Material() {}

VRCOLLADA_MaterialPtr VRCOLLADA_Material::create() { return VRCOLLADA_MaterialPtr( new VRCOLLADA_Material() ); }
VRCOLLADA_MaterialPtr VRCOLLADA_Material::ptr() { return static_pointer_cast<VRCOLLADA_Material>(shared_from_this()); }

void VRCOLLADA_Material::newEffect(string id) {
    auto m = VRMaterial::create();
    library_effects[id] = m;
    currentMaterial = m;
}

void VRCOLLADA_Material::closeEffect() {
    currentMaterial = 0;
}

void VRCOLLADA_Material::setColor(string sid, Color4f col) {
    if (sid == "diffuse" && currentMaterial) currentMaterial->setDiffuse(Color3f(col[0], col[1], col[2]));
    //if (sid == "emission" && currentMaterial) currentMaterial->setEmissive(col);
}
