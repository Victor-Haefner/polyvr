#include "boundingbox.h"

using namespace OSG;

boundingbox::boundingbox() { clear(); }

void boundingbox::clear() {
    cleared = true;
    float m = 1e6;
    bb1 = Vec3f(m,m,m);
    bb2 = Vec3f(-m,-m,-m);
}

void boundingbox::update(Vec3f v) {
    cleared = false;
    for (int i=0; i<3; i++) {
        if (v[i] < bb1[i]) bb1[i] = v[i];
        if (v[i] > bb2[i]) bb2[i] = v[i];
    }
}

bool boundingbox::empty() { return cleared; }
Vec3f boundingbox::min() { return bb1; }
Vec3f boundingbox::max() { return bb2; }
Vec3f boundingbox::center() { return (bb2+bb1)*0.5; }
float boundingbox::radius() { return cleared ? 0 : (min()-center()).length(); }

void boundingbox::move(const Vec3f& t) {
    bb1 += t;
    bb2 += t;
}

bool boundingbox::intersectedBy(Line l) {
    Vec3f p0 = Vec3f(l.getPosition());
    Vec3f dir = l.getDirection();
    Vec3f dirfrac;

    dirfrac[0] = 1.0f / dir[0];
    dirfrac[1] = 1.0f / dir[1];
    dirfrac[2] = 1.0f / dir[2];

    // lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
    // r.org is origin of ray
    Vec3f bbp1 = bb1-p0;
    Vec3f bbp2 = bb2-p0;

    float t1 = bbp1[0]*dirfrac[0];
    float t3 = bbp1[1]*dirfrac[1];
    float t5 = bbp1[2]*dirfrac[2];

    float t2 = bbp2[0]*dirfrac[0];
    float t4 = bbp2[1]*dirfrac[1];
    float t6 = bbp2[2]*dirfrac[2];

    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    if (tmax < 0) return false; // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    if (tmin > tmax) return false; // if tmin > tmax, ray doesn't intersect AABB
    return true;
}
