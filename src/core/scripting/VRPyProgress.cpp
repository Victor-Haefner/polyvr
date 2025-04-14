#include "VRPyProgress.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType( Progress, New_ptr );

template<> string typeName(const VRProgress::Mode* t) { return "Progress Mode"; }
template<> bool toValue(PyObject* o, VRProgress::Mode& v) { if (!PyLong_Check(o)) return 0; v = (VRProgress::Mode)PyLong_AsLong(o); return 1; }

PyMethodDef VRPyProgress::methods[] = {
    {"setup", PyWrapOpt(Progress, setup, "Setup the progress parameters", "0", void, string, size_t, VRProgress::Mode) },
    {"get", PyWrap(Progress, get, "Get the current state t[0,1]", float) },
    {"set", PyWrap(Progress, set, "Set the current state t[0,1]", void, float) },
    {"update", PyWrapOpt(Progress, update, "Update the progress by n steps, optional signal delta (n, delta=0.01)", "0.01", void, size_t, double) },
    {"reset", PyWrap(Progress, reset, "Reset the progress", void) },
    {NULL}  /* Sentinel */
};
