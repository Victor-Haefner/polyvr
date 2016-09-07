#include "VRPyGraphLayout.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGraph.h"

using namespace OSG;

simpleVRPyType(GraphLayout, New_ptr);

PyMethodDef VRPyGraphLayout::methods[] = {
    {"setGraph", (PyCFunction)VRPyGraphLayout::setGraph, METH_VARARGS, "Set graph - setGraph(graph)" },
    {"setAlgorithm", (PyCFunction)VRPyGraphLayout::setAlgorithm, METH_VARARGS, "Set pipeline algorithms - setAlgorithm( str algorithm, int position )\n\talgorithm: 'SPRINGS', 'OCCUPANCYMAP'" },
    {"setParameters", (PyCFunction)VRPyGraphLayout::setParameters, METH_VARARGS, "Set parameters - setParameters( flt radius )" },
    {"fixNode", (PyCFunction)VRPyGraphLayout::fixNode, METH_VARARGS, "Fix a node, making it static - fixNode( int n )" },
    {"compute", (PyCFunction)VRPyGraphLayout::compute, METH_VARARGS, "Compute N steps - compute( int steps, float threshold )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyGraphLayout::setGraph(VRPyGraphLayout* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGraph* g = 0;
    if (!PyArg_ParseTuple(args, "O", &g)) return NULL;
    self->objPtr->setGraph( g->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyGraphLayout::setAlgorithm(VRPyGraphLayout* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* a = 0;
    int p = 0;
    if (!PyArg_ParseTuple(args, "s|i", &a, &p)) return NULL;
    self->objPtr->setAlgorithm( a, p );
    Py_RETURN_TRUE;
}

PyObject* VRPyGraphLayout::setParameters(VRPyGraphLayout* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float r = 1;
    if (!PyArg_ParseTuple(args, "f", &r)) return NULL;
    self->objPtr->setRadius( r );
    Py_RETURN_TRUE;
}

PyObject* VRPyGraphLayout::compute(VRPyGraphLayout* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int N;
    float t = 0.1;
    if (!PyArg_ParseTuple(args, "i|f", &N, &t)) return NULL;
    self->objPtr->compute( N, t );
    Py_RETURN_TRUE;
}

PyObject* VRPyGraphLayout::fixNode(VRPyGraphLayout* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->fixNode( parseInt(args) );
    Py_RETURN_TRUE;
}
