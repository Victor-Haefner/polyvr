#ifndef TOSTRING_H_INCLUDED
#define TOSTRING_H_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include <sstream>

using namespace std;

vector<string> splitString(string s, char c = ' ');

template<typename T>
string toString(const T& s);
string toString(const double& f, int d = -1);
string toString(const float& f, int d = -1);

template<typename T> string typeName(const T& t);
template<typename T> string typeName(const vector<T>& t) { return "list of "+typeName<T>(T()); }

template<typename T> int toValue(stringstream& s, T& t);
template<typename T> int toValue(string s, T& t) { stringstream ss(s); return toValue(ss,t); }
template<typename T> int toValue(string s, std::shared_ptr<T>& t) { t = 0; return true; }
template<typename T> int toValue(string s, vector<T>& t) { return true; }
template<class T>    T   toValue(string s) { T t; toValue(s,t); return t; }

int   toInt  (string s);
float toFloat(string s);

#endif // TOSTRING_H_INCLUDED
