#ifndef VRPYWEBCAM_H_INCLUDED
#define VRPYWEBCAM_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRWebCam.h"

struct VRPyWebCam : VRPyBaseT<OSG::VRWebCam> {
    static PyMethodDef methods[];
};

#endif // VRPYWEBCAM_H_INCLUDED
