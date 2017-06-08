#include "VRPyAnimation.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

#include "core/scene/VRAnimationManagerT.h"

using namespace OSG;

simpleVRPyType(Animation, New_named_ptr);

PyMethodDef VRPyAnimation::methods[] = {
    {"start", PyWrapOpt(Animation, start, void, float) PyWrapParams("0"), "Start animation" },
    {"stop", PyWrap(Animation, stop, void), "Stop animation" },
    {"isActive", PyWrap(Animation, isActive, bool), "Check if running - bool isActive()" },
    {"setCallback", PyWrap(Animation, setCallback, void, VRAnimCbPtr), "Set animation callback - setCallback(callback)" },
    {"setDuration", PyWrap(Animation, setDuration, void, float), "Set animation duration - setDuration(float)" },
    {"setLoop", PyWrap(Animation, setLoop, void, bool), "Set animation loop flag - setLoop(bool)" },
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


