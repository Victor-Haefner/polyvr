#ifndef POLYGON_H_INCLUDED
#define POLYGON_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGMatrix.h>
#include "boundingbox.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRPolygon {
    private:
        vector<Vec2d> points;
        vector<Vec3d> points3;
        bool is3D = false;
        bool convex = false;
        bool closed = false;

        double getTurn(Vec2d p0, Vec2d p1, Vec2d p2);

    public:
        VRPolygon();
        static std::shared_ptr<VRPolygon> create();

        void addPoint(Vec2d p);
        void addPoint(Vec3d p);
        Vec2d getPoint(int i);
        Vec3d getPoint3(int i);
        void remPoint(int i);
        void remPoint3(int i);
        void close();
        int size();
        int size2();
        int size3();
        void clear();

        bool isConvex();
        bool isCCW();
        void reverseOrder();
        void translate(Vec3d v);
        void scale(Vec3d s);

        vector<Vec2d>& get();
        vector<Vec3d>& get3();
        void set(vector<Vec2d> vec);
        VRPolygon sort();
        VRPolygon getConvexHull();
        Boundingbox getBoundingBox();
        vector< VRPolygon > getConvexDecomposition();
        vector< VRPolygonPtr > gridSplit(double G);
        void removeDoubles(float d = 1e-3);

        double computeArea();
        double getDistance(Vec3d p);
        VRPolygonPtr shrink(double amount);
        Vec3d getRandomPoint();
        vector<Vec3d> getRandomPoints(double density = 10, double padding = 0, double spread = 0.5);

        vector<Vec3d> toSpace(Matrix4d m);
        bool isInside(Vec2d p);
        bool isInside(Vec2d p, double& dist);
        bool areInside(vector<Vec2d> pv);

        string toString();
        PathPtr toPath();
        static void runTest();
};

OSG_END_NAMESPACE;

#endif // POLYGON_H_INCLUDED
