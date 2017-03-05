#ifndef VRPYCHARACTER_H_INCLUDED
#define VRPYCHARACTER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRCharacter.h"

struct VRPySkeleton : VRPyBaseT<OSG::VRSkeleton> {
    static PyMethodDef methods[];
};

struct VRPyCharacter : VRPyBaseT<OSG::VRCharacter> {
    static PyMethodDef methods[];
    static PyObject* setSkeleton(VRPyCharacter* self, PyObject* args);
    static PyObject* setSkin(VRPyCharacter* self, PyObject* args);
    static PyObject* addBehavior(VRPyCharacter* self, PyObject* args);
    static PyObject* triggerBehavior(VRPyCharacter* self, PyObject* args);
    static PyObject* simpleSetup(VRPyCharacter* self);
};

#endif // VRPYCHARACTER_H_INCLUDED
