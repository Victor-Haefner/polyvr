#ifndef VRWOODS_H_INCLUDED
#define VRWOODS_H_INCLUDED

#include "core/objects/VRLod.h"
#include "core/objects/VRTransform.h"
#include "core/math/VRMathFwd.h"
#include "addons/RealWorld/VRRealWorldFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLodLeaf : public VRTransform {
    private:
        Octree* oLeaf = 0;
        int lvl = 0;
        VRLodPtr lod;
        vector<VRObjectPtr> levels;

    public:
        VRLodLeaf(string name, Octree* o, int lvl);
        ~VRLodLeaf();
        static VRLodLeafPtr create(string name, Octree* o, int lvl);
        VRLodLeafPtr ptr();

        void addLevel(float dist);
        void add(VRObjectPtr obj, int lvl);

        Octree* getOLeaf();
        int getLevel();
};

class VRLodTree : public VRObject {
    protected:
        OctreePtr octree;
        VRLodLeafPtr rootLeaf;
        map<Octree*, VRLodLeafPtr> leafs;
        map<int, vector<VRTransformPtr> > objects;

        void addLeaf(Octree* o, int lvl);

    public:
        VRLodTree(string name, float size = 10);
        ~VRLodTree();
        static VRLodTreePtr create(string name);
        VRLodTreePtr ptr();

        void addObject(VRTransformPtr obj, Vec3f p, int lvl);
};

class VRWoods : public VRLodTree {
    private:
        vector<VRTreePtr> trees;

        void computeFirstLevel();
        void computeSecondLevel();

        void initLOD();

    public:
        VRWoods();
        ~VRWoods();

        static VRWoodsPtr create();
        VRWoodsPtr ptr();

        void addTree(VRTreePtr t);

        void test();
};

OSG_END_NAMESPACE;

#endif // VRWOODS_H_INCLUDED
