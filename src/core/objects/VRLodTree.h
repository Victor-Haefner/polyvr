#ifndef VRLODTREE_H_INCLUDED
#define VRLODTREE_H_INCLUDED

#include "core/objects/VRLod.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class OctreeNode;

class VRLodLeaf : public VRTransform {
    private:
        OctreeNode* oLeaf = 0;
        int lvl = 0;
        VRLodPtr lod;
        vector<VRObjectPtr> levels;

    public:
        VRLodLeaf(string name, OctreeNode* o, int lvl);
        ~VRLodLeaf();
        static VRLodLeafPtr create(string name, OctreeNode* o, int lvl);
        VRLodLeafPtr ptr();

        void addLevel(float dist);
        void add(VRObjectPtr obj, int lvl);
        void set(VRObjectPtr obj, int lvl);
        void reset();

        OctreeNode* getOLeaf();
        int getLevel();
};

class VRLodTree : public VRObject {
    protected:
        OctreePtr octree;
        VRLodLeafPtr rootLeaf;
        map<OctreeNode*, VRLodLeafPtr> leafs;
        map<int, vector<VRTransformPtr> > objects;

        VRLodLeafPtr addLeaf(OctreeNode* o, int lvl);

    public:
        VRLodTree(string name, float size = 10);
        ~VRLodTree();
        static VRLodTreePtr create(string name = "lodtree");
        VRLodTreePtr ptr();

        VRLodLeafPtr addObject(VRTransformPtr obj, Vec3d p, int lvl);
        VRLodLeafPtr remObject(VRTransformPtr obj);
        void reset(float size = 0);
        int size();

        vector<VRLodLeafPtr> getSubTree(VRLodLeafPtr l);

        void showOctree();
};

OSG_END_NAMESPACE;

#endif // VRLODTREE_H_INCLUDED
