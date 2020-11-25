#ifndef VRPYMACHINING_H_INCLUDED
#define VRPYMACHINING_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRMachiningCode.h"
#include "VRCartesianKinematics.h"

struct VRPyMachiningCode : VRPyBaseT<OSG::VRMachiningCode> {
    static PyMethodDef methods[];
};

struct VRPyCartesianKinematics : VRPyBaseT<OSG::VRCartesianKinematics> {
    static PyMethodDef methods[];
};

#endif VRPYMACHINING_H_INCLUDED
