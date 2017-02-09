#include "VRPyAnimation.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

#include "core/scene/VRAnimationManagerT.h"

using namespace OSG;

simpleVRPyType(Animation, New_named_ptr);

PyMethodDef VRPyAnimation::methods[] = {
    {"start", (PyCFunction)VRPyAnimation::start, METH_VARARGS, "Start animation" },
    {"stop", (PyCFunction)VRPyAnimation::stop, METH_NOARGS, "Stop animation" },
    {"isActive", (PyCFunction)VRPyAnimation::isActive, METH_NOARGS, "Check if running - bool isActive()" },
    {"setCallback", (PyCFunction)VRPyAnimation::setCallback, METH_VARARGS, "Set animation callback - setCallback(callback)" },
    {"setDuration", (PyCFunction)VRPyAnimation::setDuration, METH_VARARGS, "Set animation duration - setDuration(float)" },
    {"setLoop", (PyCFunction)VRPyAnimation::setLoop, METH_VARARGS, "Set animation loop flag - setLoop(bool)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAnimation::setDuration(VRPyAnimation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->setDuration( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyAnimation::setLoop(VRPyAnimation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    self->objPtr->setLoop( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyAnimation::setCallback(VRPyAnimation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    auto cb = parseCallback<float>(args); if (cb == 0) return NULL;
    VRAnimCbPtr acb = shared_ptr< VRFunction<float> >(cb);
    self->objPtr->setSimpleCallback(acb, 1);
    self->objPtr->setCallbackOwner(true);
    Py_RETURN_TRUE;
}

PyObject* VRPyAnimation::start(VRPyAnimation* self, PyObject* args) {
	if (!self->valid()) return NULL;
    float offset = 0;
    if (pySize(args) == 1) offset = parseFloat(args);
    self->objPtr->start(offset);
    Py_RETURN_TRUE;
}

PyObject* VRPyAnimation::stop(VRPyAnimation* self) {
	if (!self->valid()) return NULL;
    self->objPtr->stop();
    Py_RETURN_TRUE;
}

PyObject* VRPyAnimation::isActive(VRPyAnimation* self) {
	if (!self->valid()) return NULL;
    return PyBool_FromLong( self->objPtr->isActive() );
}


