#ifndef VRPYSTATEMACHINE_H_INCLUDED
#define VRPYSTATEMACHINE_H_INCLUDED

#include "core/math/VRStateMachine.h"
#include "core/scripting/VRPyBase.h"

struct VRPyStateMachine : VRPyBaseT<OSG::VRStateMachine> {
    static PyMethodDef methods[];

    static PyObject* setState(VRPyStateMachine* self, PyObject *args);
    static PyObject* getState(VRPyStateMachine* self, PyObject *args);
    static PyObject* addState(VRPyStateMachine* self, PyObject *args);
};

#endif // VRPYSTATEMACHINE_H_INCLUDED
