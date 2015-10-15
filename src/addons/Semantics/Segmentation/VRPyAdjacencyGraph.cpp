#include "VRPyAdjacencyGraph.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRAdjacencyGraph>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.AdjacencyGraph",             /*tp_name*/
    sizeof(VRPyAdjacencyGraph),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRAdjacencyGraph binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyAdjacencyGraph::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_ptr,                 /* tp_new */
};

PyMethodDef VRPyAdjacencyGraph::methods[] = {
    {"setGeometry", (PyCFunction)VRPyAdjacencyGraph::setGeometry, METH_VARARGS, "Set the geometry to set up the graph - setGeometry( geo )" },
    {"computeNeighbors", (PyCFunction)VRPyAdjacencyGraph::computeNeighbors, METH_NOARGS, "Compute the vertex neighbors list - computeNeighbors()" },
    {"computeTriLoockup", (PyCFunction)VRPyAdjacencyGraph::computeTriLoockup, METH_NOARGS, "Compute the triangle loockup table - computeTriLoockup()" },
    {"computeCurvatures", (PyCFunction)VRPyAdjacencyGraph::computeCurvatures, METH_VARARGS, "Compute the vertex curvatures list - computeCurvatures( int range )" },
    {"getNeighbors", (PyCFunction)VRPyAdjacencyGraph::getNeighbors, METH_VARARGS, "Return the neighbor indices to index i - [int] getNeighbors(int i, int range)" },
    {"getCurvature", (PyCFunction)VRPyAdjacencyGraph::getCurvature, METH_VARARGS, "Return the mesh curvature at index i - float getCurvature(int i)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAdjacencyGraph::setGeometry(VRPyAdjacencyGraph* self, PyObject* args) {
    VRPyGeometry* geo = NULL;
    if (! PyArg_ParseTuple(args, "O:setGeometry", &geo)) return NULL;
    self->objPtr->setGeometry(geo->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPyAdjacencyGraph::computeNeighbors(VRPyAdjacencyGraph* self) {
    self->objPtr->compNeighbors();
    Py_RETURN_TRUE;
}

PyObject* VRPyAdjacencyGraph::computeTriLoockup(VRPyAdjacencyGraph* self) {
    self->objPtr->compTriLoockup();
    Py_RETURN_TRUE;
}

PyObject* VRPyAdjacencyGraph::computeCurvatures(VRPyAdjacencyGraph* self, PyObject* args) {
    int i = 1;
    if (! PyArg_ParseTuple(args, "|i:computeCurvatures", &i)) return NULL;
    self->objPtr->compCurvatures(i);
    Py_RETURN_TRUE;
}

PyObject* VRPyAdjacencyGraph::getNeighbors(VRPyAdjacencyGraph* self, PyObject* args) {
    int i;
    int r = 1;
    if (! PyArg_ParseTuple(args, "i|i:getNeighbors", &i, &r)) return NULL;
    auto v = self->objPtr->getNeighbors(i,r);
    PyObject* res = PyList_New(v.size());
    for (uint i=0; i<v.size(); i++) PyList_SetItem(res, i, PyInt_FromLong(v[i]));
    return res;
}

PyObject* VRPyAdjacencyGraph::getCurvature(VRPyAdjacencyGraph* self, PyObject* args) {
    int i;
    if (! PyArg_ParseTuple(args, "i:getCurvature", &i)) return NULL;
    return PyFloat_FromDouble( self->objPtr->getCurvature(i) );
}
