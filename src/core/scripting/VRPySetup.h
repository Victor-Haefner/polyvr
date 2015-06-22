#ifndef VRPYSETUP_H_INCLUDED
#define VRPYSETUP_H_INCLUDED

#include "VRPyBase.h"
#include "core/setup/VRSetup.h"

struct VRPySetup : VRPyBaseT<OSG::VRSetup> {
    static PyMethodDef methods[];

    static PyObject* toggleStereo(VRPySetup* self, PyObject* args);
};

#endif // VRPYSETUP_H_INCLUDED
