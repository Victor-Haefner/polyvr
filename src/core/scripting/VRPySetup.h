#ifndef VRPYSETUP_H_INCLUDED
#define VRPYSETUP_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRWebXR.h"
#include "core/setup/windows/VRView.h"
#include "core/setup/windows/VRWindow.h"

struct VRPyViewManager : VRPyBaseT<OSG::VRViewManager> {
    static PyMethodDef methods[];
};

struct VRPyWindowManager : VRPyBaseT<OSG::VRWindowManager> {
    static PyMethodDef methods[];
};

struct VRPySetup : VRPyBaseT<OSG::VRSetup> {
    static PyMethodDef methods[];
};

struct VRPyView : VRPyBaseT<OSG::VRView> {
    static PyMethodDef methods[];
};

struct VRPyWindow : VRPyBaseT<OSG::VRWindow> {
    static PyMethodDef methods[];
};

struct VRPyWebXR : VRPyBaseT<OSG::VRWebXR> {
    static PyMethodDef methods[];
};

#endif // VRPYSETUP_H_INCLUDED
