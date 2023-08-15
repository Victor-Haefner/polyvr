#include "VRPyCEF.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/objects/VRObjectFwd.h"

using namespace OSG;

simplePyType(CEF, New_ptr);
simpleVRPyType(Gui, New_ptr);

PyMethodDef VRPyCEF::methods[] = {
    {"open", PyWrap2(CEF, open, "Open URL", void, string) },
    {"setMaterial", PyWrap2(CEF, setMaterial, "Set material", void, VRMaterialPtr) },
    {"addMouse", PyWrap2(CEF, addMouse, "Add mouse interaction - addMouse(mouse, geo, LMB, MMB, RMB, SCRUP, SCRDOWN)", void, VRDevicePtr, VRObjectPtr, int, int, int, int, int) },
    {"addKeyboard", PyWrap2(CEF, addKeyboard, "Add keyboard device", void, VRDevicePtr) },
    {"setResolution", PyWrap2(CEF, setResolution, "Set horizontal resolution W", void, float) },
    {"setAspectRatio", PyWrap2(CEF, setAspectRatio, "Set aspect ratio a to define the height H: H = W/a", void, float) },
    {"toggleInput", PyWrap2(CEF, toggleInput, "Toggle mouse and keyboard input usage (useMouse, useKeyboard)", void, bool, bool) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyGui::methods[] = {
    {"open", PyWrap(Gui, open, "Open URL", void, string) },
    {"setMaterial", PyWrap(Gui, setMaterial, "Set material", void, VRMaterialPtr) },
    {"addMouse", PyWrap(Gui, addMouse, "Add mouse interaction - addMouse(mouse, geo, LMB, RMB, SCRUP, SCRDOWN)", void, VRDevicePtr, VRObjectPtr, int, int, int, int) },
    {"addKeyboard", PyWrap(Gui, addKeyboard, "Add keyboard device", void, VRDevicePtr) },
    {"setResolution", PyWrap(Gui, setResolution, "Set horizontal resolution W", void, float) },
    {"setAspectRatio", PyWrap(Gui, setAspectRatio, "Set aspect ratio a to define the height H: H = W/a", void, float) },
    {"toggleInput", PyWrap(Gui, toggleInput, "Toggle mouse and keyboard input usage (useMouse, useKeyboard)", void, bool, bool) },
    {NULL}  /* Sentinel */
};
