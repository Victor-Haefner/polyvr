#ifndef OCTREE_H_INCLUDED_COLL
#define OCTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>

using namespace std;

OSG_BEGIN_NAMESPACE

struct OcPoint {
    Vec3f pos;
    void* data;

    OcPoint(Vec3f p, void* d = 0);
};

class Octree {
    private:
        float resolution = 0.1;
        float size = 10;

        Vec3f center;

        Octree* parent = 0;
        Octree* children[8] = {0,0,0,0,0,0,0,0};

        vector<void*> data;

        void destroy(Octree* guard);
        void findInSphere(Vec3f p, float r, vector<void*>& res);
        int getOctant(Vec3f p);
        bool inBox(Vec3f p, Vec3f c, float size);

    public:
        Octree(float resolution);
        Octree* getRoot();

        void add(OcPoint p, int maxjump = -1);
        void add(Vec3f p, void* data, int maxjump = -1);
        void set(Octree* node, void* data);
        Octree* get(Vec3f p);

        void clear();

        vector<void*> radiusSearch(Vec3f p, float r);

        void test();
        void print(int indent = 0);
        vector<void*> getData();
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
