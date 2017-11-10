#include "toString.h"
#include "core/math/VRMathFwd.h"
#include "core/math/pose.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRFunctionFwd.h"
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGLine.h>

using namespace OSG;

vector<string> splitString(string s, char c) {
    stringstream ss(s);
    string token;
    vector<string> res;
    while (std::getline(ss, token, c)) res.push_back(token);
    return res;
}

typedef void* voidPtr;

template<> string toString(const string& s) { return s; }
template<> string toString(const bool& b) { stringstream ss; ss << b; return ss.str(); }
template<> string toString(const int& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const unsigned char& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const voidPtr& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const size_t& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const unsigned int& i) { stringstream ss; ss << i; return ss.str(); }
string toString(const float& f, int d) {
    stringstream ss;
    if (d >= 0) ss << fixed << setprecision(d);//ss.precision(d);
    ss << f;
    return ss.str();
}
string toString(const double& f, int d) {
    stringstream ss;
    if (d >= 0) ss << fixed << setprecision(d);//ss.precision(d);
    ss << f;
    return ss.str();
}

template<> string toString(const Vec2d& v) {
    stringstream ss;
    ss << v[0] << " " << v[1];
    return ss.str();
}

template<> string toString(const Vec3d& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
    return ss.str();
}

template<> string toString(const Vec4d& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2] << " " << v[3];
    return ss.str();
}

template<> string toString(const Pnt2d& v) { return toString(Vec2d(v)); }
template<> string toString(const Pnt3d& v) { return toString(Vec3d(v)); }
template<> string toString(const Pnt4d& v) { return toString(Vec4d(v)); }

template<> string toString(const Color3f& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
    return ss.str();
}

template<> string toString(const Color4f& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2] << " " << v[3];
    return ss.str();
}

template<> string toString(const Vec2i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1];
    return ss.str();
}

template<> string toString(const Vec3i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
    return ss.str();
}

template<> string toString(const Vec4i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2] << " " << v[3];
    return ss.str();
}

template<> string toString(const PosePtr& po) {
    return toString(po->pos()) + " " + toString(po->dir()) + " " + toString(po->up());
}

template<> string toString(const Pose& po) {
    return toString(po.pos()) + " " + toString(po.dir()) + " " + toString(po.up());
}

template<> string toString(const Boundingbox& b) {
    return toString(b.min()) + " " + toString(b.max()) + " " + toString(b.empty());
}

template<> string toString(const Line& l) {
    return toString(Pnt3d(l.getPosition())) + " " + toString(Vec3d(l.getDirection()));
}

template<> string typeName(const string& t) { return "string"; }
template<> string typeName(const int& t) { return "int"; }
template<> string typeName(const unsigned int& t) { return "int"; }
template<> string typeName(const float& t) { return "float"; }
template<> string typeName(const double& t) { return "double"; }
template<> string typeName(const bool& t) { return "bool"; }
template<> string typeName(const Pnt2d& t) { return "Pnt2d"; }
template<> string typeName(const Pnt3d& t) { return "Pnt3d"; }
template<> string typeName(const Pnt4d& t) { return "Pnt4d"; }
template<> string typeName(const Vec2d& t) { return "Vec2d"; }
template<> string typeName(const Vec3d& t) { return "Vec3d"; }
template<> string typeName(const Vec4d& t) { return "Vec4d"; }
template<> string typeName(const Vec2i& t) { return "Vec2i"; }
template<> string typeName(const Vec3i& t) { return "Vec3i"; }
template<> string typeName(const Color3f& t) { return "Vec3d"; }
template<> string typeName(const Color4f& t) { return "Vec4d"; }
template<> string typeName(const VRAnimCbPtr& t) { return "void callback(float)"; }
template<> string typeName(const Boundingbox& t) { return "Boundingbox"; }

template <typename T> int ssToVal(stringstream& ss, T& t) {
    int N = ss.tellg();
    ss >> t;
    return int(ss.tellg()) - N;
}

template<> int toValue(stringstream& ss, string& s) { s = ss.str(); return true; }
template<> int toValue(stringstream& ss, bool& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, int& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, unsigned int& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, float& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, double& v) { return ssToVal(ss, v); }

int   toInt  (string s) { return toValue<int  >(s); }
float toFloat(string s) { return toValue<float>(s); }

template<> int toValue(stringstream& ss, Vec2d& v) {
    ss >> v[0];
    return bool(ss >> v[1]);
}

template<> int toValue(stringstream& ss, Vec3d& v) {
    ss >> v[0];
    ss >> v[1];
    return bool(ss >> v[2]);
}

template<> int toValue(stringstream& ss, Vec4d& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return bool(ss >> v[3]);
}

template<> int toValue(stringstream& ss, Pnt2d& v) {
    ss >> v[0];
    return bool(ss >> v[1]);
}

template<> int toValue(stringstream& ss, Pnt3d& v) {
    ss >> v[0];
    ss >> v[1];
    return bool(ss >> v[2]);
}

template<> int toValue(stringstream& ss, Pnt4d& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return bool(ss >> v[3]);
}

template<> int toValue(stringstream& ss, Vec2i& v) {
    ss >> v[0];
    return bool(ss >> v[1]);
}

template<> int toValue(stringstream& ss, Vec3i& v) {
    ss >> v[0];
    ss >> v[1];
    return bool(ss >> v[2]);
}

template<> int toValue(stringstream& ss, Vec4i& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return bool(ss >> v[3]);
}

template<> int toValue(stringstream& ss, Color3f& v) {
    ss >> v[0];
    ss >> v[1];
    return bool(ss >> v[2]);
}

template<> int toValue(stringstream& ss, Color4f& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return bool(ss >> v[3]);
}

template<> int toValue(stringstream& ss, Pose& po) {
    Vec3d p,d,u;
    toValue(ss, p);
    toValue(ss, d);
    bool b = toValue(ss, u);
    po.set(p,d,u);
    return b;
}

template<> int toValue(stringstream& ss, PosePtr& po) {
    Vec3d p,d,u;
    toValue(ss, p);
    toValue(ss, d);
    bool b = toValue(ss, u);
    if (po) po->set(p,d,u);
    else po = Pose::create(p,d,u);
    return b;
}

template<> int toValue(stringstream& ss, Boundingbox& box) {
    Vec3d a,b;
    bool c;
    toValue(ss, a);
    bool B = toValue(ss, b);
    ss >> c;
    box.update(a);
    box.update(b);
    if (c) box.clear();
    return B;
}



