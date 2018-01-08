#ifndef VRPYTRAFFICSIMULATION_H_INCLUDED
#define VRPYTRAFFICSIMULATION_H_INCLUDED


#include "core/scripting/VRPyBase.h"
#include "VRTrafficSimulation.h"

struct VRPyTrafficSimulation : VRPyBaseT<OSG::VRTrafficSimulation> {
    static PyMethodDef methods[];
};


#endif // VRPYTRAFFICSIMULATION_H_INCLUDED
