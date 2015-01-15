#ifndef OCTREE_H_INCLUDED_COLL
#define OCTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>

using namespace std;

OSG_BEGIN_NAMESPACE

class OcPoint {
    private:

    public:
        OcPoint(float x = 0, float y = 0, float z = 0);

        float x,y,z;
        void* data;

        float length();
        float dist(OcPoint p);
        OcPoint mult(float a);
        OcPoint add(OcPoint p);
        OcPoint sub(OcPoint p);
        bool inBox(OcPoint c, float size);

        void print();
};

class Octree {
    private:
        float resolution;
        float size;

        OcPoint center;

        Octree* parent;
        Octree* children[8];

        vector<void*> data;

        void destroy(Octree* guard);
        void findInSphere(OcPoint p, float r, vector<void*>& res);
        int getOctant(OcPoint p);

    public:
        Octree(float resolution);
        Octree* getRoot();

        void add(OcPoint p, void* data);
        void add(float x, float y, float z, void* data);

        void clear();

        vector<void*> radiusSearch(OcPoint p, float r);
        vector<void*> radiusSearch(float x, float y, float z, float r);

        void test();
        void print(int indent = 0);
        vector<void*> getData();
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
