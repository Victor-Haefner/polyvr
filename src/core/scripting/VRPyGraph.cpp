#include "VRPyGraph.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

newPyType( graph_base , Graph , 0 );

PyMethodDef VRPyGraph::methods[] = {
    {"getEdges", (PyCFunction)VRPyGraph::getEdges, METH_NOARGS, "Return graph edges - getEdges()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyGraph::getEdges(VRPyGraph* self) {
    if (!self->valid()) return NULL;
    PyObject* res = PyList_New(0);
    auto edges = self->objPtr->getEdges();
    for (auto& n : edges) {
        for (auto& e : n) {
            PyObject* epy = PyTuple_New(2);
            PyTuple_SetItem(epy, 0, PyInt_FromLong(e.from));
            PyTuple_SetItem(epy, 1, PyInt_FromLong(e.to));
            PyList_Append(res, epy);
        }
    }
    return res;
}
