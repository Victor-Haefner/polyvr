#ifndef VRPYMEASURE_H_INCLUDED
#define VRPYMEASURE_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/VRMesure.h"

struct VRPyMeasure : VRPyBaseT<OSG::VRMeasure> {
    static PyMethodDef methods[];
};

#endif // VRPYMEASURE_H_INCLUDED
