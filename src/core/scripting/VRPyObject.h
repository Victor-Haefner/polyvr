#ifndef VRPYOBJECT_H_INCLUDED
#define VRPYOBJECT_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include "VRPyBase.h"

struct VRPyObject : VRPyBaseT<OSG::VRObject> {
    static PyMethodDef methods[];

    static int compare(PyObject* o1, PyObject* o2);
    static long hash(PyObject* o);

    static PyObject* getName(VRPyObject* self);
    static PyObject* getBaseName(VRPyObject* self);
    static PyObject* setName(VRPyObject* self, PyObject* args);
    static PyObject* setPersistency(VRPyObject* self, PyObject* args);
    static PyObject* getPersistency(VRPyObject* self);
};

#endif // VRPYOBJECT_H_INCLUDED
