#include "VRProfile.h"


OSG_BEGIN_NAMESPACE;

void VRProfile::add(Vec2f v) { pnts.push_back(v); }

vector<Vec3f> VRProfile::get(Vec3f n, Vec3f u) {
    vector<Vec3f> res;
    Vec3f v;
    Vec3f x = u.cross(n);
    for (uint i=0; i<pnts.size(); i++) {
        v = x*pnts[i][0] + u*pnts[i][1];
        res.push_back(v);
    }
    return res;
}

OSG_END_NAMESPACE;
