#ifndef VRPYMULTIGRID_H_INCLUDED
#define VRPYMULTIGRID_H_INCLUDED

#include "VRPyBase.h"
#include "core/objects/geometry/VRMultiGrid.h"

struct VRPyMultiGrid : VRPyBaseT<OSG::VRMultiGrid> {
    static PyMethodDef methods[];
};

#endif // VRPYMULTIGRID_H_INCLUDED
