#ifndef VRPYLISTMATH_H_INCLUDED
#define VRPYLISTMATH_H_INCLUDED

#include "VRPyBase.h"

struct VRPyListMath {
    static void init(PyObject* mod);

    static PyObject* add(PyObject* self, PyObject* args);
};

#endif // VRPYLISTMATH_H_INCLUDED
