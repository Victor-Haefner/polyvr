#ifndef VRPYHAPTIC_H_INCLUDED
#define VRPYHAPTIC_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/devices/VRHaptic.h"
#include "core/objects/geometry/VRPhysics.h"


struct VRPyHaptic : VRPyBaseT<OSG::VRHaptic> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* setForce(VRPyHaptic* self, PyObject* args);
    static PyObject* setSimulationScales(VRPyHaptic* self, PyObject* args);
    static PyObject* updateHapticToObject(VRPyHaptic* self, PyObject* args);
};

#endif // VRPYHAPTIC_H_INCLUDED
