#include "VRAtlas.h"

#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"

#include <iostream>
#include <fstream>
#include <cmath>

using namespace OSG;

template<> string typeName(const VRAtlas& p) { return "Atlas"; }

VRAtlas::VRAtlas() {}
VRAtlas::~VRAtlas() {}

VRAtlasPtr VRAtlas::create() { return VRAtlasPtr( new VRAtlas() ); }
//VRAtlasPtr VRAtlas::ptr() { return static_pointer_cast<VRAtlasPtr>(shared_from_this()); }

void VRAtlas::test() {
    cout << "VRAtlas::test" << endl;
}
