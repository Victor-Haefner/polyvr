#ifndef FRUSTUM_H_INCLUDED
#define FRUSTUM_H_INCLUDED

#include "core/math/VRMathFwd.h"
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPlane.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Frustum {
    private:
        PosePtr trans;
        vector<Vec3d> directions;
        VRPolygonPtr profile;
        bool convex = false;
        Vec2d near_far;

        void computeProfile();
        FrustumPtr fromProfile(VRPolygonPtr p, PosePtr t);

        bool inFrustumPlanes(Vec3f p);

    public:
        Frustum();
        ~Frustum();

        static FrustumPtr create();

        void setPose(PosePtr trans);
        PosePtr getPose();
        void setNearFar(Vec2d near_far);
        void addEdge(Vec3d dir);
        void close();
        int size();
        void clear();

        FrustumPtr getConvexHull();
        vector< FrustumPtr > getConvexDecomposition();
        vector< Plane > getPlanes();
        vector< Plane > getNearFarPlanes();
        vector< Vec3d > getEdges();

        bool isInside(Vec3d p);

        string toString();
        static void runTest();
};

OSG_END_NAMESPACE;

#endif // FRUSTUM_H_INCLUDED
