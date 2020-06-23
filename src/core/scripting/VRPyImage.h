#ifndef VRPyTexture_H_INCLUDED
#define VRPyTexture_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/material/VRTexture.h"

struct VRPyTexture : VRPyBaseT<OSG::VRTexture> {
    static PyMethodDef methods[];

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);

    static PyObject* read(VRPyTexture* self, PyObject *args);
    static PyObject* write(VRPyTexture* self, PyObject *args);
    static PyObject* getPixel(VRPyTexture* self, PyObject *args);
    static PyObject* getSize(VRPyTexture* self, PyObject *args);
    static PyObject* getAspectRatio(VRPyTexture* self, PyObject *args);
    static PyObject* getChannels(VRPyTexture* self, PyObject *args);
};

#endif // VRPyTexture_H_INCLUDED
