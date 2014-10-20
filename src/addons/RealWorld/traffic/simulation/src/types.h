#ifndef TYPES_H
#define TYPES_H

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGQuaternion.h>
#include <inttypes.h>

using OSG::Vec2f;
using OSG::Vec3f;
using OSG::Quaternion;

const double VEHICLE_LENGTH = 5;

const unsigned int CROSSROAD_RADIUS = 6;

// Needs to be uint64_t shortly but JSON can not hold long values.
typedef uint32_t ID;

// Okay, does not belong here. But I need this method everywhere...
inline double calcDistance(const Vec2f& a, const Vec2f& b) {
    return sqrt((a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]));
}

inline double calcDistance(const Vec2f& a, const Vec3f& b) {
    return calcDistance(a, Vec2f(b[0], b[2]));
}

inline double calcDistance(const Vec3f& a, const Vec2f& b) {
    return calcDistance(Vec2f(a[0], a[2]), b);
}

inline double calcDistance(const Vec3f& a, const Vec3f& b) {
    return calcDistance(Vec2f(a[0], a[2]), Vec2f(b[0], b[2]));
}

/**
 Calculates the direction the given vector is pointing in.
 The positive y-axis is considered as 0°, increasing clockwise.
 @param direction The vector to calculate the angle of.
 @return An angle in [0..360].
 */
inline double calcAngle(Vec2f direction) {

    double angle = atan2(direction[1], direction[0]);
    // To degree
    angle *= 180 / M_PI;
    // Move 0 degree to positive y-axis
    angle = ((int)angle + 360) % 360;
    return angle;
}


inline Vec2f toVec2f(const Vec3f& vec3) {
    return Vec2f(vec3[0], vec3[2]);
}

inline Vec3f toVec3f(const Vec2f& vec2) {
    return Vec3f(vec2[0], 0, vec2[1]);
}

#endif // TYPES_H
