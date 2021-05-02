#ifndef OCTREE_H_INCLUDED_COLL
#define OCTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include "core/math/VRMathFwd.h"
#include "boundingbox.h"
#include "Partitiontree.h"

using namespace std;

OSG_BEGIN_NAMESPACE

class OctreeNode : public PartitiontreeNode {
    private:
        OctreeWeakPtr tree;
        OctreeNode* parent = 0;
        OctreeNode* children[8] = {0,0,0,0,0,0,0,0};

    public:
        OctreeNode(OctreePtr tree, float resolution, float size = 10, int level = 0);
        ~OctreeNode();

        OctreeNode* getParent();
        OctreeNode* getRoot();
        vector<OctreeNode*> getAncestry();
        void set(OctreeNode* node, Vec3d p, void* data);
        Vec3d getLocalCenter();

        OctreeNode* add(Vec3d p, void* data, int targetLevel = -1, bool checkPosition = true, int partitionLimit = -1);
        OctreeNode* get(Vec3d p, bool checkPosition = true);

        bool isLeaf();

        vector<OctreeNode*> getChildren();
        vector<OctreeNode*> getSubtree();
        vector<OctreeNode*> getLeafs();
        vector<OctreeNode*> getPathTo(Vec3d p);

        vector<void*> getAllData();

        int dataSize();

        //void destroy(OctreeNode* guard);
        void findInSphere(Vec3d p, float r, int d, vector<void*>& res);
        void findPointsInSphere(Vec3d p, float r, int d, vector<Vec3d>& res, bool getAll);
        void findInBox(const Boundingbox& b, int d, vector<void*>& res);
        int getOctant(Vec3d p);
        bool inBox(Vec3d p, Vec3d c, float size);

        void print(int indent = 0);
        string toString(int indent = 0);
};

class Octree : public Partitiontree {
    private:
        OctreeNode* root = 0;

        Octree(float resolution, float size = 10, string name = "");

    public:
        ~Octree();
        static OctreePtr create(float resolution, float size = 10, string name = "");
        OctreePtr ptr();

        OctreeNode* getRoot();
        void addBox(const Boundingbox& b, void* data, int targetLevel = -1, bool checkPosition = true);
        OctreeNode* add(Vec3d p, void* data, int targetLevel = -1, bool checkPosition = true, int partitionLimit = -1);
        OctreeNode* get(Vec3d p, bool checkPosition = true);
        vector<OctreeNode*> getAllLeafs();

        void setResolution(float res);
        float getSize();
        void clear();
        void updateRoot();

        template<class T>
        void delContent() {
            for (void* o : getAllData()) delete (T*)o;
        }

        vector<void*> getAllData();
        vector<void*> radiusSearch(Vec3d p, float r, int d = -1);
        vector<Vec3d> radiusPointSearch(Vec3d p, float r, int d = -1, bool getAll = true);
        vector<void*> boxSearch(const Boundingbox& b, int d = -1);

        void test();

        VRGeometryPtr getVisualization(bool onlyLeafes = false);
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
