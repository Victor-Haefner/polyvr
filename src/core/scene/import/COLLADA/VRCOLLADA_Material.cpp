#include "VRCOLLADA_Material.h"

using namespace OSG;

VRCOLLADA_Material::VRCOLLADA_Material() {}
VRCOLLADA_Material::~VRCOLLADA_Material() {}

VRCOLLADA_MaterialPtr VRCOLLADA_Material::create() { return VRCOLLADA_MaterialPtr( new VRCOLLADA_Material() ); }
VRCOLLADA_MaterialPtr VRCOLLADA_Material::ptr() { return static_pointer_cast<VRCOLLADA_Material>(shared_from_this()); }
