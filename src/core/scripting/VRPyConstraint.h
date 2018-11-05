#ifndef VRPYCONSTRAINT_H_INCLUDED
#define VRPYCONSTRAINT_H_INCLUDED

#include "VRPyObject.h"
#include "core/math/kinematics/VRConstraint.h"

struct VRPyConstraint : VRPyBaseT<OSG::VRConstraint> {
    static PyMethodDef methods[];
};

#endif // VRPYCONSTRAINT_H_INCLUDED
