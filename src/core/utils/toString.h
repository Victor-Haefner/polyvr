#ifndef TOSTRING_H_INCLUDED
#define TOSTRING_H_INCLUDED

#include <string>
#include <OpenSG/OSGVector.h>

using namespace std;

string toString(string s);
string toString(bool b);
string toString(int i);
string toString(size_t i);
string toString(unsigned int i);
string toString(float f);
string toString(OSG::Vec2f v);
string toString(OSG::Vec3f v);
string toString(OSG::Pnt3f v);
string toString(OSG::Vec4f v);
string toString(OSG::Vec3i v);

// deprecated?
bool toBool(string s);
int toInt(string s);
float toFloat(string s);
OSG::Vec2f toVec2f(string s);
OSG::Vec3f toVec3f(string s);
OSG::Vec4f toVec4f(string s);
OSG::Vec3i toVec3i(string s);
OSG::Pnt3f toPnt3f(string s);

void toValue(string s, string& s2);
void toValue(string s, bool& b);
void toValue(string s, int& i);
void toValue(string s, float& f);
void toValue(string s, OSG::Vec2f& v);
void toValue(string s, OSG::Vec3f& v);
void toValue(string s, OSG::Vec4f& v);
void toValue(string s, OSG::Vec3i& v);

#endif // TOSTRING_H_INCLUDED
