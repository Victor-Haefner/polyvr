#include "VRPyMillingWorkPiece.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"


template<> PyTypeObject VRPyBaseT<OSG::VRMillingWorkPiece>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.MillingWorkPiece",             /*tp_name*/
    sizeof(VRPyMillingWorkPiece),             /*tp_basicsize*/
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
    "MillingWorkPiece binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMillingWorkPiece::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects_ptr,                 /* tp_new */
};

PyMethodDef VRPyMillingWorkPiece::methods[] = {
    {"reset", (PyCFunction)VRPyMillingWorkPiece::reset, METH_VARARGS, "Set piece volume parameter and reset - reset( [int] gridSize, float blockSize )" },
    {"setCuttingTool", (PyCFunction)VRPyMillingWorkPiece::setCuttingTool, METH_VARARGS, "Set cutting tool geometry - setCuttingTool( geo )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMillingWorkPiece::reset(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* vec; float s;
    if (! PyArg_ParseTuple(args, "Of", &vec, &s)) return NULL;
    auto v = parseVec3iList(vec);
    self->objPtr->reset(v,s);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::setCuttingTool(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* geo;
    if (! PyArg_ParseTuple(args, "O", &geo)) return NULL;
    self->objPtr->setCuttingTool( geo->objPtr );
    Py_RETURN_TRUE;
}



