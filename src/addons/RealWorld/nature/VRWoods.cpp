#include "VRWoods.h"

#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/VRLod.h"

using namespace OSG;

VRLodTree::VRLodTree(string name) : VRLod(name) {}
VRLodTree::~VRLodTree() {}
VRLodTreePtr VRLodTree::create(string name) { return VRLodTreePtr(new VRLodTree(name)); }
VRLodTreePtr VRLodTree::ptr() { return static_pointer_cast<VRLodTree>( shared_from_this() ); }

void VRLodTree::addObject(VRObjectPtr obj) {
    ;
}



VRWoods::VRWoods() : VRLodTree("woods") {}
VRWoods::~VRWoods() {}
VRWoodsPtr VRWoods::create() { return VRWoodsPtr(new VRWoods()); }
VRWoodsPtr VRWoods::ptr() { return static_pointer_cast<VRWoods>( shared_from_this() ); }

void VRWoods::addTree(VRTreePtr t) {
    ;
}
