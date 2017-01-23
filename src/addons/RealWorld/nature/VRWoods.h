#ifndef VRWOODS_H_INCLUDED
#define VRWOODS_H_INCLUDED

#include "core/objects/VRLod.h"
#include "core/math/VRMathFwd.h"
#include "addons/RealWorld/VRRealWorldFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLodTree : public VRLod {
    private:
        OctreePtr octree;

    public:
        VRLodTree(string name);
        ~VRLodTree();

        static VRLodTreePtr create(string name);
        VRLodTreePtr ptr();

        void addObject(VRObjectPtr obj);
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
};

OSG_END_NAMESPACE;

#endif // VRWOODS_H_INCLUDED
