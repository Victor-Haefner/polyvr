#ifndef VRPYJOINTTOOL_H_INCLUDED
#define VRPYJOINTTOOL_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRKinematictool.h"

struct VRPyJointTool : VRPyBaseT<OSG::VRJointTool> {
    static PyMethodDef methods[];

    static PyObject* append(VRPyJointTool* self, PyObject* args);
    static PyObject* clear(VRPyJointTool* self);
};

#endif // VRPYJOINTTOOL_H_INCLUDED
