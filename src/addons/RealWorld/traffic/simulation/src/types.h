#ifndef TYPES_H
#define TYPES_H

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGQuaternion.h>
#include <inttypes.h>

using OSG::Vec2d;
using OSG::Vec3d;
using OSG::Quaterniond;

const double VEHICLE_LENGTH = 5;

const unsigned int CROSSROAD_RADIUS = 6;

// Needs to be uint64_t shortly but JSON can not hold long values.
typedef uint32_t ID;

// Okay, does not belong here. But I need this method everywhere...
inline double calcDistance(const Vec2d& a, const Vec2d& b) {
    return sqrt((a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]));
}

inline double calcDistance(const Vec2d& a, const Vec3d& b) {
    return calcDistance(a, Vec2d(b[0], b[2]));
}

inline double calcDistance(const Vec3d& a, const Vec2d& b) {
    return calcDistance(Vec2d(a[0], a[2]), b);
}

inline double calcDistance(const Vec3d& a, const Vec3d& b) {
    return calcDistance(Vec2d(a[0], a[2]), Vec2d(b[0], b[2]));
}

/**
 Calculates the direction the given vector is pointing in.
 The positive y-axis is considered as 0°, increasing clockwise.
 @param direction The vector to calculate the angle of.
 @return An angle in [0..360].
 */
inline double calcAngle(Vec2d direction) {

    double angle = atan2(direction[1], direction[0]);
    // To degree
    angle *= 180 / M_PI;
    // Move 0 degree to positive y-axis
    angle = ((int)angle + 360) % 360;
    return angle;
}


inline Vec2d toVec2d(const Vec3d& vec3) {
    return Vec2d(vec3[0], vec3[2]);
}

inline Vec3d toVec3d(const Vec2d& vec2) {
    return Vec3d(vec2[0], 0, vec2[1]);
}

#endif // TYPES_H
