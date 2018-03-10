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
        Pose trans;
        vector<Vec3d> directions;
        VRPolygon profile;
        bool convex = false;
        Vec2d near_far;

        void computeProfile();
        frustum fromProfile(VRPolygon p, Pose t);

    public:
        frustum();
        void setPose(Pose trans);
        Pose getPose();
        void setNearFar(Vec2d near_far);
        void addEdge(Vec3d dir);
        void close();
        int size();
        void clear();

        frustum getConvexHull();
        vector< frustum > getConvexDecomposition();
        vector< Plane > getPlanes();
        vector< Plane > getNearFarPlanes();
        vector< Vec3d > getEdges();

        string toString();
        static void runTest();
};

OSG_END_NAMESPACE;

#endif // FRUSTUM_H_INCLUDED
