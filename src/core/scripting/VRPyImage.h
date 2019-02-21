#ifndef VRPYIMAGE_H_INCLUDED
#define VRPYIMAGE_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/material/VRTexture.h"

struct VRPyImage : VRPyBaseT<OSG::VRTexture> {
    static PyMethodDef methods[];

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);

    static PyObject* read(VRPyImage* self, PyObject *args);
    static PyObject* write(VRPyImage* self, PyObject *args);
    static PyObject* getPixel(VRPyImage* self, PyObject *args);
    static PyObject* getSize(VRPyImage* self, PyObject *args);
    static PyObject* getAspectRatio(VRPyImage* self, PyObject *args);
    static PyObject* getChannels(VRPyImage* self, PyObject *args);
};

#endif // VRPYIMAGE_H_INCLUDED
