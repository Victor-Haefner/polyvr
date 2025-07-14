#ifndef VRPYOBJECT_H_INCLUDED
#define VRPYOBJECT_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include "VRPyBase.h"

struct VRPyObject : VRPyBaseT<OSG::VRObject> {
    static PyMethodDef methods[];

    static PyObject* compare(PyObject* o1, PyObject* o2, int op);
    static Py_hash_t hash(PyObject* o);

    static PyObject* destroy(VRPyObject* self);

    static PyObject* getName(VRPyObject* self);
    static PyObject* getBaseName(VRPyObject* self);
    static PyObject* setName(VRPyObject* self, PyObject* args);
    static PyObject* setPersistency(VRPyObject* self, PyObject* args);
    static PyObject* getPersistency(VRPyObject* self);
};

#endif // VRPYOBJECT_H_INCLUDED
