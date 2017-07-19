#ifndef TOSTRING_H_INCLUDED
#define TOSTRING_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGLine.h>
#include "core/math/VRMathFwd.h"

using namespace std;

vector<string> splitString(string s, char c = ' ');

string toString(const string& s);
string toString(const bool& b);
string toString(const int& i);
string toString(const size_t& i);
string toString(const unsigned int& i);
string toString(const double& f, int d = -1);
string toString(const float& f, int d = -1);
string toString(const OSG::Vec2d& v);
string toString(const OSG::Vec3d& v);
string toString(const OSG::Vec4d& v);
string toString(const OSG::Pnt3d& v);
string toString(const OSG::Vec2i& v);
string toString(const OSG::Vec3i& v);
string toString(const OSG::Vec4i& v);
string toString(const OSG::Color3f& v);
string toString(const OSG::Color4f& v);
string toString(const OSG::Line& l);
string toString(const OSG::pose& p);
string toString(const OSG::posePtr& p);
string toString(const OSG::Boundingbox& b);

// deprecated?
bool toBool(string s, int* N = 0);
int toInt(string s, int* N = 0);
unsigned int toUInt(string s, int* N = 0);
float toFloat(string s, int* N = 0);
double toDouble(string s, int* N = 0);
OSG::Vec2d toVec2d(string s);
OSG::Vec3d toVec3d(string s);
OSG::Vec4d toVec4d(string s);
OSG::Vec2i toVec2i(string s);
OSG::Vec3i toVec3i(string s);
OSG::Vec4i toVec4i(string s);
OSG::Pnt3d toPnt3f(string s);

template<typename T> string typeName(const T& t);
template<typename T> string typeName(const vector<T>& t) { return "list of "+typeName<T>(T()); }

template<typename T> bool toValue(stringstream& s, T& t);
template<typename T> bool toValue(string s, T& t){
    stringstream ss(s);
    return toValue(ss,t);
}

template<typename T> bool toValue(string s, std::shared_ptr<T>& t) {
    t = 0;
    return true;
}

template<typename T> bool toValue(string s, vector<T>& t) {
    return true;
}

#endif // TOSTRING_H_INCLUDED
