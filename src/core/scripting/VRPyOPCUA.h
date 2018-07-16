#ifndef VRPYOPCUA_H_INCLUDED
#define VRPYOPCUA_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/networking/VROPCUA.h"

struct VRPyOPCUANode : VRPyBaseT<OSG::VROPCUANode> {
    static PyMethodDef methods[];
};

struct VRPyOPCUA : VRPyBaseT<OSG::VROPCUA> {
    static PyMethodDef methods[];
};

#endif // VRPYOPCUA_H_INCLUDED
