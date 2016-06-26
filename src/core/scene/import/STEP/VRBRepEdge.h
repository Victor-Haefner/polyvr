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
        vector<Vec3f> points;
        vector<Vec3f> cpoints;
        vector<float> angles;
        vector<double> weights;
        vector<double> knots;
        Vec3f n, EBeg, EEnd;
        double radius;
        posePtr center;
        int deg;
        bool fullCircle = false;

        VRBRepEdge();

        Vec3f& beg();
        Vec3f& end();
        void swap();
        bool connectsTo(VRBRepEdge& e);

        void build(string type);
};

OSG_END_NAMESPACE;

#endif // VRBREPEDGE_H_INCLUDED
