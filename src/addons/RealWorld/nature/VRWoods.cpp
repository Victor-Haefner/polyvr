#include "VRWoods.h"

#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/VRLod.h"
#include "core/math/Octree.h"
#include "core/utils/toString.h"

using namespace OSG;

VRLodLeaf::VRLodLeaf(string name) : VRTransform(name) {}
VRLodLeaf::~VRLodLeaf() {}
VRLodLeafPtr VRLodLeaf::ptr() { return static_pointer_cast<VRLodLeaf>( shared_from_this() ); }

VRLodLeafPtr VRLodLeaf::create(string name) {
    auto l = VRLodLeafPtr(new VRLodLeaf(name));
    l->lod = VRLod::create("lod");
    l->addChild(l->lod);
    auto lvl0 = VRObject::create("lvl");
    l->levels.push_back(lvl0);
    l->lod->addChild(lvl0);
    return l;
}

void VRLodLeaf::addLevel(float dist) {
    auto lvl = VRObject::create("lvl");
    levels.push_back(lvl);
    lod->addChild(lvl);
    lod->addDistance(dist);
}

void VRLodLeaf::add(VRObjectPtr obj, int lvl) {
    levels[lvl]->addChild(obj);
}



// --------------------------------------------------------------------------------------------------

VRLodTree::VRLodTree(string name) : VRObject(name) {}

VRLodTree::~VRLodTree() { octree = Octree::create(10); }
VRLodTreePtr VRLodTree::create(string name) { return VRLodTreePtr(new VRLodTree(name)); }
VRLodTreePtr VRLodTree::ptr() { return static_pointer_cast<VRLodTree>( shared_from_this() ); }

void VRLodTree::addObject(VRObjectPtr obj, Vec3f p, int lvl) {

}


void VRLodTree::newQuad(VRObjectPtr obj, VRObjectPtr parent, float o) {
    auto o1 = dynamic_pointer_cast<VRTransform>( obj->duplicate() );
    auto o2 = dynamic_pointer_cast<VRTransform>( obj->duplicate() );
    auto o3 = dynamic_pointer_cast<VRTransform>( obj->duplicate() );
    auto o4 = dynamic_pointer_cast<VRTransform>( obj->duplicate() );

    o1->setFrom(Vec3f(-o, 0,-o));
    o2->setFrom(Vec3f(-o, 0, o));
    o3->setFrom(Vec3f( o, 0,-o));
    o4->setFrom(Vec3f( o, 0, o));

    parent->addChild( o1 );
    parent->addChild( o2 );
    parent->addChild( o3 );
    parent->addChild( o4 );
}

VRLodLeafPtr VRLodTree::addLayer(float s, float d, VRObjectPtr obj) {
    auto l = VRLodLeaf::create("lodLeaf");
    l->addLevel(d);
    l->add( obj, 1 );
    //layers[s] = l;
    return l;
}

// --------------------------------------------------------------------------------------------------

VRWoods::VRWoods() : VRLodTree("woods") { setPersistency(0); }
VRWoods::~VRWoods() {}
VRWoodsPtr VRWoods::create() { return VRWoodsPtr(new VRWoods()); }
VRWoodsPtr VRWoods::ptr() { return static_pointer_cast<VRWoods>( shared_from_this() ); }

void VRWoods::addTree(VRTreePtr t) {}

void VRWoods::test() {
    auto newCylinder = [](float s) {
        auto box = VRGeometry::create("box");
        string S = toString(s*0.5);
        box->setPrimitive("Cylinder", "1 "+S+" 16 1 1 1");
        return box;
    };

    auto l1 = addLayer(1, 5,newCylinder(1));
    auto l2 = addLayer(2,10,newCylinder(2));
    auto l3 = addLayer(4,20,newCylinder(4));

    auto c = newCylinder(0.5);
    auto b = VRObject::create("node");
    newQuad( c, b, 0.25*1);
    l1->add( b, 0 );

    b = VRObject::create("node");
    newQuad( l1, b, 0.25*2);
    l2->add( b, 0 );

    b = VRObject::create("node");
    newQuad( l2, b, 0.25*4);
    l3->add( b, 0 );

    addChild(l3);
}


