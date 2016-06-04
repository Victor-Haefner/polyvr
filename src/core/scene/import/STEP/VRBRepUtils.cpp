#include "VRBRepUtils.h"

using namespace OSG;

const float VRBRepUtils::Dangle = 2*Pi/(VRBRepUtils::Ncurv-1);
vector<float> VRBRepUtils::Adict;

VRBRepUtils::VRBRepUtils() {
    for (int i=0; i<Ncurv; i++) Adict.push_back(Dangle*i);
}

bool VRBRepUtils::sameVec(const Vec3f& v1, const Vec3f& v2, float d) {
    Vec3f dv = v2-v1;
    return ( abs(dv[0]) < d && abs(dv[1]) < d && abs(dv[2]) < d );
}

int VRBRepUtils::getSideN(float a) {
    return round( a/Dangle - 0.5 );
}

Vec2f VRBRepUtils::getSide(float a) {
    int i = getSideN(a);
    return getSide(i);
}

Vec2f VRBRepUtils::getSide(int i) {
    return Vec2f(i*Dangle, (i+1)*Dangle);
}

vector<float> VRBRepUtils::angleFrame(float a1, float a2) {
    if (a1 > a2) a2 += 2*Pi;
    vector<float> angles;
    angles.push_back(a1);
    int s1 = getSideN(a1);
    float a = s1*Dangle;
    while (a <= a1) a += Dangle;
    for(; a < a2; a += Dangle) angles.push_back(a);
    angles.push_back(a2);
    return angles;
}

float VRBRepUtils::Bik(float t, int i, int k, const vector<double>& knots) {
    float ti = knots[i];
    float ti1 = knots[i+1];
    float tik = knots[i+k];
    float tik1 = knots[i+k+1];
    float tL = knots[knots.size()-1];
    if (k == 0) {
        if (t >= ti && t <= ti1) return 1;
        if (t == ti1 && t == tL) return 1;
        else return 0;
    }
    float A = tik == ti ? 0 : Bik(t, i, k-1, knots)*(t-ti)/(tik-ti);
    float B = tik1 == ti1 ? 0 : Bik(t, i+1, k-1, knots)*(tik1 - t)/(tik1 - ti1);
    return A + B;
}

Vec3f VRBRepUtils::BSpline(float t, int deg, const vector<Vec3f>& cpoints, const vector<double>& knots) {
    Vec3f p;
    for (int i=0; i<cpoints.size(); i++) p += cpoints[i]*Bik(t, i, deg, knots);
    return p;
}

Vec3f VRBRepUtils::BSplineW(float t, int deg, const vector<Vec3f>& cpoints, const vector<double>& knots, const vector<double>& weights) {
    Vec3f p;
    float W = 0;
    for (int i=0; i<cpoints.size(); i++) W += Bik(t, i, deg, knots)*weights[i];
    for (int i=0; i<cpoints.size(); i++) p += cpoints[i]*Bik(t, i, deg, knots)*weights[i]/W;
    return p;
}

Vec3f VRBRepUtils::BSpline(float u, float v, int degu, int degv, const field<Vec3f>& cpoints) {
    return Vec3f();
}


