#include "isNan.h"

bool isNan(const OSG::Matrix& m) { return isNan(m[0]) || isNan(m[1])  || isNan(m[2])  || isNan(m[3]); }
bool isNan(const OSG::Matrix4d& m) { return isNan(m[0]) || isNan(m[1])  || isNan(m[2])  || isNan(m[3]); }
bool isNan(const OSG::Vec4f& v) { return v[0] != v[0] || v[1] != v[1] || v[2] != v[2] || v[3] != v[3]; }
bool isNan(const OSG::Vec4d& v) { return v[0] != v[0] || v[1] != v[1] || v[2] != v[2] || v[3] != v[3]; }
bool isNan(const OSG::Vec3d& v) { return v[0] != v[0] || v[1] != v[1] || v[2] != v[2]; }
bool isNan(const OSG::Vec2d& v) { return v[0] != v[0] || v[1] != v[1]; }
bool isNan(const float& f) { return f != f; }
