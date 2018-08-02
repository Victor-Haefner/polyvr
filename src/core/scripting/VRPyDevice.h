#ifndef VRPYDEVICE_H_INCLUDED
#define VRPYDEVICE_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/devices/VRDevice.h"

struct VRPySignal : VRPyBaseT<OSG::VRSignal> {
    static PyMethodDef methods[];
};

struct VRPyDevice : VRPyBaseT<OSG::VRDevice> {
    static PyMethodDef methods[];

    static PyObject* fromSharedPtr(OSG::VRDevicePtr dev);
};

#endif // VRPYDEVICE_H_INCLUDED
