#ifndef FRUSTUM_H_INCLUDED
#define FRUSTUM_H_INCLUDED

#include "polygon.h"
#include "pose.h"
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPlane.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class frustum {
    private:
        pose trans;
        vector<Vec3f> directions;
        Polygon profile;
        bool convex = false;
        Vec2f near_far;

        void computeProfile();
        frustum fromProfile(Polygon p, pose t);

    public:
        frustum();
        void setPose(pose trans);
        pose getPose();
        void setNearFar(Vec2f near_far);
        void addEdge(Vec3f dir);
        void close();
        int size();
        void clear();

        frustum getConvexHull();
        vector< frustum > getConvexDecomposition();
        vector< Plane > getPlanes();
        vector< Plane > getNearFarPlanes();
        vector< Vec3f > getEdges();

        string toString();
        static void runTest();
};

OSG_END_NAMESPACE;

#endif // FRUSTUM_H_INCLUDED
