#ifndef VRPYSELECTOR_H_INCLUDED
#define VRPYSELECTOR_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/selection/VRSelector.h"

struct VRPySelector : VRPyBaseT<OSG::VRSelector> {
    static PyMethodDef methods[];
};

#endif // VRPYSELECTOR_H_INCLUDED
