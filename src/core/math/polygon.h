#ifndef POLYGON_H_INCLUDED
#define POLYGON_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include "boundingbox.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPolygon {
    private:
        vector<Vec2f> points;
        vector<Vec3f> points3;
        bool is3D = false;
        bool convex = false;
        bool closed = false;

        float getTurn(Vec2f p0, Vec2f p1, Vec2f p2);

    public:
        VRPolygon();
        static std::shared_ptr<VRPolygon> create();

        void addPoint(Vec2f p);
        void addPoint(Vec3f p);
        Vec2f getPoint(int i);
        Vec3f getPoint3(int i);
        void close();
        int size();
        void clear();

        bool isConvex();
        bool isCCW();
        void turn();
        void translate(Vec3f v);
        void scale(Vec3f s);

        vector<Vec2f> get();
        vector<Vec3f> get3();
        void set(vector<Vec2f> vec);
        VRPolygon sort();
        VRPolygon getConvexHull();
        Boundingbox getBoundingBox();
        vector< VRPolygon > getConvexDecomposition();
        vector< VRPolygonPtr > gridSplit(float G);

        float computeArea();
        VRPolygonPtr shrink(float amount);
        Vec3f getRandomPoint();
        vector<Vec3f> getRandomPoints(float density = 10, float padding = 0);

        vector<Vec3f> toSpace(Matrix m);
        bool isInside(Vec2f p);

        string toString();
        static void runTest();
};

OSG_END_NAMESPACE;

#endif // POLYGON_H_INCLUDED
