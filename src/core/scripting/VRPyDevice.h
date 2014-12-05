#ifndef VRPYDEVICE_H_INCLUDED
#define VRPYDEVICE_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/devices/VRDevice.h"

struct VRPyDevice : VRPyBaseT<OSG::VRDevice> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* getName(VRPyDevice* self);
    static PyObject* destroy(VRPyDevice* self);
    static PyObject* getBeacon(VRPyDevice* self);
    static PyObject* setBeacon(VRPyDevice* self, PyObject *args);
    static PyObject* getTarget(VRPyDevice* self);
    static PyObject* setTarget(VRPyDevice* self, PyObject *args);
    static PyObject* getKey(VRPyDevice* self);
    static PyObject* getKeyState(VRPyDevice* self, PyObject *args);
    static PyObject* getSlider(VRPyDevice* self, PyObject *args);
    static PyObject* getMessage(VRPyDevice* self);
    static PyObject* getType(VRPyDevice* self);
    static PyObject* setDnD(VRPyDevice* self, PyObject *args);
    static PyObject* getIntersected(VRPyDevice* self);
    static PyObject* getIntersection(VRPyDevice* self);
    static PyObject* addIntersection(VRPyDevice* self, PyObject *args);
    static PyObject* remIntersection(VRPyDevice* self, PyObject *args);
    static PyObject* getDragged(VRPyDevice* self);
};

#endif // VRPYDEVICE_H_INCLUDED
