#ifndef VRPYMACHINING_H_INCLUDED
#define VRPYMACHINING_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRMachiningSimulation.h"
#include "VRMachiningCode.h"
#include "VRCartesianKinematics.h"

struct VRPyMachiningSimulation : VRPyBaseT<OSG::VRMachiningSimulation> {
    static PyMethodDef methods[];
};

struct VRPyMachiningCode : VRPyBaseT<OSG::VRMachiningCode> {
    static PyMethodDef methods[];
};

struct VRPyMachiningKinematics : VRPyBaseT<OSG::VRMachiningKinematics> {
    static PyMethodDef methods[];
};

struct VRPyCartesianKinematics : VRPyBaseT<OSG::VRCartesianKinematics> {
    static PyMethodDef methods[];
};

#endif //VRPYMACHINING_H_INCLUDED
