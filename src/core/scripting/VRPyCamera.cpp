#include "VRPyCamera.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Camera, New_VRObjects_ptr);

PyMethodDef VRPyCamera::methods[] = {
    {"activate", (PyCFunction)VRPyCamera::activate, METH_NOARGS, "Switch to camera - activate()" },
    {"setFov", (PyCFunction)VRPyCamera::setFov, METH_VARARGS, "Set the camera's field of view" },
    {"focus", (PyCFunction)VRPyCamera::focus, METH_VARARGS, "Set the camera's position to see the whole scene under the object - focus(object)" },
    {"setAspect", PyWrap(Camera, setAspect, "Set camera aspect ratio", void, float) },
    {"setFov", PyWrap(Camera, setFov, "Set camera field of view", void, float) },
    {"setNear", PyWrap(Camera, setNear, "Set camera near clipping", void, float) },
    {"setFar", PyWrap(Camera, setFar, "Set camera far clipping", void, float) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCamera::activate(VRPyCamera* self) {
    if (!self->valid()) return NULL;
    self->objPtr->activate();
    Py_RETURN_TRUE;
}

PyObject* VRPyCamera::setFov(VRPyCamera* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float fov = parseFloat(args);
    self->objPtr->setFov(fov);
    Py_RETURN_TRUE;
}

PyObject* VRPyCamera::focus(VRPyCamera* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* o;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->focus(o->objPtr);
    Py_RETURN_TRUE;
}
