#ifndef VRPYLOD_H
#define VRPYLOD_H

#include "VRPyBase.h"
#include "core/objects/VRLod.h"

struct VRPyLod : VRPyBaseT<OSG::VRLod> {
    static PyMethodDef methods[];
};

#endif // VRPYLOD_H
