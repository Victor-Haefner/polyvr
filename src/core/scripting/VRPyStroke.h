#ifndef VRPyStroke_H_INCLUDED
#define VRPyStroke_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/geometry/VRStroke.h"

struct VRPyStroke : VRPyBaseT<OSG::VRStroke> {
    static PyMethodDef methods[];
};

#endif // VRPyStroke_H_INCLUDED
