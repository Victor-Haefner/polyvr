#include "VRPyWebCam.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(WebCam, New_ptr);

PyMethodDef VRPyWebCam::methods[] = {
    {"connect", PyWrap( WebCam, connect, "Connect to a webcam raw stream: uri, resolution, aspect ratio", void, string, int, float ) },
    {NULL}  /* Sentinel */
};
