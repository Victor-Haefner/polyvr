#ifndef VRPYCLIPPLANE_H_INCLUDED
#define VRPYCLIPPLANE_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRClipPlane.h"

struct VRPyClipPlane : VRPyBaseT<OSG::VRClipPlane> {
    static PyMethodDef methods[];
};

#endif // VRPYCLIPPLANE_H_INCLUDED
