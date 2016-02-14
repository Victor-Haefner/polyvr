#include "VRBRepEdge.h"

using namespace OSG;

VRBRepEdge::VRBRepEdge() {}

Vec3f& VRBRepEdge::beg() { return points.size() > 0 ? points[0] : n; }
Vec3f& VRBRepEdge::end() { return points.size() > 0 ? points[points.size()-1] : n; }

void VRBRepEdge::swap() { reverse(points.begin(), points.end()); }

bool VRBRepEdge::connectsTo(VRBRepEdge& e) { return ( sameVec(end(), e.beg()) ); }
