#ifndef VRPYTYPECASTER_H_INCLUDED
#define VRPYTYPECASTER_H_INCLUDED

#include <vector>
#include <map>

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRDeviceFwd.h"

using namespace std;

class VRPyTypeCaster {
    private:
        static PyObject* pack(const vector<PyObject*>& v);
        static PyObject* pack(const vector< vector<PyObject*> >& v);
        static PyObject* pack(const vector< pair<PyObject*,PyObject*> >& v);

    public:
        VRPyTypeCaster();
        static PyObject* err;

        template<typename T>
        static PyObject* cast(const T& t);

        template<typename T>
        static PyObject* cast(const vector<T>& vt) {
            vector<PyObject*> l;
            for (auto t : vt) {
                PyObject* o = cast<T>(t);
                if (o) l.push_back(o);
            }
            return pack(l);
        }

        template<typename T>
        static PyObject* cast(const vector<vector<T>>& vvt) {
            vector<vector<PyObject*>> l2;
            for (auto vt : vvt) {
                vector<PyObject*> l;
                for (auto t : vt) {
                    PyObject* o = cast<T>(t);
                    if (o) l.push_back(o);
                }
                l2.push_back(l);
            }
            return pack(l2);
        }

        template<typename T, typename G>
        static PyObject* cast(const map<T,G>& vt) {
            vector< pair<PyObject*,PyObject*> > l;
            for (auto t : vt) {
                PyObject* o1 = cast<T>(t.first);
                PyObject* o2 = cast<G>(t.second);
                if (o1 && o2) l.push_back(make_pair(o1, o2));
            }
            return pack(l);
        }

        template<typename T, typename G>
        static PyObject* cast(const map<T,vector<G>>& vt) {
            vector< pair<PyObject*,PyObject*> > l;
            for (auto t : vt) {
                PyObject* o1 = cast<T>(t.first);
                PyObject* o2 = cast<G>(t.second);
                if (o1 && o2) l.push_back(make_pair(o1, o2));
            }
            return pack(l);
        }
};

#endif // VRPYTYPECASTER_H_INCLUDED
