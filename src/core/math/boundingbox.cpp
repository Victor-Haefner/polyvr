#include "boundingbox.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include <OpenSG/OSGGeometry.h>

using namespace OSG;

boundingbox::boundingbox() { clear(); }

void boundingbox::clear() {
    cleared = true;
    float m = 1e6;
    bb1 = Vec3f(m,m,m);
    bb2 = Vec3f(-m,-m,-m);
}

void boundingbox::update(const Vec3f& v) {
    cleared = false;
    for (int i=0; i<3; i++) {
        if (v[i] < bb1[i]) bb1[i] = v[i];
        if (v[i] > bb2[i]) bb2[i] = v[i];
    }
}

void boundingbox::update(VRGeometryPtr g) {
    clear();
    auto pos = g->getMesh()->geo->getPositions();
    for (uint i=0; i<pos->size(); i++) {
        Vec3f p = pos->getValue<Pnt3f>(i).subZero();
        update(p);
    }
}

Vec3f boundingbox::getRandomPoint() {
    float x = float(rand())/RAND_MAX;
    float y = float(rand())/RAND_MAX;
    float z = float(rand())/RAND_MAX;
    Vec3f s = size();
    return bb1 + Vec3f(x*s[0], y*s[1], z*s[2]);
}

void boundingbox::update(const vector<Vec3f>& v) { for (auto p : v) update(p); }

bool boundingbox::empty() const { return cleared; }
Vec3f boundingbox::min() const { return bb1; }
Vec3f boundingbox::max() const { return bb2; }
Vec3f boundingbox::center() const { return cleared ? Vec3f() : (bb2+bb1)*0.5; }
Vec3f boundingbox::size() const { return cleared ? Vec3f() : bb2-bb1; }
float boundingbox::radius() const { return cleared ? 0 : (size()*0.5).length(); }

bool boundingbox::isInside(Vec3f p) const {
    return (p[0] <= bb2[0] && p[0] >= bb1[0]
         && p[1] <= bb2[1] && p[1] >= bb1[1]
         && p[2] <= bb2[2] && p[2] >= bb1[2]);
}

void boundingbox::move(const Vec3f& t) {
    bb1 += t;
    bb2 += t;
}

void boundingbox::setCenter(const Vec3f& t) {
    if (cleared) update(t);
    else move( t - center() );
}

void boundingbox::scale(float s) {
    Vec3f si = size();
    Vec3f sis = (si*s-si)*0.5;
    bb1 -= sis;
    bb2 += sis;
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
