#ifndef VRPYDRAWING_H_INCLUDED
#define VRPYDRAWING_H_INCLUDED

#include "core/scripting/VRPyBase.h"
#include "VRTechnicalDrawing.h"

struct VRPyTechnicalDrawing : VRPyBaseT<OSG::VRTechnicalDrawing> {
    static PyMethodDef methods[];
};

#endif // VRPYDRAWING_H_INCLUDED
