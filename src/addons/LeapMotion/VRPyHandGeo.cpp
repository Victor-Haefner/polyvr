#include "VRPyHandGeo.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"
#include "VRPyLeap.h"

using namespace OSG;

simpleVRPyType(HandGeo, New_VRObjects_ptr);

PyMethodDef VRPyHandGeo::methods[] = {
    {"update", (PyCFunction)VRPyHandGeo::update, METH_VARARGS, "Update hand with new frame" },
    {"setLeft", (PyCFunction)VRPyHandGeo::setLeft, METH_VARARGS, "Add description" },
    {"setRight", (PyCFunction)VRPyHandGeo::setRight, METH_VARARGS, "Add description" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyHandGeo::update(VRPyHandGeo *self, PyObject *args) {
    if (!self->valid()) return NULL;
    VRPyLeapFrame* frame = 0;
    if (!PyArg_ParseTuple(args, "O", &frame)) return NULL;
    self->objPtr->update(frame->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyHandGeo::setLeft(VRPyHandGeo* self) {
    if (!self->valid()) return NULL;
    self->objPtr->setLeft();
    Py_RETURN_TRUE;
}

PyObject* VRPyHandGeo::setRight(VRPyHandGeo* self) {
    if (!self->valid()) return NULL;
    self->objPtr->setRight();
    Py_RETURN_TRUE;
}