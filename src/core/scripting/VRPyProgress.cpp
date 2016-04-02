#include "VRPyProgress.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType( Progress, New_ptr );

PyMethodDef VRPyProgress::methods[] = {
    {"setup", (PyCFunction)VRPyProgress::setup, METH_VARARGS, "Setup the progression - setup()" },
    {"update", (PyCFunction)VRPyProgress::update, METH_VARARGS, "Set the path start point" },
    {"reset", (PyCFunction)VRPyProgress::reset, METH_NOARGS, "Set the path start point" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyProgress::setup(VRPyProgress* self, PyObject* args) {
    if (!self->valid()) return NULL;
    Py_RETURN_TRUE;
}

PyObject* VRPyProgress::update(VRPyProgress* self, PyObject* args) {
    if (!self->valid()) return NULL;
    Py_RETURN_TRUE;
}

PyObject* VRPyProgress::reset(VRPyProgress* self) {
    if (!self->valid()) return NULL;
    Py_RETURN_TRUE;
}
