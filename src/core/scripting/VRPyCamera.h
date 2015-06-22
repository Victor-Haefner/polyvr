#ifndef VRPYCAMERA_H_INCLUDED
#define VRPYCAMERA_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/VRCamera.h"

struct VRPyCamera : VRPyBaseT<OSG::VRCamera> {
    static PyMethodDef methods[];
    static PyObject* activate(VRPyCamera* self);
};

#endif // VRPYCAMERA_H_INCLUDED
