#ifndef VRPYTYPECASTER_H_INCLUDED
#define VRPYTYPECASTER_H_INCLUDED

#include <vector>

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRDeviceFwd.h"

using namespace std;

class VRPyTypeCaster {
    private:
        static PyObject* pack(const vector<PyObject*>& v);

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
};

#endif // VRPYTYPECASTER_H_INCLUDED
