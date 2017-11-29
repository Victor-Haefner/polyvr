#ifndef VRPYMOBILE_H_INCLUDED
#define VRPYMOBILE_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/devices/VRServer.h"

struct VRPyServer : VRPyBaseT<OSG::VRServer> {
    static PyMethodDef methods[];
};

#endif // VRPYMOBILE_H_INCLUDED
