#ifndef VRPYANALYTICGEOMETRY_H_INCLUDED
#define VRPYANALYTICGEOMETRY_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "core/tools/VRAnalyticGeometry.h"

struct VRPyAnalyticGeometry : VRPyBaseT<OSG::VRAnalyticGeometry> {
    static PyMethodDef methods[];

    static PyObject* setVector(VRPyAnalyticGeometry* self, PyObject* args);
    static PyObject* setLabelSize(VRPyAnalyticGeometry* self, PyObject* args);
    static PyObject* clear(VRPyAnalyticGeometry* self);
};

#endif // VRPYANALYTICGEOMETRY_H_INCLUDED
