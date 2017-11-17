#ifndef VRPYRAIN_H_INCLUDED
#define VRPYRAIN_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRRain.h"

struct VRPyRain: VRPyBaseT<OSG::VRRain> {
    static PyMethodDef methods[];
};

#endif // VRPYRAIN_H_INCLUDED
