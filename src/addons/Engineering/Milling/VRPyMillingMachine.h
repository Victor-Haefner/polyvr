#ifndef VRPYMILLINGMACHINE_H_INCLUDED
#define VRPYMILLINGMACHINE_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRMillingMachine.h"

struct VRPyMillingMachine : VRPyBaseT<OSG::VRMillingMachine> {
    static PyMethodDef methods[];

    static PyObject* connect(VRPyMillingMachine* self, PyObject* args);
    static PyObject* disconnect(VRPyMillingMachine* self);
    static PyObject* setSpeed(VRPyMillingMachine* self, PyObject* args);
    static PyObject* setGeometry(VRPyMillingMachine* self, PyObject* args);
    static PyObject* update(VRPyMillingMachine* self);
    static PyObject* setPosition(VRPyMillingMachine* self, PyObject* args);
};

#endif // VRPYMILLINGMACHINE_H_INCLUDED
