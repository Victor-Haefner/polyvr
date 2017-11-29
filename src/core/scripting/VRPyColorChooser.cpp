#include "VRPyColorChooser.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(ColorChooser, New_ptr);

PyMethodDef VRPyColorChooser::methods[] = {
    {"setGeometry", PyWrap(ColorChooser, setGeometry, "Set the geometry", void, VRGeometryPtr ) },
    {"set", PyWrap(ColorChooser, setColor, "Set the color", void, Color3f ) },
    {"get", PyWrap(ColorChooser, getColor, "Return the active color", Color3f ) },
    {"getLast", PyWrap(ColorChooser, getLastColor, "Return the previous color", Color3f ) },
    {"resolve", PyWrap(ColorChooser, resolve, "Get the color from device interaction", void, VRDevicePtr ) },
    {NULL}  /* Sentinel */
};
