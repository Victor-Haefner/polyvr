#include "VRLodTree.h"
#include "core/math/Octree.h"
#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace OSG;

VRLodLeaf::VRLodLeaf(string name, OctreeNode* o, int l) : VRObject(name), oLeaf(o), lvl(l) {}
VRLodLeaf::~VRLodLeaf() {}
VRLodLeafPtr VRLodLeaf::ptr() { return static_pointer_cast<VRLodLeaf>( shared_from_this() ); }

VRLodLeafPtr VRLodLeaf::create(string name, OctreeNode* o, int lvl) {
    auto l = VRLodLeafPtr(new VRLodLeaf(name, o, lvl));
    l->lod = VRLod::create("natureLod");
    l->lod->setPersistency(0);
    l->addChild(l->lod);
    auto lvl0 = VRObject::create("natureLodLvl");
    lvl0->setPersistency(0);
    l->levels.push_back(lvl0);
    l->lod->addChild(lvl0);
    return l;
}

VRLodPtr VRLodLeaf::getLod() { return lod; }

void VRLodLeaf::addLevel(float dist) {
    auto lvl = VRObject::create("lvl");
    lvl->setPersistency(0);
    levels.push_back(lvl);
    lod->addChild(lvl);
    lod->addDistance(dist);
}

void VRLodLeaf::add(VRObjectPtr obj, int lvl) {
    //cout << " VRLodLeaf::add leaf " << name << ", " << obj->getName() << " " << lvl << endl;
    levels[lvl]->addChild(obj);
}

void VRLodLeaf::set(VRObjectPtr obj, int lvl) {
    //if (obj) cout << " VRLodLeaf::set leaf " << name << ", " << obj->getName() << " " << lvl << endl;
    //else cout << " VRLodLeaf::set leaf " << name << " cleared " << lvl << endl;
    if (lvl < 0 || lvl >= int(levels.size())) return;
    levels[lvl]->clearChildren();
    if (obj) {
        levels[lvl]->addChild(obj);
        //cout << "  VRLodLeaf set addChild " << levels[lvl]->getName() << " " << obj->getName() << endl;
    }
}

void VRLodLeaf::reset() { set(0,1); }

OctreeNode* VRLodLeaf::getOLeaf() { return oLeaf; }
int VRLodLeaf::getLevel() { return lvl; }

// --------------------------------------------------------------------------------------------------

VRLodTree::VRLodTree(string name, float size) : VRObject(name) { octree = Octree::create(size,size,name); }
VRLodTree::~VRLodTree() {}
VRLodTreePtr VRLodTree::ptr() { return static_pointer_cast<VRLodTree>( shared_from_this() ); }
VRLodTreePtr VRLodTree::create(string name, float size) { return VRLodTreePtr(new VRLodTree(name, size)); }

void VRLodTree::reset(float size) {
    leafs.clear();
    objects.clear();
    rootLeaf = 0;
    if (size > 0) octree = Octree::create(size,size,name);
    else {
        auto s = octree->getSize();
        octree = Octree::create(s,s,name);
    }
    clearChildren();
}

VRLodLeafPtr VRLodTree::getLeaf(OctreeNode* o) {
    return leafs.count(o) ? leafs[o] : 0;
}

map<OctreeNode*, VRLodLeafPtr>& VRLodTree::getLeafs() { return leafs; }

vector<VRLodLeafPtr> VRLodTree::getSubTree(VRLodLeafPtr l) {
    vector<VRLodLeafPtr> res;
    if (!l->getOLeaf()) return res;
    for (auto o : l->getOLeaf()->getSubtree()) {
        if (!leafs.count(o)) continue;
        res.push_back(leafs[o]);
    }
    return res;
}

vector<VRObjectPtr> VRLodTree::rangeSearch(Vec3d p, float r, int d) {
    vector<VRObjectPtr> res;
    for (auto obj : octree->radiusSearch(p, r, d)) {
        VRObjectPtr o = ((VRObject*)obj)->ptr();
        res.push_back(o);
    }
    return res;
}

void VRLodTree::showOctree() {
    VRGeometryPtr o = octree->getVisualization();
    addChild(o);
}

VRLodLeafPtr VRLodTree::addLeaf(OctreeNode* o, int lvl) {
    if (leafs.count(o)) return leafs[o];
    auto l = VRLodLeaf::create("lodLeaf", o, lvl);
    l->setPersistency(0);
    if (lvl > 0) l->addLevel( o->getSize()*2 );
    l->getLod()->setCenter( o->getCenter() );
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

    VRLodLeafPtr oldRootLeaf = 0;
    if (auto p = o->getParent()) {
        if (!leafs.count(p)) addLeaf(p, lvl+1);
        leafs[p]->add(l,0);
    } else {
        if (rootLeaf) oldRootLeaf = rootLeaf;
        rootLeaf = l;
        addChild(l);
    }

    if (oldRootLeaf) { // TODO: find pl
        auto p = oldRootLeaf->getOLeaf()->getParent();
        if (!leafs.count(p)) addLeaf(p, lvl+1);
        leafs[p]->add(oldRootLeaf,0);
        oldRootLeaf->getLod()->setCenter( oldRootLeaf->getOLeaf()->getCenter() );
    }

    return l;
}

VRLodLeafPtr VRLodTree::addObject(VRTransformPtr obj, Vec3d p, int lvl, bool underLod) {
    if (!octree || !obj) return 0;
    //cout << "VRLodTree::addObject " << obj->getName() << " p " << p << " lvl: " << lvl << endl;
    if (leafs.size() == 0) addLeaf(octree->getRoot(), 0);
    objects[lvl].push_back(obj);
    OctreeNode* oLeaf = octree->add(p, obj.get(), lvl, true);
    //cout << " VRLodTree::addObject octree leaf: " << oLeaf->getSize() << endl;
    auto leaf = addLeaf(oLeaf, lvl);
    if (lvl == 0 || !underLod) leaf->add(obj, 0);
    else          leaf->add(obj, 1);
    obj->setRelativePosition(p, ptr());
    obj->setDir(Vec3d(0,0,-1));
    obj->setUp(Vec3d(0,1,0));
    return leaf;
}

VRLodLeafPtr VRLodTree::remObject(VRTransformPtr obj) { // TODO, finish it!
    if (!octree) return 0;
    OctreeNode* o = octree->get( obj->getWorldPosition() );
    o->remData(obj.get());
    return leafs[o];
}

int VRLodTree::size() {
    int N = 0;
    for (auto ov : objects) N += ov.second.size();
    return N;
}
