#include "VRPyCEF.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/objects/VRObjectFwd.h"

using namespace OSG;

simplePyType(CEF, New_ptr);

PyMethodDef VRPyCEF::methods[] = {
    {"open", PyWrap2(CEF, open, "Open URL", void, string) },
    {"setMaterial", PyWrap2(CEF, setMaterial, "Set material", void, VRMaterialPtr) },
    {"addMouse", PyWrap2(CEF, addMouse, "Add mouse interaction - addMouse(mouse, geo, LMB, RMB, SCRUP, SCRDOWN)", void, VRDevicePtr, VRObjectPtr, int, int, int, int) },
    {"addKeyboard", PyWrap2(CEF, addKeyboard, "Add keyboard device", void, VRDevicePtr) },
    {"setResolution", PyWrap2(CEF, setResolution, "Set horizontal resolution W", void, float) },
    {"setAspectRatio", PyWrap2(CEF, setAspectRatio, "Set aspect ratio a to define the height H: H = W/a", void, float) },
    {NULL}  /* Sentinel */
};
