#include "VRPolygonSelection.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace OSG;

VRPolygonSelection::VRPolygonSelection() {}

void VRPolygonSelection::setOrigin(pose orig) { selection.setPose(orig); }
void VRPolygonSelection::addEdge(Vec3f dir) { selection.addEdge(dir); needs_update = true; }
void VRPolygonSelection::close() { selection.close(); }

void VRPolygonSelection::clear() {
    VRSelection::clear();
    selection.clear();
    convex_hull.clear();
}

bool VRPolygonSelection::objSelected(VRGeometryPtr geo) {
    Vec3f v1,v2;
    geo->getBoundingBox(v1,v2);
    vector<Vec3f> corners;
    corners.push_back(v1);
    corners.push_back(Vec3f(v1[0], v1[1], v2[2]));
    corners.push_back(Vec3f(v1[0], v2[1], v2[2]));
    corners.push_back(v2);
    corners.push_back(Vec3f(v1[0], v2[1], v1[2]));
    corners.push_back(Vec3f(v2[0], v2[1], v1[2]));
    corners.push_back(Vec3f(v2[0], v1[1], v2[2]));
    corners.push_back(Vec3f(v2[0], v1[1], v1[2]));

    for (auto c : corners) if (!vertSelected(c)) return false;
    return true;
}

bool VRPolygonSelection::partialSelected(VRGeometryPtr geo) {
    Vec3f v1,v2;
    geo->getBoundingBox(v1,v2);
    vector<Vec3f> corners;
    corners.push_back(v1);
    corners.push_back(Vec3f(v1[0], v1[1], v2[2]));
    corners.push_back(Vec3f(v1[0], v2[1], v2[2]));
    corners.push_back(v2);
    corners.push_back(Vec3f(v1[0], v2[1], v1[2]));
    corners.push_back(Vec3f(v2[0], v2[1], v1[2]));
    corners.push_back(Vec3f(v2[0], v1[1], v2[2]));
    corners.push_back(Vec3f(v2[0], v1[1], v1[2]));

    for (auto c : corners) if (vertSelected(c)) return true;
    return false;
}

bool VRPolygonSelection::vertSelected(Vec3f p) {
    if (needs_update) {
        needs_update = false;
        convex_hull = selection.getConvexHull();
    }

    auto planes = convex_hull.getPlanes();
    for (Plane pl : planes) if ( pl.distance(p) < 0) return false;

    for (auto f : selection.getConvexDecomposition() ) {
        for (Plane pl : f.getPlanes()) if ( pl.distance(p) < 0) return false;
    }

    return true;
}
