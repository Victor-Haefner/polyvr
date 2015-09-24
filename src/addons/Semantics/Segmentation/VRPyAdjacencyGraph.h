#ifndef VRPYADJACENCYGRAPH_H_INCLUDED
#define VRPYADJACENCYGRAPH_H_INCLUDED

#include "VRAdjacencyGraph.h"
#include "core/scripting/VRPyBase.h"

struct VRPyAdjacencyGraph : VRPyBaseT<OSG::VRAdjacencyGraph> {
    static PyMethodDef methods[];

    static PyObject* setGeometry(VRPyAdjacencyGraph* self, PyObject* args);
    static PyObject* compute(VRPyAdjacencyGraph* self, PyObject* args);
    static PyObject* getNeighbors(VRPyAdjacencyGraph* self, PyObject* args);
};

#endif // VRPYADJACENCYGRAPH_H_INCLUDED
