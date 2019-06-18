#ifndef VRPYKINEMATICS_H_INCLUDED
#define VRPYKINEMATICS_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "core/math/kinematics/VRKinematics.h"
#include "core/math/kinematics/VRFABRIK.h"

struct VRPyKinematics : VRPyBaseT<OSG::VRKinematics> {
    static PyMethodDef methods[];
};

struct VRPyFABRIK : VRPyBaseT<OSG::FABRIK> {
    static PyMethodDef methods[];
};

#endif // VRPYKINEMATICS_H_INCLUDED
