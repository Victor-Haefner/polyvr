#ifndef VRPYLIGHT_H_INCLUDED
#define VRPYLIGHT_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/VRLight.h"

struct VRPyLight : VRPyBaseT<OSG::VRLight> {
    static PyMethodDef methods[];

    static PyObject* setOn(VRPyLight* self, PyObject *args);
    static PyObject* setBeacon(VRPyLight* self, PyObject *args);
    static PyObject* setAttenuation(VRPyLight* self, PyObject *args);
    static PyObject* setDiffuse(VRPyLight* self, PyObject *args);
    static PyObject* setAmbient(VRPyLight* self, PyObject *args);
    static PyObject* setSpecular(VRPyLight* self, PyObject *args);
    static PyObject* setType(VRPyLight* self, PyObject *args);
    static PyObject* setShadow(VRPyLight* self, PyObject *args);
};

#endif // VRPYLIGHT_H_INCLUDED
