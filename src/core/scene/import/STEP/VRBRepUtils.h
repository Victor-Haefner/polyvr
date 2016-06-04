#ifndef VRBREPUTILS_H_INCLUDED
#define VRBREPUTILS_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/math/field.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBRepUtils {
    protected:

        static const int Ncurv = 17;//17; // N in 2pi - should be multiple of 4 +1
        static const float Dangle; // angle segment
        static vector<float> Adict; // all angles

        bool sameVec(const Vec3f& v1, const Vec3f& v2, float d = 1e-5);
        vector<float> angleFrame(float a1, float a2);
        int getSideN(float a);
        Vec2f getSide(float a);
        Vec2f getSide(int i);

        // B Splines
        float Bik(float t, int i, int k, const vector<double>& knots);
        Vec3f BSpline(float t, int deg, const vector<Vec3f>& cpoints, const vector<double>& knots);
        Vec3f BSplineW(float t, int deg, const vector<Vec3f>& cpoints, const vector<double>& knots, const vector<double>& weights);
        Vec3f BSpline(float u, float v, int degu, int degv, const field<Vec3f>& cpoints);

    public:
        VRBRepUtils();
};

OSG_END_NAMESPACE;

#endif // VRBREPUTILS_H_INCLUDED
