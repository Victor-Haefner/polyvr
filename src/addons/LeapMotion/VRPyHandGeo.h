#pragma once

#include "core/scripting/VRPyObject.h"
#include "VRHandGeo.h"

struct VRPyHandGeo : VRPyBaseT<OSG::VRHandGeo> {
    static PyMethodDef methods[];

    static PyObject* update(VRPyHandGeo* self, PyObject* args);
    static PyObject* setLeft(VRPyHandGeo* self);
    static PyObject* setRight(VRPyHandGeo* self);
    static PyObject* connectToLeap(VRPyHandGeo* self, PyObject* args);
};
