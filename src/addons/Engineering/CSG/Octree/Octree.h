#ifndef OCTREE_H_INCLUDED_COLL
#define OCTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>

using namespace std;

OSG_BEGIN_NAMESPACE

namespace CSGApp {

class Point {
    private:

    public:
        Point(float x = 0, float y = 0, float z = 0);

        float x,y,z;
        void* data;

        float length();
        float dist(Point p);
        Point mult(float a);
        Point add(Point p);
        Point sub(Point p);
        bool inBox(Point c, float size);

        void print();
};

class Octree {
    private:
        float resolution;
        float size;

        Point center;

        Octree* parent;
        Octree* children[8];

        vector<void*> data;

        void destroy(Octree* guard);
        void findInSphere(Point p, float r, vector<void*>& res);
        int getOctant(Point p);

    public:
        Octree(float resolution);
        Octree* getRoot();

        void add(Point p, void* data);
        void add(float x, float y, float z, void* data);

        void clear();

        vector<void*> radiusSearch(Point p, float r);
        vector<void*> radiusSearch(float x, float y, float z, float r);

        void test();
        void print(int indent = 0);
        vector<void*> getData();
};

}

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
