#ifndef VRPYCONSTRUCTIONKIT_H_INCLUDED
#define VRPYCONSTRUCTIONKIT_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRConstructionKit.h"

struct VRPyConstructionKit : VRPyBaseT<OSG::VRConstructionKit> {
    static PyMethodDef methods[];

    static PyObject* getSnappingEngine(VRPyConstructionKit* self);
    static PyObject* getSelector(VRPyConstructionKit* self);
};

#endif // VRPYCONSTRUCTIONKIT_H_INCLUDED
