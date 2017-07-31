#ifndef VRPYSKY_H_INCLUDED
#define VRPYSKY_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/objects/geometry/VRSky.h"

struct VRPySky : VRPyBaseT<OSG::VRSky> {
    static PyMethodDef methods[];
};


#endif // VRPYSKY_H_INCLUDED
