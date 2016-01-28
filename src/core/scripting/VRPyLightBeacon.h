#ifndef VRPYLIGHTBEACON_H_INCLUDED
#define VRPYLIGHTBEACON_H_INCLUDED

#include "VRPyObject.h"
#include "core/objects/VRLightBeacon.h"

struct VRPyLightBeacon : VRPyBaseT<OSG::VRLightBeacon> {
    static PyMethodDef methods[];
};

#endif // VRPYLIGHTBEACON_H_INCLUDED
