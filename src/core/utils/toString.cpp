#include "toString.h"
#include "core/math/pose.h"
#include "core/math/boundingbox.h"
#include <OpenSG/OSGColor.h>

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

string toString(const OSG::Vec2f& v) {
    stringstream ss;
    ss << v[0] << " " << v[1];
    return ss.str();
}

string toString(const OSG::Pnt3f& v) { return toString(OSG::Vec3f(v)); }
string toString(const OSG::Vec3f& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
    return ss.str();
}

string toString(const OSG::Vec4f& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2] << " " << v[3];
    return ss.str();
}

string toString(const OSG::Vec2i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1];
    return ss.str();
}

string toString(const OSG::Vec3i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
    return ss.str();
}

string toString(const OSG::Vec4i& v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2] << " " << v[3];
    return ss.str();
}

template <class T>
T ssToVal(const string& s, int* N = 0) {
    stringstream ss;
    ss << s;
    if(N) *N = ss.tellg();
    T v;
    ss >> v;
    if(N) *N = int(ss.tellg()) - *N;
    return v;
}

bool toBool(string s, int* N) { return ssToVal<bool>(s,N); }
int toInt(string s, int* N) { return ssToVal<int>(s,N); }
unsigned int toUInt(string s, int* N) { return ssToVal<unsigned int>(s,N); }
float toFloat(string s, int* N) { return ssToVal<float>(s,N); }
double toDouble(string s, int* N) { return ssToVal<double>(s,N); }

OSG::Vec2f toVec2f(string s) {
    OSG::Vec2f v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    return v;
}

OSG::Vec3f toVec3f(string s) {
    OSG::Vec3f v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return v;
}

OSG::Vec4f toVec4f(string s) {
    OSG::Vec4f v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    ss >> v[3];
    return v;
}

OSG::Pnt3f toPnt3f(string s) {
    OSG::Pnt3f v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return v;
}

OSG::Vec2i toVec2i(string s) {
    OSG::Vec2i v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    return v;
}

OSG::Vec3i toVec3i(string s) {
    OSG::Vec3i v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return v;
}

OSG::Vec4i toVec4i(string s) {
    OSG::Vec4i v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    ss >> v[3];
    return v;
}

string toString(const OSG::posePtr& po) {
    return toString(po->pos()) + " " + toString(po->dir()) + " " + toString(po->up());
}

string toString(const OSG::pose& po) {
    return toString(po.pos()) + " " + toString(po.dir()) + " " + toString(po.up());
}

string toString(const OSG::boundingbox& b) {
    return toString(b.min()) + " " + toString(b.max()) + " " + toString(b.empty());
}

string toString(const OSG::Line& l) {
    return toString(l.getPosition()) + " " + toString(l.getDirection());
}

template<> void toValue(stringstream& ss, string& s) { s = ss.str(); }
template<> void toValue(stringstream& ss, bool& b) { ss >> b; }
template<> void toValue(stringstream& ss, int& i) { ss >> i; }
template<> void toValue(stringstream& ss, float& f) { ss >> f; }

template<> void toValue(stringstream& ss, OSG::Vec2f& v) {
    ss >> v[0];
    ss >> v[1];
}

template<> void toValue(stringstream& ss, OSG::Vec3f& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
}

template<> void toValue(stringstream& ss, OSG::Vec4f& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    ss >> v[3];
}

template<> void toValue(stringstream& ss, OSG::Vec2i& v) {
    ss >> v[0];
    ss >> v[1];
}

template<> void toValue(stringstream& ss, OSG::Vec3i& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
}

template<> void toValue(stringstream& ss, OSG::Color3f& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
}

template<> void toValue(stringstream& ss, OSG::Color4f& v) {
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    ss >> v[3];
}

template<> void toValue(stringstream& ss, OSG::pose& po) {
    OSG::Vec3f p,d,u;
    toValue(ss, p);
    toValue(ss, d);
    toValue(ss, u);
    po.set(p,d,u);
}

template<> void toValue(stringstream& ss, OSG::posePtr& po) {
    OSG::Vec3f p,d,u;
    toValue(ss, p);
    toValue(ss, d);
    toValue(ss, u);
    if (po) po->set(p,d,u);
    else po = OSG::pose::create(p,d,u);
}

template<> void toValue(stringstream& ss, OSG::boundingbox& box) {
    OSG::Vec3f a,b;
    bool c;
    toValue(ss, a);
    toValue(ss, b);
    ss >> c;
    box.update(a);
    box.update(b);
    if (c) box.clear();
}
