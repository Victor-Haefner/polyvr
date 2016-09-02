#ifndef VRPYGRAPH_H_INCLUDED
#define VRPYGRAPH_H_INCLUDED

#include "VRPyBase.h"
#include "core/math/graph.h"

struct VRPyGraph : VRPyBaseT<OSG::graph_base> {
    static PyMethodDef methods[];
    //static PyObject* activate(VRPyGraph* self);
    //static PyObject* setFov(VRPyGraph* self, PyObject* args);
};

#endif // VRPYGRAPH_H_INCLUDED
