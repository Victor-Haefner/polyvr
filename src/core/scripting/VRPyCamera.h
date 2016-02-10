#ifndef VRPYCAMERA_H_INCLUDED
#define VRPYCAMERA_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/VRCamera.h"

struct VRPyCamera : VRPyBaseT<OSG::VRCamera> {
    static PyMethodDef methods[];
    static PyObject* activate(VRPyCamera* self);
    static PyObject* setFov(VRPyCamera* self, PyObject* args);
    static PyObject* focus(VRPyCamera* self, PyObject* args);
};

#endif // VRPYCAMERA_H_INCLUDED
