#include "VRPyMillingCuttingToolProfile.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"

using namespace OSG;

simpleVRPyType(MillingCuttingToolProfile, New_ptr);

PyMethodDef VRPyMillingCuttingToolProfile::methods[] = {
    {"addPointProfile", (PyCFunction)VRPyMillingCuttingToolProfile::addPointProfile, METH_VARARGS,
    "addPointProfile() - adds a point to the cutting tool profile."},
    {NULL}  /* Sentinel */
};

//Add of Marie
PyObject* VRPyMillingCuttingToolProfile::addPointProfile(VRPyMillingCuttingToolProfile* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* newPoint;
    if (!PyArg_ParseTuple(args, "O", &newPoint)) return NULL;
    auto p = parseVec2fList(newPoint);
    self->objPtr->addPointProfile(p);
    Py_RETURN_TRUE;
}
