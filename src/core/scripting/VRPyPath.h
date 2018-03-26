#ifndef VRPyPath_H_INCLUDED
#define VRPyPath_H_INCLUDED

#include "core/math/path.h"
#include "core/scripting/VRPyBase.h"

struct VRPyPath : VRPyBaseT<OSG::Path> {
    static PyMethodDef methods[];
};

#endif // VRPyPath_H_INCLUDED
