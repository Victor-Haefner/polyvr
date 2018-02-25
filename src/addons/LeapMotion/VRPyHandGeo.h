#pragma once

#include "core/scripting/VRPyObject.h"
#include "VRHandGeo.h"

struct VRPyHandGeo : VRPyBaseT<OSG::VRHandGeo> {
    static PyMethodDef methods[];
};
