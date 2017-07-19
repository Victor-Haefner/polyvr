#include "VRProfile.h"


OSG_BEGIN_NAMESPACE;

void VRProfile::add(Vec2d v) { pnts.push_back(v); }

vector<Vec3d> VRProfile::get(Vec3d n, Vec3d u) {
    vector<Vec3d> res;
    Vec3d v;
    Vec3d x = u.cross(n);
    for (uint i=0; i<pnts.size(); i++) {
        v = x*pnts[i][0] + u*pnts[i][1];
        res.push_back(v);
    }
    return res;
}

OSG_END_NAMESPACE;
