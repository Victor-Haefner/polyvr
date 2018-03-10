#ifndef ISNAN_H_INCLUDED
#define ISNAN_H_INCLUDED

#include <OpenSG/OSGMatrix.h>

bool isNan(const OSG::Matrix& m);
bool isNan(const OSG::Matrix4d& m);
bool isNan(const OSG::Vec4f& v);
bool isNan(const OSG::Vec4d& v);
bool isNan(const OSG::Vec3d& v);
bool isNan(const OSG::Vec2d& v);
bool isNan(const float& f);

#endif // ISNAN_H_INCLUDED
