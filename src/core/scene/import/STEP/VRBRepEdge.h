#ifndef VRBREPEDGE_H_INCLUDED
#define VRBREPEDGE_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
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
        Vec3d n, EBeg, EEnd;
        double radius;
        PosePtr center;
        float a1,a2;
        int deg;
        bool fullCircle = false;
        string etype;

        VRBRepEdge();

        Vec3d& beg();
        Vec3d& end();
        void swap();
        bool connectsTo(VRBRepEdge& e);

        void build(string type);
};

OSG_END_NAMESPACE;

#endif // VRBREPEDGE_H_INCLUDED
