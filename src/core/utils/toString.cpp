#include "toString.h"
#include "core/math/VRMathFwd.h"
#include "core/math/pose.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRFunctionFwd.h"

#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGLine.h>

#include <boost/algorithm/string.hpp>

using namespace OSG;
using namespace boost::algorithm;

vector<string> splitString(const string& s, char c) {
    stringstream ss(s);
    string token;
    vector<string> res;
    while (std::getline(ss, token, c)) res.push_back(token);
    return res;
}

string subString(const string& s, int beg, int len) { return s.substr(beg, len); }

bool startsWith(const string& s, const string& s2, bool caseSensitive) {
    string s1 = subString(s, 0, s2.size());
    if (caseSensitive) return bool(s1 == s2);
    else return bool(to_lower_copy(s1) == to_lower_copy(s2));
}

bool endsWith(const string& s, const string& s2, bool caseSensitive) {
    string s1 = subString(s, s.size() - s2.size(), s2.size());
    if (caseSensitive) return bool(s1 == s2);
    else return bool(to_lower_copy(s1) == to_lower_copy(s2));
}

bool contains(const string& s, const string& s2, bool caseSensitive) {
    if (caseSensitive) return bool(s.find(s2) != std::string::npos);
    else return bool(to_lower_copy(s).find(to_lower_copy(s2)) != std::string::npos);
}

typedef void* voidPtr;

template<> string toString(const string& s) { return s; }
template<> string toString(const bool& b) { stringstream ss; ss << b; return ss.str(); }
template<> string toString(const short& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const unsigned short& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const int& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const long& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const unsigned long& i) { stringstream ss; ss << i; return ss.str(); } // sometimes same as size_t ?
template<> string toString(const signed char& i) { stringstream ss; ss << i; return ss.str(); }
template<> string toString(const unsigned char& i) { stringstream ss; ss << int(i); return ss.str(); }
template<> string toString(const voidPtr& i) { stringstream ss; ss << i; return ss.str(); }
#ifdef _WIN32
template<> string toString(const size_t& i) { stringstream ss; ss << i; return ss.str(); }
#endif
template<> string toString(const unsigned int& i) { stringstream ss; ss << long(i); return ss.str(); }

#ifdef _WIN32
template<> string toString(const __int64& i) { stringstream ss; ss << i; return ss.str(); }
#endif

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

string toString(const vector<float>& v, int d) {
    string res = "[";
    for (unsigned int i=0; i<v.size(); i++) {
        if (i > 0) res += ", ";
        res += toString(v[i], d);
    }
    return res+"]";
}

string toString(const vector<double>& v, int d) {
    string res = "[";
    for (unsigned int i=0; i<v.size(); i++) {
        if (i > 0) res += ", ";
        res += toString(v[i], d);
    }
    return res+"]";
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
    if (!po) return "";
    return toString(po->pos()) + " " + toString(po->dir()) + " " + toString(po->up());
}

template<> string toString(const Pose& po) {
    Pose* p = (Pose*)&po;
    return toString(p->pos()) + " " + toString(p->dir()) + " " + toString(p->up());
}

template<> string toString(const Boundingbox& b) {
    return toString(b.min()) + " " + toString(b.max()) + " " + toString(b.empty());
}

template<> string toString(const Line& l) {
    return toString(Pnt3d(l.getPosition())) + " " + toString(Vec3d(l.getDirection()));
}

template<> string toString(const Matrix4d& m) {
    stringstream ss;
    ss << toString(m[0]) << " " << toString(m[1]) << " " << toString(m[2]) << " " << toString(m[3]);
    return ss.str();
}

typedef void* voidPtr;

template<> string typeName(const voidPtr& t) { return "pointer"; }
template<> string typeName(const string& t) { return "string"; }
template<> string typeName(const int& t) { return "int"; }
template<> string typeName(const unsigned int& t) { return "int"; }
//#ifdef _WIN32
template<> string typeName(const size_t& t) { return "int"; }
//#endif
template<> string typeName(const float& t) { return "float"; }
template<> string typeName(const double& t) { return "double"; }
template<> string typeName(const bool& t) { return "bool"; }
template<> string typeName(const char& t) { return "char"; }
template<> string typeName(const unsigned char& t) { return "unsigned char"; }
template<> string typeName(const Pnt2d& t) { return "Pnt2d"; }
template<> string typeName(const Pnt3d& t) { return "Pnt3d"; }
template<> string typeName(const Pnt4d& t) { return "Pnt4d"; }
template<> string typeName(const Vec2d& t) { return "Vec2d"; }
template<> string typeName(const Vec3d& t) { return "Vec3d"; }
template<> string typeName(const Vec4d& t) { return "Vec4d"; }
template<> string typeName(const Vec2i& t) { return "Vec2i"; }
template<> string typeName(const Vec3i& t) { return "Vec3i"; }
template<> string typeName(const Vec4i& t) { return "Vec4i"; }
template<> string typeName(const Matrix4d& t) { return "Matrix"; }
template<> string typeName(const Color3f& t) { return "Vec3d"; }
template<> string typeName(const Color4f& t) { return "Vec4d"; }
template<> string typeName(const Line& t) { return "Line"; }
string typeName(const std::shared_ptr<VRFunction<void>> t) { return "callback()"; }

template <typename T> int ssToVal(stringstream& ss, T& t) {
    int N = ss.tellg();
    bool b = false;
    do {
        b = bool(ss >> t);
        if (ss.fail()) {
            ss.clear();
            char dummy;
            ss >> dummy;
        } else break;
    } while(!ss.eof());
    return (int(ss.tellg()) - N)*b;
}

template<> int toValue(stringstream& ss, void*& s) { return true; }
template<> int toValue(stringstream& ss, string& s) { s = ss.str(); return true; }
template<> int toValue(stringstream& ss, bool& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, char& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, signed char& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, unsigned char& v) { return ssToVal(ss, v); }
#ifdef _WIN32
template<> int toValue(stringstream& ss, size_t& v) { return ssToVal(ss, v); }
#endif
template<> int toValue(stringstream& ss, short& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, unsigned short& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, int& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, unsigned int& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, long& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, unsigned long& v) { return ssToVal(ss, v); }
template<> int toValue(stringstream& ss, float& v) { double d; auto r = ssToVal(ss, d); v = d; return r; } // use double because stringstreams may fail to convert scientific notations to float
template<> int toValue(stringstream& ss, double& v) { return ssToVal(ss, v); }

int   toInt  (string s) { return toValue<int  >(s); }
float toFloat(string s) { return toValue<float>(s); }

template<> int toValue(stringstream& ss, Vec2d& v) {
    ssToVal(ss, v[0]);
    return ssToVal(ss, v[1]);
}

template<> int toValue(stringstream& ss, Vec3d& v) {
    ssToVal(ss, v[0]);
    ssToVal(ss, v[1]);
    return ssToVal(ss, v[2]);
}

template<> int toValue(stringstream& ss, Vec4d& v) {
    ssToVal(ss, v[0]);
    ssToVal(ss, v[1]);
    ssToVal(ss, v[2]);
    return ssToVal(ss, v[3]);
}

template<> int toValue(stringstream& ss, Pnt2d& v) {
    ssToVal(ss, v[0]);
    return ssToVal(ss, v[1]);
}

template<> int toValue(stringstream& ss, Pnt3d& v) {
    ssToVal(ss, v[0]);
    ssToVal(ss, v[1]);
    return ssToVal(ss, v[2]);
}

template<> int toValue(stringstream& ss, Pnt4d& v) {
    ssToVal(ss, v[0]);
    ssToVal(ss, v[1]);
    ssToVal(ss, v[2]);
    return ssToVal(ss, v[3]);
}

template<> int toValue(stringstream& ss, Vec2i& v) {
    ssToVal(ss, v[0]);
    return ssToVal(ss, v[1]);
}

template<> int toValue(stringstream& ss, Vec3i& v) {
    ssToVal(ss, v[0]);
    ssToVal(ss, v[1]);
    return ssToVal(ss, v[2]);
}

template<> int toValue(stringstream& ss, Vec4i& v) {
    ssToVal(ss, v[0]);
    ssToVal(ss, v[1]);
    ssToVal(ss, v[2]);
    return ssToVal(ss, v[3]);
}

template<> int toValue(stringstream& ss, Color3f& v) {
    ssToVal(ss, v[0]);
    ssToVal(ss, v[1]);
    return ssToVal(ss, v[2]);
}

template<> int toValue(stringstream& ss, Color4f& v) {
    ssToVal(ss, v[0]);
    ssToVal(ss, v[1]);
    ssToVal(ss, v[2]);
    return ssToVal(ss, v[3]);
}

template<> int toValue(stringstream& ss, Line& l) {
    Vec3d p,d;
    toValue(ss, p);
    bool b = toValue(ss, d);
    l = Line(Pnt3f(p),Vec3f(d));
    return b;
}

template<> int toValue(stringstream& ss, Matrix4d& m) {
    Vec4d a,b,c,d;
    toValue(ss, a);
    toValue(ss, b);
    toValue(ss, c);
    bool r = toValue(ss, d);
    m = Matrix4d(a,b,c,d);
    return r;
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
    if (ss.str() == "0") return true;
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

bool isNumber(string number) {
    if (number[0] >= '0' && number[0] <= '9') return true;
    if (number[0] == '-' || number[0] == '+' || number[0] == '.') {
        if (number[1] >= '0' && number[1] <= '9') return true;
    }
    if (number[0] == '-' || number[0] == '+') {
        if (number[1] == '.' && number[2] >= '0' && number[3] <= '9') return true;
    }
    return false;
}

