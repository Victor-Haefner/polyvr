#include "VRPySetup.h"
#include "VRPyBaseT.h"

#include "core/setup/windows/VRView.h"
#include "core/scripting/VRPyPose.h"
#include "core/scripting/VRPyImage.h"
#include "core/scripting/VRPyCamera.h"
#include "core/scripting/VRPyMath.h"

using namespace OSG;

simpleVRPyType(ViewManager, 0);
simpleVRPyType(WindowManager, 0);
simpleVRPyType(Setup, 0);
simpleVRPyType(View, 0);
simpleVRPyType(Window, 0);
simpleVRPyType(WebXR, 0);


PyMethodDef VRPyViewManager::methods[] = {
    {"getView", PyWrap(ViewManager, getView, "Get view by index", VRViewPtr, int ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyWindowManager::methods[] = {
    {"getWindow", PyWrap(WindowManager, getWindow, "Get window by name", VRWindowPtr, string ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPySetup::methods[] = {
    {"getUser", PyWrap(Setup, getUser, "Get user", VRTransformPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyView::methods[] = {
    {"toggleStereo", PyWrap(View, toggleStereo, "Toggle stereo mode", void ) },
    {"toggleStats", PyWrap(View, toggleStats, "Toggle displayed stats", void) },
    {"setPose", PyWrap(View, setPose, "Set the pose", void, PosePtr) },
    {"getPose", PyWrap(View, getPose, "Get the pose", PosePtr) },
    {"setProjectionSize", PyWrap(View, setProjectionSize, "Set the size in meter", void, Vec2d) },
    {"getProjectionSize", PyWrap(View, getProjectionSize, "Get the size in meter", Vec2d) },
    {"setSize", PyWrap(View, setSize, "Set the size in pixel", void, Vec2i) },
    {"getSize", PyWrap(View, getSize, "Get the size in pixel", Vec2i) },
    {"setStereoEyeSeparation", PyWrap(View, setStereoEyeSeparation, "Set the distance between eyes for stereo, typical is 0.065", void, float) },
    {"grab", PyWrap(View, grab, "Get the current visual as texture", VRTexturePtr) },
    {"setCamera", PyWrap(View, setCamera, "Set the camera of the view", void, VRCameraPtr ) },
    {"getName", PyWrap(View, getName, "Get the name of the view", string ) },
    {"getUser", PyWrap(View, getUser, "Get the user node", VRTransformPtr ) },
    {"testUpdate", PyWrap(View, testUpdate, "Trigger a test update for debug purpose", void ) },
    {"getRoot", PyWrap(View, getRoot, "Get the root node of the view", VRObjectPtr ) },
    {"getRenderingL", PyWrap(View, getRenderingL, "Get the left rendering studio", VRRenderStudioPtr ) },
    {"getRenderingR", PyWrap(View, getRenderingR, "Get the right rendering studio", VRRenderStudioPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyWindow::methods[] = {
    {"getSize", PyWrap(Window, getSize, "Get window size (width, height) in pixel", Vec2i ) },
    {"setTitle", PyWrap(Window, setTitle, "Set window title bar", void, string ) },
    {"setIcon", PyWrap(Window, setIcon, "Set window icon", void, string ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyWebXR::methods[] = {
    {NULL}  /* Sentinel */
};


