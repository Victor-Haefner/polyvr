#ifndef TOSTRING_H_INCLUDED
#define TOSTRING_H_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include "core/utils/VRFunctionFwd.h"

using namespace std;

vector<string> splitString(const string& s, char c = ' ');
string subString(const string& s, int beg, int len);
bool startsWith(const string& s, const string& s2);
bool endsWith(const string& s, const string& s2);

template<typename T> string toString(const T& s);
string toString(const double& f, int d = -1);
string toString(const float& f, int d = -1);

template<typename T> string toString(const vector<T>& v) {
    string res = "[";
    for (unsigned int i=0; i<v.size(); i++) {
        if (i > 0) res += ", ";
        res += toString<T>(v[i]);
    }
    return res+"]";
}

string toString(const vector<float>& v, int d=-1);
string toString(const vector<double>& v, int d=-1);

template<typename T> string typeName(const T& t);
template<typename T> string typeName(const vector<T>& t) { return "list of "+typeName<T>(T()); }
template<typename T> string typeName(const vector<vector<T>>& t) { return "list of lists of "+typeName<T>(T()); }
template<typename T> string typeName(const std::shared_ptr<VRFunction<T>> t) { return "callback("+typeName<T>(T())+")"; }
template<typename T> string typeName(const std::shared_ptr<VRFunction<vector<T>>> t) { return "callback(list of "+typeName<T>(T())+")"; }

template<typename T> int toValue(stringstream& s, T& t);
template<typename T> int toValue(string s, T& t) { stringstream ss(s); return toValue(ss,t); }

template<typename T> int toValue(string s, vector<T>& t) {
    stringstream ss(s);
    T v;
    while (!ss.eof()) {
        int b = toValue(ss, v);
        if (b) t.push_back(v);
        else break;
    }
    return true;
}

template<typename T> int toValue(string s, vector<vector<T>>& t) { return true; } // not implemented
template<class T>    T   toValue(string s) { T t; toValue(s,t); return t; }

int   toInt  (string s);
float toFloat(string s);

#endif // TOSTRING_H_INCLUDED
