#ifndef VRPYMECHANISM_H_INCLUDED
#define VRPYMECHANISM_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRMechanism.h"
#include "VRGearSegmentation.h"
#include "VRAxleSegmentation.h"

struct VRPyMechanism : VRPyBaseT<OSG::VRMechanism> {
    static PyMethodDef methods[];
};

#ifndef WITHOUT_EIGEN
struct VRPyGearSegmentation : VRPyBaseT<OSG::VRGearSegmentation> {
    static PyMethodDef methods[];
};

struct VRPyAxleSegmentation : VRPyBaseT<OSG::VRAxleSegmentation> {
    static PyMethodDef methods[];
};
#endif

#endif // VRPYMECHANISM_H_INCLUDED
