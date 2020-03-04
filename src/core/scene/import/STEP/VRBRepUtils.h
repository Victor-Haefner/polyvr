#ifndef VRBREPUTILS_H_INCLUDED
#define VRBREPUTILS_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include "core/math/field.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRBRepUtils {
    protected:

        static const int Ncurv = 17;//17; // N in 2pi - should be multiple of 4 +1
        static const float Dangle; // angle segment
        static vector<float> Adict; // all angles

        bool sameVec(const Vec3d& v1, const Vec3d& v2, float d = 1e-5);
        vector<float> angleFrame(float a1, float a2);
        int getSideN(double a);
        int getSideN(float a);
        Vec2d getSide(double a);
        Vec2d getSide(float a);
        Vec2d getSide(int i);

        // B Splines
        float Bik(float t, int i, int k, const vector<double>& knots, bool verbose = 0);
        Vec3d BSpline(float t, int deg, const vector<Vec3d>& cpoints, const vector<double>& knots);
        Vec3d BSplineW(float t, int deg, const vector<Vec3d>& cpoints, const vector<double>& knots, const vector<double>& weights);
        Vec3d BSpline(float u, float v, int degu, int degv, const field<Vec3d>& cpoints, const vector<double>& knotsu, const vector<double>& knotsv);
        Vec3d BSplineNorm(float u, float v, int degu, int degv, const field<Vec3d>& cpoints, const vector<double>& knotsu, const vector<double>& knotsv);
        Vec3d BSpline(float u, float v, int degu, int degv, const field<Vec3d>& cpoints, const vector<double>& knotsu, const vector<double>& knotsv, const field<double>& weights);
        Vec3d BSplineNorm(float u, float v, int degu, int degv, const field<Vec3d>& cpoints, const vector<double>& knotsu, const vector<double>& knotsv, const field<double>& weights);

    public:
        VRBRepUtils();
};

OSG_END_NAMESPACE;

#endif // VRBREPUTILS_H_INCLUDED
