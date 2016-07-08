#ifndef VRPYPOSE_H_INCLUDED
#define VRPYPOSE_H_INCLUDED

#include "core/math/pose.h"
#include "core/scripting/VRPyBase.h"

struct VRPyPose : VRPyBaseT<OSG::pose> {
    static PyMethodDef methods[];

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static PyObject* pos(VRPyPose* self);
    static PyObject* dir(VRPyPose* self);
    static PyObject* up(VRPyPose* self);
    static PyObject* set(VRPyPose* self, PyObject *args);
    static PyObject* mult(VRPyPose* self, PyObject *args);
    static PyObject* multInv(VRPyPose* self, PyObject *args);
    static PyObject* invert(VRPyPose* self);
};

#endif // VRPYPOSE_H_INCLUDED
