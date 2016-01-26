#ifndef VRPYIMAGE_H_INCLUDED
#define VRPYIMAGE_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/material/VRTexture.h"

struct VRPyImage : VRPyBaseT<OSG::VRTexture> {
    static PyMethodDef methods[];

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
};

#endif // VRPYIMAGE_H_INCLUDED
