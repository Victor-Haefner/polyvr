#ifndef VRPYNUMBERINGENGINE_H_INCLUDED
#define VRPYNUMBERINGENGINE_H_INCLUDED

#include "VRNumberingEngine.h"
#include "core/scripting/VRPyBase.h"

struct VRPyNumberingEngine : VRPyBaseT<OSG::VRNumberingEngine> {
    static PyMethodDef methods[];
};

#endif // VRPYNUMBERINGENGINE_H_INCLUDED
