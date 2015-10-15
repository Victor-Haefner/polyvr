#ifndef VRPYMOUSE_H_INCLUDED
#define VRPYMOUSE_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/devices/VRMouse.h"

struct VRPyMouse : VRPyBaseT<OSG::VRMouse> {
    static PyMethodDef methods[];

    static PyObject* setCursor(VRPyMouse* self, PyObject* args);
};

#endif // VRPYMOUSE_H_INCLUDED
