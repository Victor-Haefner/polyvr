#include "VRPyGraph.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

newPyType( graph_base , Graph , 0 );

PyMethodDef VRPyGraph::methods[] = {
    {"getEdges", (PyCFunction)VRPyGraph::getEdges, METH_NOARGS, "Return graph edges - getEdges()" },
    {"getInEdges", (PyCFunction)VRPyGraph::getInEdges, METH_VARARGS, "Return graph edges going in node n - getInEdges( int n )" },
    {"getOutEdges", (PyCFunction)VRPyGraph::getOutEdges, METH_VARARGS, "Return graph edges comming out node n - getOutEdges( int n )" },
    {NULL}  /* Sentinel */
};

PyObject* convEdge(graph_base::edge& e) {
    PyObject* epy = PyTuple_New(2);
    PyTuple_SetItem(epy, 0, PyInt_FromLong(e.from));
    PyTuple_SetItem(epy, 1, PyInt_FromLong(e.to));
    return epy;
}

PyObject* VRPyGraph::getEdges(VRPyGraph* self) {
    if (!self->valid()) return NULL;
    PyObject* res = PyList_New(0);
    auto edges = self->objPtr->getEdges();
    for (auto& n : edges) {
        for (auto& e : n) PyList_Append(res, convEdge(e));
    }
    return res;
}

PyObject* VRPyGraph::getInEdges(VRPyGraph* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i = -1;
    if (!PyArg_ParseTuple(args, "i", &i)) return NULL;

    PyObject* res = PyList_New(0);
    auto edges = self->objPtr->getEdges();
    for (auto& n : edges) {
        for (auto& e : n) {
            if (e.to != i) continue;
            PyList_Append(res, convEdge(e));
        }
    }
    return res;
}

PyObject* VRPyGraph::getOutEdges(VRPyGraph* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int i = -1;
    if (!PyArg_ParseTuple(args, "i", &i)) return NULL;

    PyObject* res = PyList_New(0);
    auto edges = self->objPtr->getEdges();
    if (i<0 || i >= int(edges.size())) return res;
    auto& n = edges[i];
    for (auto& e : n) PyList_Append(res, convEdge(e));
    return res;
}
