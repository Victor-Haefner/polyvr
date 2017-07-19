#include "VRPyHaptic.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

#include "VRPyTransform.h"

using namespace OSG;
simpleVRPyType(Haptic, 0);

PyMethodDef VRPyHaptic::methods[] = {
    {"setSimulationScales", (PyCFunction)VRPyHaptic::setSimulationScales, METH_VARARGS, "Set force on haptic device" },
    {"setForce", (PyCFunction)VRPyHaptic::setForce, METH_VARARGS, "Set force on haptic device" },
    {"getForce", (PyCFunction)VRPyHaptic::getForce, METH_NOARGS, "get 3-Tuple positional force the user generated" },
    {"attachTransform", (PyCFunction)VRPyHaptic::attachTransform, METH_VARARGS, "attaches given Transform to the virtuose (Command-Mode has to be COMMAND_MODE_VIRTMECH) Gravity for this transform should be set to zero,"},
    {"detachTransform", (PyCFunction)VRPyHaptic::detachTransform, METH_NOARGS, "detach previously attached transform" },
    {"getButtonStates",(PyCFunction)VRPyHaptic::getButtonStates, METH_NOARGS,"return a 3-Tuple with the states of virtuose's three buttons. e.g. (0,0,1) means the 3rd button is active, the others not"},
    {"setBase",(PyCFunction)VRPyHaptic::setBase, METH_VARARGS,"sets the given transform to the representation of the haptic's base"},
    {NULL}  /* Sentinel */
};


PyObject* VRPyHaptic::setSimulationScales(VRPyHaptic* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyHaptic::setSimulationScales - objPtrect is invalid"); return NULL; }
    OSG::Vec2d v = parseVec2f(args);
    self->objPtr->setSimulationScales(v[0], v[1]);
    Py_RETURN_TRUE;
}

PyObject* VRPyHaptic::setForce(VRPyHaptic* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyHaptic::setForce - objPtrect is invalid"); return NULL; }
    float x,y,z,u,v,w;
    if (! PyArg_ParseTuple(args, "ffffff", &x, &y, &z, &u, &v, &w)) return NULL;
    self->objPtr->setForce(OSG::Vec3d(x,y,z), OSG::Vec3d(u,v,w));
    Py_RETURN_TRUE;
}

PyObject* VRPyHaptic::getButtonStates(VRPyHaptic* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyHaptic::getButtonStates - objPtrect is invalid"); return NULL; }
    OSG::Vec3i states = self->objPtr->getButtonStates();
    return toPyTuple(states);
}

PyObject* VRPyHaptic::getForce(VRPyHaptic* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyHaptic::getForce - objPtrect is invalid"); return NULL; }
    OSG::Vec3d force = self->objPtr->getForce();
    return toPyTuple(force);
}

PyObject* VRPyHaptic::attachTransform(VRPyHaptic* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyHaptic::attachTransform - objPtrect is invalid"); return NULL; }
    VRPyTransform* tr;
    if (! PyArg_ParseTuple(args, "O", &tr)) return NULL;
    self->objPtr->attachTransform(tr->objPtr);
    Py_RETURN_TRUE;
}
PyObject* VRPyHaptic::setBase(VRPyHaptic* self, PyObject* args) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyHaptic::setBase - objPtrect is invalid"); return NULL; }
    VRPyTransform* tr;
    if (! PyArg_ParseTuple(args, "O", &tr)) return NULL;
    self->objPtr->setBase(tr->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyHaptic::detachTransform(VRPyHaptic* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyHaptic::updateHapticToobjPtrect - objPtrect is invalid"); return NULL; }
    self->objPtr->detachTransform();
    Py_RETURN_TRUE;
}




