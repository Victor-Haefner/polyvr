#include "VRPyAnimation.h"
#include "VRPyBaseT.h"

#include "core/scene/VRAnimationManagerT.h"
#include "core/objects/VRKeyFrameAnimation.h"

using namespace OSG;

simpleVRPyType(Animation, New_named_ptr);
simpleVRPyType(KeyFrameAnimation, New_named_ptr);

PyMethodDef VRPyAnimation::methods[] = {
    {"start", PyWrapOpt(Animation, start, "Start animation, pass an optional offset in seconds", "0", void, float) },
    {"stop", PyWrap(Animation, stop, "Stop animation", void) },
    {"isActive", PyWrap(Animation, isActive, "Check if running", bool) },
    {"pause", PyWrap(Animation, pause, "Pause animation", void) },
    {"resume", PyWrap(Animation, resume, "Resume if paused", void) },
    {"isPaused", PyWrap(Animation, isPaused, "Check if paused", bool) },
    {"setCallback", PyWrap(Animation, addCallback, "Set animation callback", void, VRAnimCbPtr) },
    {"addCallback", PyWrap(Animation, addCallback, "Set animation callback", void, VRAnimCbPtr) },
    {"setDuration", PyWrap(Animation, setDuration, "Set animation duration", void, float) },
    {"setLoop", PyWrap(Animation, setLoop, "Set animation loop flag", void, bool) },
    {"getDuration", PyWrap(Animation, getDuration, "Return total animation duration", float) },
    {"getLoop", PyWrap(Animation, getLoop, "Return loop settings flag", bool) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyKeyFrameAnimation::methods[] = {
    {"addSource", PyWrap(KeyFrameAnimation, addSource, "Add animation source data (ID, [data])", void, string, int, vector<float>&) },
    {"addInterpolation", PyWrap(KeyFrameAnimation, addInterpolation, "Add animation interpolation data (ID, [LINEAR])", void, string, vector<string>&) },
    {"addChannel", PyWrap(KeyFrameAnimation, addChannel, "Add animation channel (ID, property, target, {'INPUT':sourceInID, 'OUTPUT':sourceOutID, 'INTERPOLATION':sourceIntID})", void, string, string, VRTransformPtr,  map<string, string>&) },
    {NULL}  /* Sentinel */
};
