#ifndef FRUSTUM_H_INCLUDED
#define FRUSTUM_H_INCLUDED

#include "polygon.h"
#include "pose.h"
#include "VRMathFwd.h"
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPlane.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Frustum {
    private:
        Pose trans;
        vector<Vec3d> directions;
        VRPolygon profile;
        bool convex = false;
        Vec2d near_far;

        void computeProfile();
        Frustum fromProfile(VRPolygon p, Pose t);

        bool inFrustumPlanes(Vec3f p);

    public:
        Frustum();
        ~Frustum();

        static FrustumPtr create();

        void setPose(Pose trans);
        Pose getPose();
        void setNearFar(Vec2d near_far);
        void addEdge(Vec3d dir);
        void close();
        int size();
        void clear();

        Frustum getConvexHull();
        vector< Frustum > getConvexDecomposition();
        vector< Plane > getPlanes();
        vector< Plane > getNearFarPlanes();
        vector< Vec3d > getEdges();

        bool isInside(Vec3d p);

        string toString();
        static void runTest();
};

OSG_END_NAMESPACE;

#endif // FRUSTUM_H_INCLUDED
