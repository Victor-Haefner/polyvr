#ifndef VRPYNAMED_H_INCLUDED
#define VRPYNAMED_H_INCLUDED

#include "VRPyBase.h"
#include "core/utils/VRName.h"

struct VRPyName : VRPyBaseT<OSG::VRName> {
    static PyMethodDef methods[];
};

#endif // VRPYNAMED_H_INCLUDED
