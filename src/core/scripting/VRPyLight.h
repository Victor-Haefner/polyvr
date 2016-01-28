#ifndef VRPYLIGHT_H_INCLUDED
#define VRPYLIGHT_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/VRLight.h"

struct VRPyLight : VRPyBaseT<OSG::VRLight> {
    static PyMethodDef methods[];

    static PyObject* setOn(VRPyLight* self, PyObject *args);
    static PyObject* setBeacon(VRPyLight* self, PyObject *args);
};

#endif // VRPYLIGHT_H_INCLUDED
