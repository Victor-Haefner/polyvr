#include "VRPyCamera.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Camera, New_VRObjects_ptr);

PyMethodDef VRPyCamera::methods[] = {
    {"activate", PyWrap(Camera, activate, "Switch to camera", void) },
    {"setFov", PyWrap(Camera, setFov, "Set the camera's field of view", void, float) },
    {"focus", PyWrap(Camera, focusObject, "Set the camera's position to see the whole scene under the object", void, VRObjectPtr ) },
    {"setAspect", PyWrap(Camera, setAspect, "Set camera aspect ratio", void, float) },
    {"setFov", PyWrap(Camera, setFov, "Set camera field of view", void, float) },
    {"setNear", PyWrap(Camera, setNear, "Set camera near clipping", void, float) },
    {"setFar", PyWrap(Camera, setFar, "Set camera far clipping", void, float) },
    {"getFov", PyWrap(Camera, getFov, "Get camera field of view", float) },
    {"getNear", PyWrap(Camera, getNear, "Get camera near clipping", float) },
    {"getFar", PyWrap(Camera, getFar, "Get camera far clipping", float) },
    {"setType", PyWrap(Camera, setType, "Set camera type, 0 for perspective and 1 for orthographic projection", void, int) },
    {NULL}  /* Sentinel */
};
