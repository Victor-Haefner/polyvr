#ifndef VRPyTexture_H_INCLUDED
#define VRPyTexture_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/material/VRTexture.h"

struct VRPyTexture : VRPyBaseT<OSG::VRTexture> {
    static PyMethodDef methods[];
    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
};

#endif // VRPyTexture_H_INCLUDED
