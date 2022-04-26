#ifndef VRBREPEDGE_H_INCLUDED
#define VRBREPEDGE_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
#include "core/math/OSGMathFwd.h"
#include "VRBRepUtils.h"
#include "core/math/VRMathFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBRepEdge : public VRBRepUtils {
    public:
        vector<Vec3d> points;
        vector<Vec3d> cpoints;
        vector<float> angles;
        vector<double> weights;
        vector<double> knots;
        Vec3d n;
        Vec3d EBeg;
        Vec3d EEnd;
        double radius;
        PosePtr center;
        float a1,a2;
        int deg;
        bool fullCircle = false;
        bool swapped = false;
        string etype;

    public:
        VRBRepEdge();
        virtual ~VRBRepEdge();

        static VRBRepEdgePtr create();

        void setLine(Vec3d point1, Vec3d point2);
        void setCircle(PosePtr center, double radius, double angle1, double angle2);

        Vec3d& beg();
        Vec3d& end();
        void swap();
        bool connectsTo(VRBRepEdgePtr e);
        double compCircleDirection(Matrix4d mI, Vec3d d);

        void compute();
};

OSG_END_NAMESPACE;

#endif // VRBREPEDGE_H_INCLUDED
