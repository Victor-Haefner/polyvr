#ifndef VRLODTREE_H_INCLUDED
#define VRLODTREE_H_INCLUDED

#include "core/objects/VRLod.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

template<class T>
class OctreeNode;

class VRLodLeaf : public VRObject {
    private:
        OctreeNode<VRTransform*>* oLeaf = 0;
        int lvl = 0;
        VRLodPtr lod;
        vector<VRObjectPtr> levels;

    public:
        VRLodLeaf(string name, OctreeNode<VRTransform*>* o, int lvl);
        ~VRLodLeaf();
        static VRLodLeafPtr create(string name, OctreeNode<VRTransform*>* o, int lvl);
        VRLodLeafPtr ptr();

        void addLevel(float dist);
        void add(VRObjectPtr obj, int lvl);
        void set(VRObjectPtr obj, int lvl);
        void reset();

        OctreeNode<VRTransform*>* getOLeaf();
        VRLodPtr getLod();
        int getLevel();
};

class VRLodTree : public VRObject {
    protected:
        shared_ptr<Octree<VRTransform*>> octree;
        VRLodLeafPtr rootLeaf;
        map<OctreeNode<VRTransform*>*, VRLodLeafPtr> leafs;
        map<int, vector<VRTransformPtr> > objects;

        VRLodLeafPtr addLeaf(OctreeNode<VRTransform*>* o, int lvl);

    public:
        VRLodTree(string name, float size = 10);
        ~VRLodTree();
        static VRLodTreePtr create(string name = "lodtree", float size = 10);
        VRLodTreePtr ptr();

        VRLodLeafPtr addObject(VRTransformPtr obj, Vec3d p, int lvl, bool underLod = true);
        VRLodLeafPtr remObject(VRTransformPtr obj);
        void reset(float size = 0);
        int size();

        vector<VRLodLeafPtr> getSubTree(VRLodLeafPtr l);
        VRLodLeafPtr getLeaf(OctreeNode<VRTransform*>* o);
        map<OctreeNode<VRTransform*>*, VRLodLeafPtr>& getLeafs();
        vector<VRObjectPtr> rangeSearch(Vec3d p, float r, int depth = -1);

        void showOctree();
};

OSG_END_NAMESPACE;

#endif // VRLODTREE_H_INCLUDED
