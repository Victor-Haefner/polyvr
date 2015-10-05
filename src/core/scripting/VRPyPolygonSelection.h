#ifndef VRPYPOLYGONSELECTION_H_INCLUDED
#define VRPYPOLYGONSELECTION_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/selection/VRPolygonSelection.h"

struct VRPyPolygonSelection : VRPyBaseT<OSG::VRPolygonSelection> {
    static PyMethodDef methods[];

    static PyObject* clear(VRPyPolygonSelection* self);
    static PyObject* setOrigin(VRPyPolygonSelection* self, PyObject* args);
    static PyObject* addEdge(VRPyPolygonSelection* self, PyObject* args);
    static PyObject* close(VRPyPolygonSelection* self);
};

#endif // VRPYPOLYGONSELECTION_H_INCLUDED
