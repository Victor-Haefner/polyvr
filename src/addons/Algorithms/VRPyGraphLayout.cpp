#include "VRPyGraphLayout.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGraph.h"

using namespace OSG;

simpleVRPyType(GraphLayout, New_ptr);

PyMethodDef VRPyGraphLayout::methods[] = {
    {"setGraph", (PyCFunction)VRPyGraphLayout::setGraph, METH_VARARGS, "Set graph - setGraph(graph)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyGraphLayout::setGraph(VRPyGraphLayout* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyGraph* g = 0;
    if (!PyArg_ParseTuple(args, "O", &g)) return NULL;
    self->objPtr->setGraph( g->objPtr );
    Py_RETURN_TRUE;
}
