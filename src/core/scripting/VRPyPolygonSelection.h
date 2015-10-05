#ifndef VRPYPOLYGONSELECTION_H_INCLUDED
#define VRPYPOLYGONSELECTION_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/selection/VRPolygonSelection.h"

struct VRPyPolygonSelection : VRPyBaseT<OSG::VRPolygonSelection> {
    static PyMethodDef methods[];
};

#endif // VRPYPOLYGONSELECTION_H_INCLUDED
