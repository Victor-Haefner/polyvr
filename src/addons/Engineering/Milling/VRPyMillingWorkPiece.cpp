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
    {"init", (PyCFunction)VRPyMillingWorkPiece::init, METH_VARARGS,
        "Set piece volume parameter and init - init( [int] blocksPerDimension, "
        " float sizeOfSmallestBlock ). sizeOfSmallestBlock is the edge length of the smallest cube"
        " while the blocksPerDimension will determine the size of the Workpiece."
        " For example ([1000, 1000, 1000], 0.01) will give a cube of the length x,y,z = 10"},
    {"reset", (PyCFunction)VRPyMillingWorkPiece::reset, METH_NOARGS,
        "resets the whole milling workpiece"},
    {"setCuttingTool", (PyCFunction)VRPyMillingWorkPiece::setCuttingTool, METH_VARARGS,
        "Set cutting tool geometry - setCuttingTool(geo)" },
    {"setRefreshWait", (PyCFunction)VRPyMillingWorkPiece::setRefreshWait, METH_VARARGS,
    "setRefreshWait(int updatesToWaitFor) sets the milling workpiece to wait at least "
    "updatesToWaitFor updates on the worktools positions to update the geometry of the "
    "workpiece" },
    {"setLevelsPerGeometry", (PyCFunction)VRPyMillingWorkPiece::setLevelsPerGeometry, METH_VARARGS,
    "setLevelsPerGeometry(int levels) - levels means each gemeotry element of the workpiece "
    "will contain 2^levels little cubes. This parameter exists to tune the performance of the "
    "workpiece simulation" },
    {"updateGeometry", (PyCFunction)VRPyMillingWorkPiece::updateGeometry, METH_VARARGS,
    "updateGeometry() - updates the geometry of the workpiece immediately." },
    {"addPointProfile", (PyCFunction)VRPyMillingWorkPiece::addPointProfile, METH_VARARGS,
    "addPointProfile() - adds a point to the worktool profile."},
    {NULL}  /* Sentinel */
};

PyObject* VRPyMillingWorkPiece::reset(VRPyMillingWorkPiece* self) {
    if (!self->valid()) return NULL;
    self->objPtr->reset();
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::init(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* vec; float s;
    if (! PyArg_ParseTuple(args, "Of", &vec, &s)) return NULL;
    auto v = parseVec3iList(vec);
    self->objPtr->init(v,s);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::setCuttingTool(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyTransform* geo;
    if (! PyArg_ParseTuple(args, "O", &geo)) return NULL;
    self->objPtr->setCuttingTool( geo->objPtr );
    Py_RETURN_TRUE;
}

//Add of Marie
PyObject* VRPyMillingWorkPiece::addPointProfile(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* newPoint;
    if (!PyArg_ParseTuple(args, "O", &newPoint)) return NULL;
    auto p = parseVec2fList(newPoint);
    self->objPtr->addPointProfile(p);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::setRefreshWait(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int refreshWait = 0;
    if (! PyArg_ParseTuple(args, "i", &refreshWait)) return NULL;
    self->objPtr->setRefreshWait(refreshWait);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::setLevelsPerGeometry(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    int levels = 0;
    if (! PyArg_ParseTuple(args, "i", &levels)) return NULL;
    self->objPtr->setLevelsPerGeometry(levels);
    Py_RETURN_TRUE;
}

PyObject* VRPyMillingWorkPiece::updateGeometry(VRPyMillingWorkPiece* self, PyObject* args) {
    if (!self->valid()) return NULL;
    self->objPtr->updateGeometry();
    Py_RETURN_TRUE;
}

