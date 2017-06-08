#include "VRPyAnimation.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

#include "core/scene/VRAnimationManagerT.h"

using namespace OSG;

simpleVRPyType(Animation, New_named_ptr);

PyMethodDef VRPyAnimation::methods[] = {
    {"start", PyWrapOpt(Animation, start, "Start animation, pass an optional offset in seconds", "0", void, float) },
    {"stop", PyWrap(Animation, stop, "Stop animation", void) },
    {"isActive", PyWrap(Animation, isActive, "Check if running", bool) },
    {"setCallback", PyWrap(Animation, setCallback, "Set animation callback", void, VRAnimCbPtr) },
    {"setDuration", PyWrap(Animation, setDuration, "Set animation duration", void, float) },
    {"setLoop", PyWrap(Animation, setLoop, "Set animation loop flag", void, bool) },
    {NULL}  /* Sentinel */
};

/*PyObject* VRPyAnimation::setDuration(VRPyAnimation* self, PyObject* args) {
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
}*/


