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

class OctreeNode : public std::enable_shared_from_this<OctreeNode> {
    private:
        float resolution = 0.1;
        float size = 10;

        Vec3d center;

        OctreeWeakPtr tree;
        OctreeNodeWeakPtr parent;
        OctreeNodePtr children[8];

        vector<void*> data;
        vector<Vec3d> points;

    public:
        OctreeNode(OctreePtr tree, float resolution, float size = 10);
        ~OctreeNode();

        static OctreeNodePtr create(OctreePtr tree, float resolution, float size = 10);
        OctreeNodePtr ptr();

        OctreeNodePtr getParent();
        vector<OctreeNodePtr> getAncestry();
        void set(OctreeNodePtr node, Vec3d p, void* data);
        float getSize();
        Vec3d getCenter();
        Vec3d getLocalCenter();

        OctreeNodePtr add(Vec3d p, void* data, int targetLevel = -1, int currentLevel = 0, bool checkPosition = true);
        OctreeNodePtr get(Vec3d p);

        void remData(void* data);
        //void clear();

        vector<OctreeNodePtr> getChildren();
        vector<OctreeNodePtr> getSubtree();
        vector<OctreeNodePtr> getPathTo(Vec3d p);

        vector<void*> getData();
        vector<void*> getAllData();

        //void destroy(OctreeNodePtr guard);
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
        OctreeNodePtr root;

        Octree(float resolution, float size = 10);

    public:
        ~Octree();
        static OctreePtr create(float resolution, float size = 10);
        OctreePtr ptr();

        OctreeNodePtr getRoot();
        void addBox(const Boundingbox& b, void* data, int targetLevel = -1, bool checkPosition = true);
        OctreeNodePtr add(Vec3d p, void* data, int targetLevel = -1, int currentLevel = 0, bool checkPosition = true);
        OctreeNodePtr get(Vec3d p);

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
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
