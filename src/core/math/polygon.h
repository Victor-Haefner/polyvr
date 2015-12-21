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
        bool closed = false;

        float getTurn(Vec2f p0, Vec2f p1, Vec2f p2);

    public:
        polygon();
        static std::shared_ptr<polygon> create();

        void addPoint(Vec2f p);
        Vec2f getPoint(int i);
        void close();
        int size();
        void clear();

        bool isConvex();
        bool isCCW();
        void turn();

        vector<Vec2f> get();
        void set(vector<Vec2f> vec);
        polygon sort();
        polygon getConvexHull();
        vector< polygon > getConvexDecomposition();

        vector<Vec3f> toSpace(Matrix m);

        string toString();
        static void runTest();
};

OSG_END_NAMESPACE;

#endif // POLYGON_H_INCLUDED
