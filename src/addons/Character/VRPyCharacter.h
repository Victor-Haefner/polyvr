#ifndef VRPYCHARACTER_H_INCLUDED
#define VRPYCHARACTER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRCharacter.h"
#include "VRSkeleton.h"
#include "VRBehavior.h"

struct VRPyBehavior : VRPyBaseT<OSG::VRBehavior> {
    static PyMethodDef methods[];
};

struct VRPySkeleton : VRPyBaseT<OSG::VRSkeleton> {
    static PyMethodDef methods[];
};

struct VRPyCharacter : VRPyBaseT<OSG::VRCharacter> {
    static PyMethodDef methods[];
};

#endif // VRPYCHARACTER_H_INCLUDED
