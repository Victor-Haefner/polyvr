#ifndef VRPYPRODUCTION_H_INCLUDED
#define VRPYPRODUCTION_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRProduction.h"

struct VRPyProduction : VRPyBaseT<OSG::VRProduction> {
    static PyMethodDef methods[];

    static PyObject* test(VRPyProduction* self);
};

#endif // VRPYPRODUCTION_H_INCLUDED
