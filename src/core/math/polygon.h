#ifndef POLYGON_H_INCLUDED
#define POLYGON_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class polygon {
    private:
        vector<Vec2f> points;
        bool convex = false;

    public:
        polygon();
        void addPoint(Vec2f p);
        Vec2f getPoint(int i);
        void close();
        int size();
        void clear();

        vector<Vec2f> get();
        vector<Vec2f> sort();
        polygon getConvexHull();
        vector< polygon > getConvexDecomposition();

        vector<Vec3f> toSpace(Matrix m);

        string toString();
        static void runTest();
};

OSG_END_NAMESPACE;

#endif // POLYGON_H_INCLUDED
