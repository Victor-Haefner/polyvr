#ifndef VRWOODS_H_INCLUDED
#define VRWOODS_H_INCLUDED

#include "../VRWorldModule.h"
#include "core/objects/VRLod.h"
#include "core/objects/VRTransform.h"
#include "core/scene/VRObjectManager.h"
#include "core/math/VRMathFwd.h"

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
        void set(VRObjectPtr obj, int lvl);
        void reset();

        Octree* getOLeaf();
        int getLevel();
};

class VRLodTree : public VRObject {
    protected:
        OctreePtr octree;
        VRLodLeafPtr rootLeaf;
        map<Octree*, VRLodLeafPtr> leafs;
        map<int, vector<VRTransformPtr> > objects;

        VRLodLeafPtr addLeaf(Octree* o, int lvl);

    public:
        VRLodTree(string name, float size = 10);
        ~VRLodTree();
        static VRLodTreePtr create(string name);
        VRLodTreePtr ptr();

        VRLodLeafPtr addObject(VRTransformPtr obj, Vec3d p, int lvl);
        VRLodLeafPtr remObject(VRTransformPtr obj);
        void reset(float size = 0);

        vector<VRLodLeafPtr> getSubTree(VRLodLeafPtr l);
};

class VRNature : public VRLodTree, public VRWorldModule {
    private:
        map<int, VRTreePtr> treesByID;
        map<string, VRTreePtr> treeTemplates;
        map<string, VRTreePtr> bushTemplates;
        map<string, shared_ptr<VRObjectManager::Entry> > treeEntries;
        map<VRTree*, VRTreePtr> treeRefs;
        VRGeometryPtr collisionMesh;

        map<VRGrassPatch*, VRGrassPatchPtr> grassPatchRefs;

        VRMaterialPtr truncMat;
        VRMaterialPtr leafMat1;
        VRMaterialPtr leafMat2;
        VRMaterialPtr grassMat;

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
        void computeLODs(map<Octree*, VRLodLeafPtr>& leafs);


        void addCollisionModels();
};

OSG_END_NAMESPACE;

#endif // VRWOODS_H_INCLUDED
