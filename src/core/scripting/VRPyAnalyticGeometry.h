#ifndef VRPYANALYTICGEOMETRY_H_INCLUDED
#define VRPYANALYTICGEOMETRY_H_INCLUDED

#include "core/tools/VRAnalyticGeometry.h"
#include "VRPyBase.h"

struct VRPyAnalyticGeometry : VRPyBaseT<OSG::VRAnalyticGeometry> {
    static PyMethodDef methods[];
};

#endif // VRPYANALYTICGEOMETRY_H_INCLUDED
