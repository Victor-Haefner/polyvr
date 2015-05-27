#ifndef VRPYHAPTIC_H_INCLUDED
#define VRPYHAPTIC_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/devices/VRHaptic.h"
#include "core/objects/geometry/VRPhysics.h"


struct VRPyHaptic : VRPyBaseT<OSG::VRHaptic> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* setForce(VRPyHaptic* self, PyObject* args);
    static PyObject* getForce(VRPyHaptic* self);
    static PyObject* setSimulationScales(VRPyHaptic* self, PyObject* args);
    static PyObject* attachTransform(VRPyHaptic* self, PyObject* args);
    static PyObject* setBase(VRPyHaptic* self, PyObject* args);
    static PyObject* detachTransform(VRPyHaptic* self);
    static PyObject* getButtonStates(VRPyHaptic* self);
};

#endif // VRPYHAPTIC_H_INCLUDED
