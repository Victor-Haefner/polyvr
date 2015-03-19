#ifndef VRPYCLIPPLANE_H_INCLUDED
#define VRPYCLIPPLANE_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRClipPlane.h"

struct VRPyClipPlane : VRPyBaseT<OSG::VRClipPlane> {
    static PyMethodDef methods[];

    static PyObject* setTree(VRPyClipPlane* self, PyObject* args);
    static PyObject* setActive(VRPyClipPlane* self, PyObject* args);
    static PyObject* isActive(VRPyClipPlane* self);
};

#endif // VRPYCLIPPLANE_H_INCLUDED
