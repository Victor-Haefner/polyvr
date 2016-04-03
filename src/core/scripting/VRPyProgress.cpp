#include "VRPyProgress.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType( Progress, New_ptr );

PyMethodDef VRPyProgress::methods[] = {
    {"setup", (PyCFunction)VRPyProgress::setup, METH_VARARGS, "Setup the progress parameters - setup()" },
    {"get", (PyCFunction)VRPyProgress::get, METH_NOARGS, "Get the current state t[0,1]- float get()" },
    {"update", (PyCFunction)VRPyProgress::update, METH_VARARGS, "Update the progress by n steps - update( int n )" },
    {"reset", (PyCFunction)VRPyProgress::reset, METH_NOARGS, "Reset the progress" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyProgress::setup(VRPyProgress* self, PyObject* args) {
    if (!self->valid()) return NULL;
    // TODO
    Py_RETURN_TRUE;
}

PyObject* VRPyProgress::update(VRPyProgress* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->update( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyProgress::get(VRPyProgress* self) {
    if (!self->valid()) return NULL;
    return PyFloat_FromDouble( self->objPtr->get() );
}

PyObject* VRPyProgress::reset(VRPyProgress* self) {
    if (!self->valid()) return NULL;
    self->objPtr->reset();
    Py_RETURN_TRUE;
}
