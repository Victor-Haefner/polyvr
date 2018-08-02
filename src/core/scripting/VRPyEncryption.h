#ifndef VRPYENCRYPTION_H_INCLUDED
#define VRPYENCRYPTION_H_INCLUDED

#include "VRPyBase.h"
#include "core/utils/VREncryption.h"

struct VRPyEncryption : VRPyBaseT<OSG::VREncryption> {
    static PyMethodDef methods[];
};

#endif // VRPYENCRYPTION_H_INCLUDED
