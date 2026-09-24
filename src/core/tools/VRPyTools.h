#ifndef VRPYTOOLS_H_INCLUDED
#define VRPYTOOLS_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRPlayer.h"
#include "VRTimeline.h"
#include "VRGizmo.h"

struct VRPyPlayer : VRPyBaseT<OSG::VRPlayer> {
    static PyMethodDef methods[];
};

struct VRPyTimeline : VRPyBaseT<OSG::VRTimeline> {
    static PyMethodDef methods[];
};

struct VRPyGizmo : VRPyBaseT<OSG::VRGizmo> {
    static PyMethodDef methods[];
};

#endif // VRPYTOOLS_H_INCLUDED
