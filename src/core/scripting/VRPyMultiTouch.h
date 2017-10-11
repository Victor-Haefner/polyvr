#ifndef VRPYMULTITOUCH_H_INCLUDED
#define VRPYMULTITOUCH_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/devices/VRMultiTouch.h"

struct VRPyMultiTouch : VRPyBaseT<OSG::VRMultiTouch> {
    static PyMethodDef methods[];
};

#endif // VRPYMULTITOUCH_H_INCLUDED
