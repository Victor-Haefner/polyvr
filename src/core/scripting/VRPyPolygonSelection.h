#ifndef VRPYPOLYGONSELECTION_H_INCLUDED
#define VRPYPOLYGONSELECTION_H_INCLUDED

#include "VRPyObject.h"
#include "core/tools/selection/VRPolygonSelection.h"

struct VRPyPolygonSelection : VRPyBaseT<OSG::VRPolygonSelection> {
    static PyMethodDef methods[];

    static PyObject* clear(VRPyPolygonSelection* self);
    static PyObject* isClosed(VRPyPolygonSelection* self);
    static PyObject* setOrigin(VRPyPolygonSelection* self, PyObject* args);
    static PyObject* addEdge(VRPyPolygonSelection* self, PyObject* args);
    static PyObject* getShape(VRPyPolygonSelection* self);
    static PyObject* close(VRPyPolygonSelection* self, PyObject* args);
};

#endif // VRPYPOLYGONSELECTION_H_INCLUDED
