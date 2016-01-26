#ifndef VRPYTRIANGULATOR_H_INCLUDED
#define VRPYTRIANGULATOR_H_INCLUDED

#include "core/math/triangulator.h"
#include "core/scripting/VRPyBase.h"

struct VRPyTriangulator : VRPyBaseT<OSG::Triangulator> {
    static PyMethodDef methods[];

    static PyObject* add(VRPyTriangulator* self, PyObject* args);
    static PyObject* compute(VRPyTriangulator* self);
};

#endif // VRPYTRIANGULATOR_H_INCLUDED
