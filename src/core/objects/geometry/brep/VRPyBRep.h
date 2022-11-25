#ifndef VRPYBREP_H_INCLUDED
#define VRPYBREP_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRBRepSurface.h"
#include "VRBRepEdge.h"
#include "VRBRepBound.h"

struct VRPyBRepSurface : VRPyBaseT<OSG::VRBRepSurface> {
    static PyMethodDef methods[];
};

struct VRPyBRepEdge : VRPyBaseT<OSG::VRBRepEdge> {
    static PyMethodDef methods[];
};

struct VRPyBRepBound : VRPyBaseT<OSG::VRBRepBound> {
    static PyMethodDef methods[];
};

#endif // VRPYBREP_H_INCLUDED
