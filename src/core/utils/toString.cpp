#include "toString.h"
#include "core/math/pose.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRFunctionFwd.h"
#include <OpenSG/OSGColor.h>

using namespace OSG;

vector<string> splitString(string s, char c) {
    stringstream ss(s);
    string token;
    vector<string> res;
    while (std::getline(ss, token, c)) res.push_back(token);
    return res;
}

string toString(const string& s) { return s; }
string toString(const bool& b) { stringstream ss; ss << b; return ss.str(); }
string toString(const int& i) { stringstream ss; ss << i; return ss.str(); }
string toString(const size_t& i) { stringstream ss; ss << i; return ss.str(); }
string toString(const unsigned int& i) { stringstream ss; ss << i; return ss.str(); }
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

string toString(const Vec2f& v) {
    stringstream ss;
    ss << v[0] << " " << v[1];
    return ss.str();
}

string toString(const Pnt3f& v) { return toString(Vec3f(v)); }
string toString(const Vec3f& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
    return ss.str();
}

string toString(const Vec4f& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2] << " " << v[3];
    return ss.str();
}

string toString(const Vec2i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1];
    return ss.str();
}

string toString(const Vec3i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
    return ss.str();
}

string toString(const Vec4i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2] << " " << v[3];
    return ss.str();
}

template <class T>
T ssToVal(const string& s, int* N = 0) {
    stringstream ss;
    ss << s;
    if(N) *N = ss.tellg();
    T v = 0;
    ss >> v;
    if(N) *N = int(ss.tellg()) - *N;
    return v;
}

bool toBool(string s, int* N) { return ssToVal<bool>(s,N); }
int toInt(string s, int* N) { return ssToVal<int>(s,N); }
unsigned int toUInt(string s, int* N) { return ssToVal<unsigned int>(s,N); }
float toFloat(string s, int* N) { return ssToVal<float>(s,N); }
double toDouble(string s, int* N) { return ssToVal<double>(s,N); }

Vec2f toVec2f(string s) {
    Vec2f v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    return v;
}

Vec3f toVec3f(string s) {
    Vec3f v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return v;
}

Vec4f toVec4f(string s) {
    Vec4f v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    ss >> v[3];
    return v;
}

Pnt3f toPnt3f(string s) {
    Pnt3f v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return v;
}

Vec2i toVec2i(string s) {
    Vec2i v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    return v;
}

Vec3i toVec3i(string s) {
    Vec3i v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return v;
}

Vec4i toVec4i(string s) {
    Vec4i v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    ss >> v[3];
    return v;
}

string toString(const posePtr& po) {
    return toString(po->pos()) + " " + toString(po->dir()) + " " + toString(po->up());
}

string toString(const pose& po) {
    return toString(po.pos()) + " " + toString(po.dir()) + " " + toString(po.up());
}

string toString(const Boundingbox& b) {
    return toString(b.min()) + " " + toString(b.max()) + " " + toString(b.empty());
}

string toString(const Line& l) {
    return toString(l.getPosition()) + " " + toString(l.getDirection());
}

template<> string typeName(const string& t) { return "string"; }
template<> string typeName(const int& t) { return "int"; }
template<> string typeName(const float& t) { return "float"; }
template<> string typeName(const double& t) { return "double"; }
template<> string typeName(const bool& t) { return "bool"; }
template<> string typeName(const Vec2f& t) { return "Vec2f"; }
template<> string typeName(const Vec3f& t) { return "Vec3f"; }
template<> string typeName(const Vec4f& t) { return "Vec4f"; }
template<> string typeName(const Vec2d& t) { return "Vec2d"; }
template<> string typeName(const Vec3d& t) { return "Vec3d"; }
template<> string typeName(const Vec4d& t) { return "Vec4d"; }
template<> string typeName(const Color3f& t) { return "Vec3f"; }
template<> string typeName(const Color4f& t) { return "Vec4f"; }
template<> string typeName(const VRAnimCbPtr& t) { return "void callback(float)"; }
template<> string typeName(const Boundingbox& t) { return "Boundingbox"; }

template<> bool toValue(stringstream& ss, string& s) { s = ss.str(); return true; }
template<> bool toValue(stringstream& ss, bool& b) { return bool(ss >> b); }
template<> bool toValue(stringstream& ss, int& i) { return bool(ss >> i); }
template<> bool toValue(stringstream& ss, float& f) { return bool(ss >> f); }
template<> bool toValue(stringstream& ss, double& d) { return bool(ss >> d); }

template<> bool toValue(stringstream& ss, Vec2f& v) {
    ss >> v[0];
    return bool(ss >> v[1]);
}

template<> bool toValue(stringstream& ss, Vec3f& v) {
    ss >> v[0];
    ss >> v[1];
    return bool(ss >> v[2]);
}

template<> bool toValue(stringstream& ss, Vec4f& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return bool(ss >> v[3]);
}

template<> bool toValue(stringstream& ss, Vec2d& v) {
    ss >> v[0];
    return bool(ss >> v[1]);
}

template<> bool toValue(stringstream& ss, Vec3d& v) {
    ss >> v[0];
    ss >> v[1];
    return bool(ss >> v[2]);
}

template<> bool toValue(stringstream& ss, Vec4d& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return bool(ss >> v[3]);
}

template<> bool toValue(stringstream& ss, Vec2i& v) {
    ss >> v[0];
    return bool(ss >> v[1]);
}

template<> bool toValue(stringstream& ss, Vec3i& v) {
    ss >> v[0];
    ss >> v[1];
    return bool(ss >> v[2]);
}

template<> bool toValue(stringstream& ss, Color3f& v) {
    ss >> v[0];
    ss >> v[1];
    return bool(ss >> v[2]);
}

template<> bool toValue(stringstream& ss, Color4f& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return bool(ss >> v[3]);
}

template<> bool toValue(stringstream& ss, pose& po) {
    Vec3f p,d,u;
    toValue(ss, p);
    toValue(ss, d);
    bool b = toValue(ss, u);
    po.set(p,d,u);
    return b;
}

template<> bool toValue(stringstream& ss, posePtr& po) {
    Vec3f p,d,u;
    toValue(ss, p);
    toValue(ss, d);
    bool b = toValue(ss, u);
    if (po) po->set(p,d,u);
    else po = pose::create(p,d,u);
    return b;
}

template<> bool toValue(stringstream& ss, Boundingbox& box) {
    Vec3f a,b;
    bool c;
    toValue(ss, a);
    bool B = toValue(ss, b);
    ss >> c;
    box.update(a);
    box.update(b);
    if (c) box.clear();
    return B;
}
