#ifndef VRPYNAVPRESET_H_INCLUDED
#define VRPYNAVPRESET_H_INCLUDED

#include "VRPyObject.h"
#include "core/navigation/VRNavigator.h"

struct VRPyNavPreset : VRPyBaseT<OSG::VRNavPreset> {
    static PyMethodDef methods[];
};

#endif // VRPYNAVPRESET_H_INCLUDED
