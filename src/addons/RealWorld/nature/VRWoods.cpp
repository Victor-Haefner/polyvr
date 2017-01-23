#include "VRWoods.h"

#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/VRLod.h"
#include "core/math/Octree.h"

using namespace OSG;

VRLodTree::VRLodTree(string name) : VRObject(name) {
    octree = Octree::create(5);
}

VRLodTree::~VRLodTree() {}
VRLodTreePtr VRLodTree::create(string name) { return VRLodTreePtr(new VRLodTree(name)); }
VRLodTreePtr VRLodTree::ptr() { return static_pointer_cast<VRLodTree>( shared_from_this() ); }

void VRLodTree::addObject(VRObjectPtr obj, Vec3f p, int lvl) {
    octree->add(p, obj.get());

    vector<Octree*> path = octree->getPathTo(p);

    VRLodPtr lod1, lod2;
    for (int i=0; i< path.size(); i++) {
        auto o = path[i];
        if (!lods.count(o)) {
            lods[o] = VRLod::create("subLod");
            lods[o]->addEmpty();
            lods[o]->addDistance(10*lvl);
        }
        lod2 = lods[o];
        if (!lod1) addChild(lod2);
        else lod1->addChild(lod2);
        lod1 = lod2;
    }

    auto oend = path[path.size()-1];
    lods[oend]->addChild(obj);
}



VRWoods::VRWoods() : VRLodTree("woods") {}

VRWoods::~VRWoods() {}
VRWoodsPtr VRWoods::create() { return VRWoodsPtr(new VRWoods()); }
VRWoodsPtr VRWoods::ptr() { return static_pointer_cast<VRWoods>( shared_from_this() ); }

void VRWoods::addTree(VRTreePtr t) {
}

void VRWoods::test() {
    for (int i=0; i<30; i++) {
        for (int j=0; j<30; j++) {
            auto test_geo = VRGeometry::create("test");
            test_geo->setPrimitive("Box", "1 1 1 1 1 1");
            addObject(test_geo, Vec3f(i,0,j), 0);
        }
    }
}
