#ifndef VRPYSTATEMACHINE_H_INCLUDED
#define VRPYSTATEMACHINE_H_INCLUDED

#include "core/math/VRStateMachine.h"
#include "core/scripting/VRPyBase.h"

struct VRPyState : VRPyBaseT<OSG::VRStateMachinePy::State> {
    static PyMethodDef methods[];

    static PyObject* getName(VRPyState* self);
};

struct VRPyStateMachine : VRPyBaseT<OSG::VRStateMachinePy> {
    static PyMethodDef methods[];

    static PyObject* addState(VRPyStateMachine* self, PyObject *args);
    static PyObject* getState(VRPyStateMachine* self, PyObject *args);
    static PyObject* setCurrentState(VRPyStateMachine* self, PyObject *args);
    static PyObject* getCurrentState(VRPyStateMachine* self);
    static PyObject* process(VRPyStateMachine* self, PyObject *args);
};

#endif // VRPYSTATEMACHINE_H_INCLUDED
