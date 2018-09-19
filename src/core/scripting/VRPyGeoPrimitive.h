#ifndef VRPYGEOPRIMITIVE_H_INCLUDED
#define VRPYGEOPRIMITIVE_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/geometry/VRHandle.h"
#include "core/tools/VRGeoPrimitive.h"

struct VRPyHandle : VRPyBaseT<OSG::VRHandle> {
    static PyMethodDef methods[];
};

struct VRPyGeoPrimitive : VRPyBaseT<OSG::VRGeoPrimitive> {
    static PyMethodDef methods[];
};

#endif // VRPYGEOPRIMITIVE_H_INCLUDED
