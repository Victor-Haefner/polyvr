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

    float Da = abs(a2-a1);
    int N = Ncurv * Da/(2*Pi);
    float a = a1;
    angles.push_back(a1);
    for (int i=1; i<N; i++) {
        a = a1+i*(Da/N);
        angles.push_back(a);
    }
    angles.push_back(a2);

    // print
    /*cout << "angles: " << a1 << " " << a2 << " :";
    for (float a : angles) cout << " " << a;
    cout << endl;*/

    /*

    int i1 = getSide(a1);
    int i2 = getSide(a2);

    angles.push_back(a1);

    for (float a = i1*Dangle; a < i2*Dangle; a += Dangle) {
        cout << " angle " << a << endl;
        angles.push_back(a);
    }

    angles.push_back(a2);*/
    return angles;
}
