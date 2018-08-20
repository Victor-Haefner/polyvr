#ifndef VRPYPATHTOOL_H_INCLUDED
#define VRPYPATHTOOL_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRPathtool.h"

struct VRPyPathtool : VRPyBaseT<OSG::VRPathtool> {
    static PyMethodDef methods[];
};

#endif // VRPYPATHTOOL_H_INCLUDED
