#include "VRPyLight.h"
#include "VRPyLightBeacon.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Light, New_VRObjects_ptr);

PyMethodDef VRPyLight::methods[] = {
    {"setOn", (PyCFunction)VRPyLight::setOn, METH_VARARGS, "Set light state - setOn(bool)" },
    {"setBeacon", (PyCFunction)VRPyLight::setBeacon, METH_VARARGS, "Set beacon - setBeacon( beacon )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyLight::setBeacon(VRPyLight* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyLight::setBeacon - Object is invalid"); return NULL; }
    VRPyLightBeacon* b = 0;
    if (! PyArg_ParseTuple(args, "O", &b)) return NULL;
    self->objPtr->setBeacon( b->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyLight::setOn(VRPyLight* self, PyObject *args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyLight::setOn - Object is invalid"); return NULL; }
    self->objPtr->setOn( parseBool(args) );
    Py_RETURN_TRUE;
}

