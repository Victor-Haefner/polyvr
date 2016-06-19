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
    if (a1 >= a2) a2 += 2*Pi;
    vector<float> angles;
    angles.push_back(a1);
    int s1 = getSideN(a1);
    float a = s1*Dangle;
    while (a <= a1) a += Dangle;
    for(; a < a2; a += Dangle) angles.push_back(a);
    angles.push_back(a2);
    return angles;
}

float VRBRepUtils::Bik(float t, int i, int k, const vector<double>& knots, bool verbose) {
    if (verbose) cout << "Bik t: " << t << " i: " << i << " k: " << k << endl;
    float ti = knots[i];
    float ti1 = knots[i+1];
    float tik = knots[i+k];
    float tik1 = knots[i+k+1];
    float tL = knots[knots.size()-1];
    if (verbose) cout << " Bik ti: " << ti << " ti1: " << ti1 << " tik: " << tik << " tik1: " << tik1 << endl;
    if (k == 0) {
        if (t >= ti && t <= ti1) { if (verbose) cout << " Bik exit first 1" << endl; return 1; }
        if (t == ti1 && t == tL) { if (verbose) cout << " Bik exit second 1" << endl; return 1; }
        else { if (verbose) cout << " Bik exit 0" << endl; return 0; }
    }
    float A = tik == ti ? 0 : Bik(t, i, k-1, knots, verbose)*(t-ti)/(tik-ti);
    float B = tik1 == ti1 ? 0 : Bik(t, i+1, k-1, knots, verbose)*(tik1 - t)/(tik1 - ti1);
    if (verbose) cout << " Bik return A " << A << "(" << (tik == ti) << ")" << " B " << B << "(" << (tik1 == ti1) << ")" << endl;
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

Vec3f VRBRepUtils::BSpline(float u, float v, int degu, int degv, const field<Vec3f>& cpoints, const vector<double>& knotsu, const vector<double>& knotsv) {
    Vec3f p;
    for (int i=0; i<cpoints.width; i++) {
        for (int j=0; j<cpoints.height; j++) {
            Vec3f cp = cpoints.get(i,j);
            p += cp*Bik(u, j, degu, knotsu)*Bik(v, i, degv, knotsv);
            //cout << " VRBRepUtils::BSpline Bik(u) " << Bik(u, j, degu, knotsu,1) << endl;
        }
    }
    //cout << "VRBRepUtils::BSpline u,v " << Vec2f(u, v) << "\t p " << p << endl;
    return p;
}

Vec3f VRBRepUtils::BSplineNorm(float u, float v, int degu, int degv, const field<Vec3f>& cpoints, const vector<double>& knotsu, const vector<double>& knotsv) {
    Vec3f du, dv;

    // derivative in u
    for (int i=0; i<cpoints.width; i++) {
        for (int j=0; j<cpoints.height-1; j++) {
            float ti1 = knotsu[j+1];
            float tik1 = knotsu[j+degu+1];
            Vec3f cp = (cpoints.get(i,j+1) - cpoints.get(i,j))*degu/(tik1-ti1);
            du += cp*Bik(u, j+1, degu-1, knotsu)*Bik(v, i, degv, knotsv);
        }
    }

    // derivative in v
    for (int i=0; i<cpoints.width-1; i++) {
        for (int j=0; j<cpoints.height; j++) {
            float ti1 = knotsv[i+1];
            float tik1 = knotsv[i+degv+1];
            Vec3f cp = (cpoints.get(i+1,j) - cpoints.get(i,j))*degu/(tik1-ti1);
            dv += cp*Bik(u, j, degu, knotsu)*Bik(v, i+1, degv-1, knotsv);
        }
    }

    return dv.cross(du);
}

Vec3f VRBRepUtils::BSpline(float u, float v, int degu, int degv, const field<Vec3f>& cpoints, const vector<double>& knotsu, const vector<double>& knotsv, const field<double>& weights) {
    float W = 0;
    for (int i=0; i<cpoints.width; i++) {
        for (int j=0; j<cpoints.height; j++) {
            W += Bik(u, j, degu, knotsu)*Bik(v, i, degv, knotsv)*weights.get(i,j);
        }
    }

    Vec3f p;
    for (int i=0; i<cpoints.width; i++) {
        for (int j=0; j<cpoints.height; j++) {
            Vec3f cp = cpoints.get(i,j)*weights.get(i,j)/W;
            p += cp*Bik(u, j, degu, knotsu)*Bik(v, i, degv, knotsv);
        }
    }
    return p;
}

Vec3f VRBRepUtils::BSplineNorm(float u, float v, int degu, int degv, const field<Vec3f>& cpoints, const vector<double>& knotsu, const vector<double>& knotsv, const field<double>& weights) {
    float W = 0;
    for (int i=0; i<cpoints.width; i++) {
        for (int j=0; j<cpoints.height; j++) {
            W += Bik(u, j, degu, knotsu)*Bik(v, i, degv, knotsv)*weights.get(i,j);
        }
    }

    Vec3f du, dv;

    // derivative in u
    for (int i=0; i<cpoints.width; i++) {
        for (int j=0; j<cpoints.height-1; j++) {
            float ti1 = knotsu[j+1];
            float tik1 = knotsu[j+degu+1];
            Vec3f cp = (cpoints.get(i,j+1)/*weights.get(i,j+1)/W*/ - cpoints.get(i,j)/*weights.get(i,j)/W*/)*degu/(tik1-ti1);
            du += cp*Bik(u, j+1, degu-1, knotsu)*Bik(v, i, degv, knotsv);
        }
    }

    // derivative in v
    for (int i=0; i<cpoints.width-1; i++) {
        for (int j=0; j<cpoints.height; j++) {
            float ti1 = knotsv[i+1];
            float tik1 = knotsv[i+degv+1];
            Vec3f cp = (cpoints.get(i+1,j)/*weights.get(i+1,j)/W*/ - cpoints.get(i,j)/*weights.get(i,j)/W*/)*degu/(tik1-ti1);
            dv += cp*Bik(u, j, degu, knotsu)*Bik(v, i+1, degv-1, knotsv);
        }
    }

    return dv.cross(du);
}


