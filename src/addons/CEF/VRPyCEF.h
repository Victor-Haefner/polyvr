#ifndef VRPyCEF_H_INCLUDED
#define VRPyCEF_H_INCLUDED

#include "CEF.h"
#include "core/scripting/VRPyBase.h"

struct VRPyCEF : VRPyBaseT<OSG::CEF> {
    static PyMethodDef methods[];
};

#endif // VRPyCEF_H_INCLUDED
