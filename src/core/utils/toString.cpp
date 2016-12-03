#include "toString.h"
#include "core/math/pose.h"

vector<string> splitString(string s, char c) {
    stringstream ss(s);
    string token;
    vector<string> res;
    while (std::getline(ss, token, c)) res.push_back(token);
    return res;
}

string toString(string s) { return s; }
string toString(bool b) { stringstream ss; ss << b; return ss.str(); }
string toString(int i) { stringstream ss; ss << i; return ss.str(); }
string toString(size_t i) { stringstream ss; ss << i; return ss.str(); }
string toString(unsigned int i) { stringstream ss; ss << i; return ss.str(); }
string toString(float f, int d) {
    stringstream ss;
    if (d >= 0) ss << fixed << setprecision(d);//ss.precision(d);
    ss << f;
    return ss.str();
}
string toString(double f, int d) {
    stringstream ss;
    if (d >= 0) ss << fixed << setprecision(d);//ss.precision(d);
    ss << f;
    return ss.str();
}

string toString(OSG::Vec2f v) {
    stringstream ss;
    ss << v[0] << " " << v[1];
    return ss.str();
}

string toString(OSG::Pnt3f v) { return toString(OSG::Vec3f(v)); }
string toString(OSG::Vec3f v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
    return ss.str();
}

string toString(OSG::Vec4f v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2] << " " << v[3];
    return ss.str();
}

string toString(OSG::Vec3i v) {
    stringstream ss;
    ss << v[0] << " " << v[1] << " " << v[2];
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

OSG::Vec3i toVec3i(string s) {
    OSG::Vec3i v;
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    return v;
}


void toValue(string s, string& s2) {
    s2 = s;
}

void toValue(string s, bool& b) {
    stringstream ss(s);
    ss >> b;
}

void toValue(string s, int& i) {
    stringstream ss(s);
    ss >> i;
}

void toValue(string s, float& f) {
    stringstream ss(s);
    ss >> f;
}

void toValue(string s, OSG::Vec2f& v) {
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
}

void toValue(string s, OSG::Vec3f& v) {
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
}

void toValue(string s, OSG::Vec4f& v) {
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
    ss >> v[3];
}

void toValue(string s, OSG::Vec3i& v) {
    stringstream ss(s);
    ss >> v[0];
    ss >> v[1];
    ss >> v[2];
}

string toString(OSG::posePtr po) {
    return toString(po->pos()) + " " + toString(po->dir()) + " " + toString(po->up());
}

void toValue(string s, OSG::posePtr& po) {
    OSG::Vec3f p,d,u;
    stringstream ss(s);
    ss >> p[0];
    ss >> p[1];
    ss >> p[2];
    ss >> d[0];
    ss >> d[1];
    ss >> d[2];
    ss >> u[0];
    ss >> u[1];
    ss >> u[2];
    if (po) po->set(p,d,u);
    else po = OSG::pose::create(p,d,u);
}
