#ifndef VRPYGRAPH_H_INCLUDED
#define VRPYGRAPH_H_INCLUDED

#include "VRPyBase.h"
#include "core/math/graph.h"

struct VRPyGraph : VRPyBaseT<OSG::Graph> {
    static PyMethodDef methods[];
};

#endif // VRPYGRAPH_H_INCLUDED
