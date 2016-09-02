#include "VRPyGraph.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

newPyType( graph_base , Graph , 0 );

PyMethodDef VRPyGraph::methods[] = {
    //{"activate", (PyCFunction)VRPyGraph::activate, METH_NOARGS, "Switch to Graph - activate()" },
    //{"setFov", (PyCFunction)VRPyGraph::setFov, METH_VARARGS, "Set the Graph's field of view" },
    //{"focus", (PyCFunction)VRPyGraph::focus, METH_VARARGS, "Set the Graph's position to see the whole scene under the object - focus(object)" },
    {NULL}  /* Sentinel */
};

/*PyObject* VRPyGraph::activate(VRPyGraph* self) {
    if (!self->valid()) return NULL;
    self->objPtr->activate();
    Py_RETURN_TRUE;
}

PyObject* VRPyGraph::setFov(VRPyGraph* self, PyObject* args) {
    if (!self->valid()) return NULL;
    float fov = parseFloat(args);
    self->objPtr->setFov(fov);
    Py_RETURN_TRUE;
}

PyObject* VRPyGraph::focus(VRPyGraph* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* o;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->focus(o->objPtr);
    Py_RETURN_TRUE;
}*/
