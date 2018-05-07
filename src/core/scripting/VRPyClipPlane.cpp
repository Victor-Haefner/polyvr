#include "VRPyClipPlane.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(ClipPlane, New_VRObjects_ptr);

PyMethodDef VRPyClipPlane::methods[] = {
    {"setTree", PyWrap(ClipPlane, setTree, "Set the tree to apply the clipping plane to", void, VRObjectPtr) },
    {"setActive", PyWrap(ClipPlane, setActive, "Activate and deactivate the clipping", void, bool) },
    {"isActive", PyWrap(ClipPlane, isActive, "Check if active", bool) },
    {"setSize", PyWrap(ClipPlane, setSize, "Set handle size", void, float, float) },
    {NULL}  /* Sentinel */
};
