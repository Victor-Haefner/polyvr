#include "VRSpaceWarper.h"
#include "../objects/VRCamera.h"

using namespace OSG;

VRSpaceWarper::VRSpaceWarper() {}
VRSpaceWarper::~VRSpaceWarper() {}

VRSpaceWarperPtr VRSpaceWarper::create() { return VRSpaceWarperPtr(new VRSpaceWarper()); }
VRSpaceWarperPtr VRSpaceWarper::ptr() { return shared_from_this(); }

void VRSpaceWarper::warp(Matrix4d& m) {
    auto p = Vec3d(m[3]);
    bool doWarp = false;
    static const double k = 4;
    if (abs(p[0]) > k) doWarp = true;
    if (abs(p[1]) > k) doWarp = true;
    if (abs(p[2]) > k) doWarp = true;
    if (!doWarp) return;

    auto sign = [](const double& d) { return d < 0 ? -1 : 1; };

    // warp position
    Vec3d P, K;
    P[0] = sign(p[0])*log(abs(p[0])); K[0] = P[0]/p[0];
    P[1] = sign(p[1])*log(abs(p[1])); K[1] = P[1]/p[1];
    P[2] = sign(p[2])*log(abs(p[2])); K[2] = P[2]/p[2];
    m.setTranslate(P);

    // warp scale
    Vec3d s, S;
    s[0] = m[0].length();
    s[1] = m[1].length();
    s[2] = m[2].length();
    Matrix4d ms;
    S = Vec3d(s[0]*K[0], s[1]*K[1], s[2]*K[2]);
    ms.setScale(S);
    cout << "VRSpaceWarper::warp p " << p << " -> " << P << " s " << s << " -> " << S << endl;
    m.mult(ms);
}
