#ifndef VRPyPath_H_INCLUDED
#define VRPyPath_H_INCLUDED

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <structmember.h>
#include "core/math/path.h"

struct VRPyPath {
    PyObject_HEAD
    OSG::path* obj;
    static PyObject* err;

    static PyObject* fromPtr(OSG::path* obj);

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static void dealloc(VRPyPath* self);

    static int init(VRPyPath *self, PyObject *args, PyObject *kwds);

    static PyMemberDef members[];
    static PyMethodDef methods[];
    static PyTypeObject type;

    static PyObject* setStartPoint(VRPyPath* self, PyObject *args);
    static PyObject* setEndPoint(VRPyPath* self, PyObject *args);
    static PyObject* compute(VRPyPath* self, PyObject* args);
    static PyObject* getStartPoint(VRPyPath* self);
    static PyObject* getEndPoint(VRPyPath* self);
    static PyObject* update(VRPyPath* self);
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

PyMODINIT_FUNC initVRPyPath(PyObject* module);

#endif // VRPyPath_H_INCLUDED
