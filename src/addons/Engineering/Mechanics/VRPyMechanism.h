#ifndef VRPYMECHANISM_H_INCLUDED
#define VRPYMECHANISM_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRMechanism.h"

class VRPyGeometry;

struct VRPyMechanism : VRPyBaseT<OSG::VRMechanism> {
    static PyMethodDef methods[];;
};

#endif // VRPYMECHANISM_H_INCLUDED
