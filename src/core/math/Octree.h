#ifndef OCTREE_H_INCLUDED_COLL
#define OCTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "VRMathFwd.h"
#include "boundingbox.h"

using namespace std;

OSG_BEGIN_NAMESPACE

class OctreeNode {
    private:
        float resolution = 0.1;
        float size = 10;

        Vec3d center;

        OctreeWeakPtr tree;
        OctreeNode* parent = 0;
        OctreeNode* children[8] = {0,0,0,0,0,0,0,0};

        vector<void*> data;
        vector<Vec3d> points;

    public:
        OctreeNode(OctreePtr tree, float resolution, float size = 10);
        ~OctreeNode();

        OctreeNode* getParent();
        vector<OctreeNode*> getAncestry();
        void set(OctreeNode* node, Vec3d p, void* data);
        float getSize();
        Vec3d getCenter();
        Vec3d getLocalCenter();

        OctreeNode* add(Vec3d p, void* data, int targetLevel = -1, int currentLevel = 0, bool checkPosition = true);
        OctreeNode* get(Vec3d p);

        void remData(void* data);
        //void clear();

        vector<OctreeNode*> getChildren();
        vector<OctreeNode*> getSubtree();
        vector<OctreeNode*> getPathTo(Vec3d p);

        vector<void*> getData();
        vector<void*> getAllData();

        //void destroy(OctreeNode* guard);
        void findInSphere(Vec3d p, float r, vector<void*>& res);
        void findInBox(const Boundingbox& b, vector<void*>& res);
        int getOctant(Vec3d p);
        bool inBox(Vec3d p, Vec3d c, float size);

        void print(int indent = 0);
        string toString(int indent = 0);
};

class Octree : public std::enable_shared_from_this<Octree> {
    private:
        float resolution = 0.1;
        float firstSize = 10;
        OctreeNode* root = 0;

        Octree(float resolution, float size = 10);

    public:
        ~Octree();
        static OctreePtr create(float resolution, float size = 10);
        OctreePtr ptr();

        OctreeNode* getRoot();
        void addBox(const Boundingbox& b, void* data, int targetLevel = -1, bool checkPosition = true);
        OctreeNode* add(Vec3d p, void* data, int targetLevel = -1, int currentLevel = 0, bool checkPosition = true);
        OctreeNode* get(Vec3d p);

        float getSize();
        void clear();
        void updateRoot();

        template<class T>
        void delContent() {
            for (void* o : getAllData()) delete (T*)o;
        }

        vector<void*> getAllData();
        vector<void*> radiusSearch(Vec3d p, float r);
        vector<void*> boxSearch(const Boundingbox& b);

        void test();

        VRGeometryPtr getVisualization();
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
