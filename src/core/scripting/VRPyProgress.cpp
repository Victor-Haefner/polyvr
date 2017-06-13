#include "VRPyProgress.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType( Progress, New_ptr );

PyMethodDef VRPyProgress::methods[] = {
    {"setup", PyWrapOpt(Progress, setup, "Setup the progress parameters", "0", void, string, int, int) },
    {"get", PyWrap(Progress, get, "Get the current state t[0,1]", float) },
    {"set", PyWrap(Progress, set, "Set the current state t[0,1]", void, float) },
    {"update", PyWrap(Progress, update, "Update the progress by n steps", void, int) },
    {"reset", PyWrap(Progress, reset, "Reset the progress", void) },
    {NULL}  /* Sentinel */
};
