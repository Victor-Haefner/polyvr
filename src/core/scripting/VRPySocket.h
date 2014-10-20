#ifndef VRPYSOCKET_H_INCLUDED
#define VRPYSOCKET_H_INCLUDED

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <structmember.h>
#include "../networking/VRSocket.h"

struct VRPySocket {
    PyObject_HEAD
    OSG::VRSocket* obj;
    static PyObject* err;

    static PyObject* fromPtr(OSG::VRSocket* obj);

    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static void dealloc(VRPySocket* self);

    static int init(VRPySocket *self, PyObject *args, PyObject *kwds);

    static PyMemberDef members[];
    static PyMethodDef methods[];
    static PyTypeObject VRPySocketType;

    static PyObject* getName(VRPySocket* self);
    static PyObject* destroy(VRPySocket* self);
    static PyObject* send(VRPySocket* self, PyObject* args);
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

PyMODINIT_FUNC initVRPySocket(PyObject* module);

#endif // VRPYSOCKET_H_INCLUDED
