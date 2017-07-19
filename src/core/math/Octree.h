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

class Octree {
    private:
        float resolution = 0.1;
        float size = 10;

        Vec3d center;

        Octree* parent = 0;
        Octree* children[8] = {0,0,0,0,0,0,0,0};

        vector<void*> data;
        vector<Vec3d> points;

        void destroy(Octree* guard);
        void findInSphere(Vec3d p, float r, vector<void*>& res);
        void findInBox(const Boundingbox& b, vector<void*>& res);
        int getOctant(Vec3d p);
        bool inBox(Vec3d p, Vec3d c, float size);

    public:
        Octree(float resolution, float size = 10);
        ~Octree();

        static OctreePtr create(float resolution, float size = 10);

        Octree* getParent();
        vector<Octree*> getAncestry();
        Octree* getRoot();
        Octree* add(Vec3d p, void* data, int targetLevel = -1, int currentLevel = 0, bool checkPosition = true);
        void addBox(const Boundingbox& b, void* data, int targetLevel = -1, bool checkPosition = true);
        void set(Octree* node, Vec3d p, void* data);
        Octree* get(Vec3d p);
        float getSize();
        Vec3d getCenter();
        Vec3d getLocalCenter();

        void remData(void* data);
        void clear();

        vector<Octree*> getChildren();
        vector<Octree*> getSubtree();
        vector<Octree*> getPathTo(Vec3d p);

        vector<void*> getData();
        vector<void*> getAllData();
        vector<void*> radiusSearch(Vec3d p, float r);
        vector<void*> boxSearch(const Boundingbox& b);

        void test();
        void print(int indent = 0);
        string toString(int indent = 0);
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
