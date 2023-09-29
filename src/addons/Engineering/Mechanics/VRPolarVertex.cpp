#include "VRPolarVertex.h"

using namespace OSG;

PolarCoords::PolarCoords(Vec3d a, Vec3d p0) {
    axis = a;
    dir0 = p0 - a*a.dot(p0);
    dir0.normalize();
    dir1 = dir0.cross(a);
    dir1.normalize();
}


PolarVertex::PolarVertex() {}
PolarVertex::PolarVertex(Vec3d v) : vertex(v) {}

Vec3d PolarVertex::getOrthogonal(Vec3d v) {
    if (abs(v[0]-v[1]) < 0.1) return Vec3d(v[0],-v[2],v[1]);
    return Vec3d(v[1],-v[0],v[2]);
}

double PolarVertex::computeRadius(PolarCoords& coords) {
    Vec3d proj = vertex - coords.axis*coords.axis.dot(vertex);
    return proj.length();
}

Vec2d PolarVertex::computePolar(PolarCoords& coords) {
    if (radius < 0) computeRadius(coords);

    Vec3d p = vertex;
    p.normalize();

    double angle = atan2(p.dot(coords.dir0), p.dot(coords.dir1));
    return Vec2d(angle, radius);
}

Vec2d PolarVertex::computeProfile(PolarCoords& coords) {
    //Vec3d o = getOrthogonal(coords.axis);
    //Vec3d o2 = o.cross(coords.axis);
    Vec3d p = vertex;
    Vec3d o = coords.dir0;
    Vec3d o2 = coords.dir1;

    p = p - o*o.dot(p);
    return Vec2d(coords.axis.dot(p), o2.dot(p));
}

void PolarVertex::computeAndSetAttributes(PolarCoords& coords) {
    profileCoords = computeProfile(coords); //TODO investigate why this needs to be run before computePolar()
    radius = computeRadius(coords);
    polarCoords = computePolar(coords);
}

void PolarVertex::setPlaneIndex(int i) { plane = i; }
