#ifndef VRPYMILLINGCUTTINGTOOLPROFILE_H
#define VRPYMILLINGCUTTINGTOOLPROFILE_H

#include "core/scripting/VRPyBase.h"
#include "core/scripting/VRPyObject.h"
#include "VRMillingCuttingToolProfile.h"

struct VRPyMillingCuttingToolProfile : VRPyBaseT<OSG::VRMillingCuttingToolProfile> {
    static PyMethodDef methods[];
    static PyObject* addPointProfile(VRPyMillingCuttingToolProfile* self, PyObject* args);
};

#endif // VRPYMILLINGCUTTINGTOOLPROFILE_H
