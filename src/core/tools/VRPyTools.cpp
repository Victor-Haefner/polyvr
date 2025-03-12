#include "VRPyTools.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Player, New_ptr);
simpleVRPyType(Timeline, New_ptr);
simpleVRPyType(Gizmo, New_VRObjects_ptr);

PyMethodDef VRPyPlayer::methods[] = {
    {"setCallback", PyWrap( Player, setCallback, "Set callback - def cb(t)", void, VRAnimCbPtr ) },
    {"reset", PyWrap( Player, reset, "reset to start - progress = 0", void ) },
    {"pause", PyWrap( Player, pause, "pause", void ) },
    {"play", PyWrap( Player, play, "play( speed modifier )", void, double ) },
    {"moveTo", PyWrap( Player, moveTo, "moveTo( progress ) - progress: [0..1]", void, double ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyTimeline::methods[] = {
    //{"setGraph", PyWrap( Timeline, setGraph, "Set graph", void, GraphPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyGizmo::methods[] = {
    //{"setGraph", PyWrap( Gizmo, setGraph, "Set graph", void, GraphPtr ) },
    {NULL}  /* Sentinel */
};
