#ifndef VRPYPROGRESS_H_INCLUDED
#define VRPYPROGRESS_H_INCLUDED

#include "core/utils/VRProgress.h"
#include "core/scripting/VRPyBase.h"

struct VRPyProgress : VRPyBaseT<OSG::VRProgress> {
    static PyMethodDef methods[];

    static PyObject* setup(VRPyProgress* self, PyObject *args);
    static PyObject* update(VRPyProgress* self, PyObject *args);
    static PyObject* get(VRPyProgress* self);
    static PyObject* reset(VRPyProgress* self);
};

#endif // VRPYPROGRESS_H_INCLUDED
