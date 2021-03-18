#include "VRPyAnimation.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

#include "core/scene/VRAnimationManagerT.h"

using namespace OSG;

simpleVRPyType(Animation, New_named_ptr);

PyMethodDef VRPyAnimation::methods[] = {
    {"start", PyWrapOpt(Animation, start, "Start animation, pass an optional offset in seconds", "0", void, float) },
    {"stop", PyWrap(Animation, stop, "Stop animation", void) },
    {"isActive", PyWrap(Animation, isActive, "Check if running", bool) },
    {"pause", PyWrap(Animation, pause, "Pause animation", void) },
    {"resume", PyWrap(Animation, resume, "Resume if paused", void) },
    {"isPaused", PyWrap(Animation, isPaused, "Check if paused", bool) },
    {"setCallback", PyWrap(Animation, setCallback, "Set animation callback", void, VRAnimCbPtr) },
    {"setDuration", PyWrap(Animation, setDuration, "Set animation duration", void, float) },
    {"setLoop", PyWrap(Animation, setLoop, "Set animation loop flag", void, bool) },
    {"getDuration", PyWrap(Animation, getDuration, "Return total animation duration", float) },
    {"getLoop", PyWrap(Animation, getLoop, "Return loop settings flag", bool) },
    {NULL}  /* Sentinel */
};
