#include "VRPyNavPreset.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(NavPreset, 0);

PyMethodDef VRPyNavPreset::methods[] = {
    {"setSpeed", (PyCFunction)VRPyNavPreset::setSpeed, METH_VARARGS, "Set the translation and rotation speed - setSpeed( t, r )" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyNavPreset::setSpeed(VRPyNavPreset* self, PyObject* args) {
    if (!self->valid()) return NULL;
    OSG::Vec2d s = parseVec2f(args);
    self->objPtr->setSpeed(s[0], s[1]);
    Py_RETURN_TRUE;
}
