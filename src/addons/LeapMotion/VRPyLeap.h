#pragma once

#include "core/scripting/VRPyBase.h"
#include "addons/LeapMotion/VRLeap.h"

struct VRPyLeapFrame : VRPyBaseT<OSG::VRLeapFrame> {
    static PyMethodDef methods[];
};

struct VRPyLeap : VRPyBaseT<OSG::VRLeap> {
    static PyMethodDef methods[];

    static PyObject* registerFrameCallback(VRPyLeap* self, PyObject* args);
};
