#include "VRPyTools.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Player, New_ptr);
simpleVRPyType(Timeline, New_ptr);
simpleVRPyType(Gizmo, New_VRObjects_ptr);

PyMethodDef VRPyPlayer::methods[] = {
    {"setCallback", PyWrap( Player, setCallback, "Set callback - def cb(t)", void, VRAnimCbPtr ) },
    {"setLoop", PyWrap( Player, setLoop, "Set looping", void, bool ) },
    {"reset", PyWrap( Player, reset, "reset to start - progress = 0", void ) },
    {"pause", PyWrap( Player, pause, "pause", void ) },
    {"play", PyWrap( Player, play, "play( speed modifier )", void, double ) },
    {"moveTo", PyWrap( Player, moveTo, "moveTo( progress ) - progress: [0..1]", void, double ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTimeline::methods[] = {
    {"addCallback", PyWrap( Timeline, addCallback, "Add callback - addCallback(t1, t2, cb) - def cb(t) - t12: [0..1]", void, double, double, VRAnimCbPtr ) },
    {"addTimeline", PyWrap( Timeline, addTimeline, "Add callback - addTimeline(t1, t2, timeline) - t12: [0..1]", void, double, double, VRTimelinePtr ) },
    {"setTime", PyWrap( Timeline, setTime, "Add callback - setTime(t) - t: [0..1]", void, double ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyGizmo::methods[] = {
    //{"setGraph", PyWrap( Gizmo, setGraph, "Set graph", void, GraphPtr ) },
    {NULL}  /* Sentinel */
};
