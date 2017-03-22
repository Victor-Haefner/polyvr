#ifndef TOSTRING_H_INCLUDED
#define TOSTRING_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
#include "core/math/VRMathFwd.h"

using namespace std;

vector<string> splitString(string s, char c = ' ');

string toString(string s);
string toString(bool b);
string toString(int i);
string toString(size_t i);
string toString(unsigned int i);
string toString(double f, int d = -1);
string toString(float f, int d = -1);
string toString(OSG::Vec2f v);
string toString(OSG::Vec3f v);
string toString(OSG::Pnt3f v);
string toString(OSG::Vec4f v);
string toString(OSG::Vec2i v);
string toString(OSG::Vec3i v);
string toString(OSG::Vec4i v);
string toString(OSG::posePtr p);
string toString(const OSG::boundingbox& b);

// deprecated?
bool toBool(string s, int* N = 0);
int toInt(string s, int* N = 0);
unsigned int toUInt(string s, int* N = 0);
float toFloat(string s, int* N = 0);
double toDouble(string s, int* N = 0);
OSG::Vec2f toVec2f(string s);
OSG::Vec3f toVec3f(string s);
OSG::Vec4f toVec4f(string s);
OSG::Vec2i toVec2i(string s);
OSG::Vec3i toVec3i(string s);
OSG::Vec4i toVec4i(string s);
OSG::Pnt3f toPnt3f(string s);

void toValue(string s, string& s2);
void toValue(string s, bool& b);
void toValue(string s, int& i);
void toValue(string s, float& f);
void toValue(string s, OSG::Vec2f& v);
void toValue(string s, OSG::Vec3f& v);
void toValue(string s, OSG::Vec4f& v);
void toValue(string s, OSG::Vec3i& v);
void toValue(string s, OSG::posePtr& p);
void toValue(string s, OSG::boundingbox& b);

#endif // TOSTRING_H_INCLUDED
