#ifndef VRWOODS_H_INCLUDED
#define VRWOODS_H_INCLUDED

#include "../VRWorldModule.h"
#include "core/objects/VRLodTree.h"
#include "core/scene/VRObjectManager.h"
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRNature : public VRLodTree, public VRWorldModule {
    private:
        map<int, VRTreePtr> treesByID;
        map<string, VRTreePtr> treeTemplates;
        map<string, VRTreePtr> bushTemplates;
        map<string, shared_ptr<VRObjectManager::Entry> > treeEntries;
        map<VRTree*, VRTreePtr> treeRefs;
        map<VRGrassPatch*, VRGrassPatchPtr> grassPatchRefs;

        VRMaterialPtr truncMat;
        VRMaterialPtr leafMat1;
        VRMaterialPtr leafMat2;
        VRMaterialPtr grassMat;

        VRGeometryPtr collisionMesh;
        VRGeometryPtr trees;
        VRGeometryPtr groundPatches;
        VRGeometryPtr grassGroundPatches;

        VRThreadCbPtr worker;
        void computeLODsThread(VRThreadWeakPtr t);

        void computeFirstLevel();
        void computeSecondLevel();

        void setup();
        void initLOD();

    public:
        VRNature(string name);
        ~VRNature();

        static VRNaturePtr create(string name = "woods");
        VRNaturePtr ptr();

        void simpleInit(int treeTypes, int bushTypes);

        void clear();
        void addTreeTemplate(VRTreePtr t);
        void addBushTemplate(VRTreePtr t);
        VRTreePtr addTree(VRTreePtr t, bool updateLODs = 0, bool addToStore = true);
        VRTreePtr addBush(VRTreePtr t, bool updateLODs = 0, bool addToStore = true);
        VRTreePtr getTree(int id);
        VRTreePtr createTree(string type, Vec3d p);
        VRTreePtr createBush(string type, Vec3d p);
        VRTreePtr createRandomTree(Vec3d p);
        VRTreePtr createRandomBush(Vec3d p);

        void addWoods(VRPolygonPtr area, bool addGround = 0);
        void addScrub(VRPolygonPtr area, bool addGround = 0);
        void addGrassPatch(VRPolygonPtr area, bool updateLODs = 0, bool addGround = 0);
        void removeTree(int id);
        void computeAllLODs(bool threaded = false);
        void computeLODs(VRLodLeafPtr leaf);
        void computeLODs(map<OctreeNode*, VRLodLeafPtr>& leafs);
        void computeLODs2(map<OctreeNode*, VRLodLeafPtr>& leafs);
        void computeLODs3(map<OctreeNode*, VRLodLeafPtr>& leafs);

        VRLodTreePtr getLodTree();

        void addCollisionModels();
        VRGeometryPtr getCollisionObject();
};

OSG_END_NAMESPACE;

#endif // VRWOODS_H_INCLUDED
