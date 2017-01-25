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

VRLodLeaf::VRLodLeaf(string name, Octree* o, int l) : VRTransform(name), oLeaf(o), lvl(l) {}
VRLodLeaf::~VRLodLeaf() {}
VRLodLeafPtr VRLodLeaf::ptr() { return static_pointer_cast<VRLodLeaf>( shared_from_this() ); }

VRLodLeafPtr VRLodLeaf::create(string name, Octree* o, int lvl) {
    auto l = VRLodLeafPtr(new VRLodLeaf(name, o, lvl));
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

Octree* VRLodLeaf::getOLeaf() { return oLeaf; }
int VRLodLeaf::getLevel() { return lvl; }

// --------------------------------------------------------------------------------------------------

VRLodTree::VRLodTree(string name, float size) : VRObject(name) { octree = Octree::create(size,size); }
VRLodTree::~VRLodTree() {}
VRLodTreePtr VRLodTree::ptr() { return static_pointer_cast<VRLodTree>( shared_from_this() ); }
VRLodTreePtr VRLodTree::create(string name) { return VRLodTreePtr(new VRLodTree(name)); }

void VRLodTree::reset(float size) {
    octree = Octree::create(size,size);
    leafs.clear();
    objects.clear();
    rootLeaf = 0;
}

void VRLodTree::addLeaf(Octree* o, int lvl) {
    if (leafs.count(o)) return;
    auto l = VRLodLeaf::create("lodLeaf", o, lvl);
    if (lvl > 0) l->addLevel( o->getSize()*4 );
    l->setFrom(o->getLocalCenter());
    leafs[o] = l;

    /**
    add lod leaf to tree, handle following cases:
        - octree leaf has parent
            - parent lod leaf does not exist
                -> call addLeaf on the parent octree leaf, may result in further recursion
            - parent lod leaf exists
                -> pass
            (at this point the leaf parent should exist!)
            -> add leaf to leaf parent
        - octree leaf has no parent
            - first lod leaf (no root leaf set), created in tree constructor
                -> pass
            - tree grows bigger, new root leaf
                -> switch parent of old root leaf to new root leaf
                -> update local position of old root leaf
            -> remember as root leaf
            -> add as child to tree
    */

    if (auto p = o->getParent()) {
        if (!leafs.count(p)) addLeaf(p, lvl+1);
        leafs[p]->add(l,0);
    } else {
        if (rootLeaf) {
            l->add(rootLeaf,0);
            rootLeaf->setFrom(rootLeaf->getOLeaf()->getLocalCenter());
        }
        rootLeaf = l;
        addChild(l);
    }
}

void VRLodTree::addObject(VRTransformPtr obj, Vec3f p, int lvl) {
    if (leafs.size() == 0) addLeaf(octree.get(), 0);
    if (!octree) return;
    objects[lvl].push_back(obj);
    auto oLeaf = octree->add(p, obj.get(), lvl, 0, true);
    addLeaf(oLeaf, lvl);
    if (lvl == 0) leafs[oLeaf]->add(obj, 0);
    else          leafs[oLeaf]->add(obj, 1);
    obj->setWorldPosition(p);
    obj->setDir(Vec3f(0,0,-1));
    obj->setUp(Vec3f(0,1,0));
}

// --------------------------------------------------------------------------------------------------

VRWoods::VRWoods() : VRLodTree("woods", 1) { setPersistency(0); }
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

    auto simpleTest = [&]() {
        // add highest detail objects
        int N = 4;
        for (int i=0; i<N; i++) {
            for (int j=0; j<N; j++) {
                auto c = newCylinder(1);
                Vec3f p = Vec3f((i-N*0.5)*15,0,(j-N*0.5)*15);
                addObject(c, p, 0);
            }
        }

        // add lower detailed objects
        for (auto l : leafs) {
            auto& leaf = l.second;
            cout << "LEAF: " << leaf->getOLeaf()->getSize() << " " << leaf->getLevel() << endl;
            int lvl = leaf->getLevel();
            if (lvl == 0) continue;
            auto c = newCylinder(pow(2.0,lvl));
            leaf->add(c,1);
            //c->setWorldPosition(Vec3f());
        }
    };

    auto simpleTest2 = [&]() {
        reset(1);

        for (int k=0; k<5; k++) {
            int N = pow(2,k);
            float s = 16.0/N;
            float o = 0.5*s;
            for (int i=0; i<N; i++) {
                for (int j=0; j<N; j++) {
                    auto c = newCylinder(s);
                    Vec3f p = Vec3f(o+s*i,0,o+s*j);
                    addObject(c, p, 4-k);
                }
            }
        }
    };

    //simpleTest();
    simpleTest2();
}




