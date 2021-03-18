#ifndef TOSTRING_H_INCLUDED
#define TOSTRING_H_INCLUDED

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include "core/utils/VRFunctionFwd.h"

namespace OSG {
    class Pose;
}

// define types here, for example for webassembly
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;

using namespace std;

vector<string> splitString(const string& s, char c = ' ');
string subString(const string& s, int beg, int len);
bool startsWith(const string& s, const string& s2, bool caseSensitive = true);
bool endsWith(const string& s, const string& s2, bool caseSensitive = true);
bool contains(const string& s, const string& s2, bool caseSensitive = true);

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
template<typename T> string typeName(const std::shared_ptr<T>& t) { return t ? typeName<T>(*t) : "null"; }
template<typename T> string typeName(const std::weak_ptr<T>& t) { return t.lock() ? typeName<T>(*t.lock()) : "null"; }
template<typename T> string typeName(const vector<T>& t) { return "list of "+typeName<T>(T()); }
template<typename T> string typeName(const vector<std::shared_ptr<T>>& t) { return "list of "+typeName<T>(T()); }
template<typename T> string typeName(const vector<vector<T>>& t) { return "list of lists of "+typeName<T>(T()); }
template<typename T> string typeName(const vector<vector<std::shared_ptr<T>>>& t) { return "list of lists of "+typeName<T>(T()); }
template<typename T, typename U> string typeName(const map<T,U>& t) { return "dictionary of "+typeName<T>(T()) + " to " + typeName<U>(U()); }

string typeName(const std::shared_ptr<VRFunction<void>> t);
template<typename T> string typeName(const std::shared_ptr<VRFunction<T>> t) { return "callback("+typeName<T>(T())+")"; }
template<typename T, typename R> string typeName(const std::shared_ptr<VRFunction<T,R>> t) { return typeName<R>(R())+" callback("+typeName<T>(T())+")"; }
template<typename T> string typeName(const std::shared_ptr<VRFunction<std::shared_ptr<T>>> t) { return "callback("+typeName<T>(T())+")"; }
template<typename T> string typeName(const std::shared_ptr<VRFunction<std::weak_ptr<T>>> t) { return "callback("+typeName<T>(T())+")"; }
template<typename T> string typeName(const std::shared_ptr<VRFunction<vector<T>>> t) { return "callback(list of "+typeName<T>(T())+")"; }
template<typename T> string typeName(const std::shared_ptr<VRFunction<vector<std::shared_ptr<T>>>> t) { return "callback(list of "+typeName<T>(T())+")"; }
template<typename T, typename U> string typeName(const std::shared_ptr<VRFunction<map<T,U>>> t) { return "callback(dictionary of "+typeName<T>(T()) + " to " + typeName<U>(U())+")"; }
template<typename T, typename U> string typeName(const std::shared_ptr<VRFunction<map<T,std::shared_ptr<U>>>> t) { return "callback(dictionary of "+typeName<T>(T()) + " to " + typeName<U>(U())+")"; }

template<typename T> int toValue(stringstream& s, T& t);
template<typename T> int toValue(stringstream& s, vector<std::shared_ptr<T>>& t) { return true; }

template<typename T> int toValue(stringstream& s, std::shared_ptr<T>& t) {
    if (s.str() != "0") { cout << "Warning in toValue<shared_ptr<T>> (toString.h): ignore data '" << s.str() << "'" << endl; return false; }
    t = 0;
    return true;
}

template<> int toValue(stringstream& ss, std::shared_ptr<OSG::Pose>& po);

template<typename T> int toValue(string s, T& t) { stringstream ss(s); return toValue(ss, t); }

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

template<typename T> int toValue(string s, vector<std::shared_ptr<T>>& t) { return true; }
template<typename T> int toValue(string s, vector<vector<T>>& t) { return true; } // not implemented
template<typename T, typename U> int toValue(string s, map<T, U>& t) { return true; } // not implemented
template<class T>    T   toValue(string s) { T t; toValue(s,t); return t; }

int   toInt  (string s);
float toFloat(string s);

bool isNumber(string s);

#endif // TOSTRING_H_INCLUDED
