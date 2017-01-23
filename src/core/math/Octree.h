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

        Vec3f center;

        Octree* parent = 0;
        Octree* children[8] = {0,0,0,0,0,0,0,0};

        vector<void*> data;
        vector<Vec3f> points;

        void destroy(Octree* guard);
        void findInSphere(Vec3f p, float r, vector<void*>& res);
        void findInBox(const boundingbox& b, vector<void*>& res);
        int getOctant(Vec3f p);
        bool inBox(Vec3f p, Vec3f c, float size);

    public:
        Octree(float resolution);
        ~Octree();

        static OctreePtr create(float resolution);

        Octree* getRoot();
        void add(Vec3f p, void* data, int maxjump = -1, bool checkPosition = true);
        void addBox(const boundingbox& b, void* data, int maxjump = -1, bool checkPosition = true);
        void set(Octree* node, Vec3f p, void* data);
        Octree* get(Vec3f p);
        vector<Octree*> getPathTo(Vec3f p);

        void clear();

        vector<void*> radiusSearch(Vec3f p, float r);
        vector<void*> boxSearch(const boundingbox& b);

        void test();
        void print(int indent = 0);
        vector<void*> getData();
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
