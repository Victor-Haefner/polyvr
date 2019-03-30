#ifndef VRPYKINEMATICS_H_INCLUDED
#define VRPYKINEMATICS_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/math/kinematics/VRKinematics.h"

struct VRPyKinematics : VRPyBaseT<OSG::VRKinematics> {
    static PyMethodDef methods[];
};

#endif // VRPYKINEMATICS_H_INCLUDED
