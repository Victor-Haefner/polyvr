#ifndef VRPYSCENEGRAPHINTERFACE_H_INCLUDED
#define VRPYSCENEGRAPHINTERFACE_H_INCLUDED

#include "core/scene/interfaces/VRScenegraphInterface.h"
#include "VRPyBase.h"

struct VRPyScenegraphInterface : VRPyBaseT<OSG::VRScenegraphInterface> {
    static PyMethodDef methods[];
};

#endif // VRPYSCENEGRAPHINTERFACE_H_INCLUDED
