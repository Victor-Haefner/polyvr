#ifndef VRPYPOLYGON_H_INCLUDED
#define VRPYPOLYGON_H_INCLUDED

#include "core/math/polygon.h"
#include "core/scripting/VRPyBase.h"

struct VRPyPolygon : VRPyBaseT<OSG::Polygon> {
    static PyMethodDef methods[];

    static PyObject* addPoint(VRPyPolygon* self, PyObject *args);
    static PyObject* getPoint(VRPyPolygon* self, PyObject *args);
    static PyObject* getPoints(VRPyPolygon* self);
    static PyObject* close(VRPyPolygon* self);
    static PyObject* size(VRPyPolygon* self);
    static PyObject* set(VRPyPolygon* self, PyObject *args);
    static PyObject* clear(VRPyPolygon* self);
    static PyObject* getConvexHull(VRPyPolygon* self);
};

#endif // VRPYPOLYGON_H_INCLUDED
