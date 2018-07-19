#ifndef VRPYCAVEKEEPER_H_INCLUDED
#define VRPYCAVEKEEPER_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "CaveKeeper.h"

struct VRPyCaveKeeper : VRPyBaseT<OSG::CaveKeeper> {
    static PyMethodDef methods[];
};

#endif // VRPYCAVEKEEPER_H_INCLUDED
