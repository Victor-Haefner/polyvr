#ifndef VRPYDEVICE_H_INCLUDED
#define VRPYDEVICE_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/devices/VRDevice.h"
#ifndef WITHOUT_OPENVR
#include "core/setup/windows/VRHeadMountedDisplay.h"
#endif

struct VRPySignal : VRPyBaseT<OSG::VRSignal> {
    static PyMethodDef methods[];
};

struct VRPyDevice : VRPyBaseT<OSG::VRDevice> {
    static PyMethodDef methods[];

    static PyObject* fromSharedPtr(OSG::VRDevicePtr dev);
};

#ifndef WITHOUT_OPENVR
struct VRPyHeadMountedDisplay : VRPyBaseT<OSG::VRHeadMountedDisplay> {
    static PyMethodDef methods[];
};
#endif

#endif // VRPYDEVICE_H_INCLUDED
