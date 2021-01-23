#include "VRPyNavPreset.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(NavPreset, 0);

PyMethodDef VRPyNavPreset::methods[] = {
    {"setSpeed", PyWrap(NavPreset, setSpeed, "Set the translation and rotation speed, ( trans, rot )", void, float, float) },
    {"activate", PyWrap(NavPreset, activate, "Activate the navigation", void) },
    {"deactivate", PyWrap(NavPreset, deactivate, "Deactivate the navigation", void) },
    {"setBindingState", PyWrap(NavPreset, setBindingState, "Set state of a navigation binding", void, size_t, bool) },
    {"setDevice", PyWrap(NavPreset, setDevice, "Set navigation device", void, VRDevicePtr) },
    {"setTarget", PyWrap(NavPreset, setTarget, "Set navigation target", void, VRTransformPtr) },
    {NULL}  /* Sentinel */
};
