#include "VRPyMillingCuttingToolProfile.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyGeometry.h"

template<> PyTypeObject VRPyBaseT<OSG::VRMillingCuttingToolProfile>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.MillingCuttingToolProfile",             /*tp_name*/
    sizeof(VRPyMillingCuttingToolProfile),             /*tp_basicsize*/
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
    "MillingCuttingToolProfile binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMillingCuttingToolProfile::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New,                 /* tp_new */
};

PyMethodDef VRPyMillingCuttingToolProfile::methods[] = {
    {"addPointProfile", (PyCFunction)VRPyMillingCuttingToolProfile::addPointProfile, METH_VARARGS,
    "addPointProfile() - adds a point to the cutting tool profile."},
    {NULL}  /* Sentinel */
};

//Add of Marie
PyObject* VRPyMillingCuttingToolProfile::addPointProfile(VRPyMillingCuttingToolProfile* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject* newPoint;
    if (!PyArg_ParseTuple(args, "O", &newPoint)) return NULL;
    auto p = parseVec2fList(newPoint);
    self->obj->addPointProfile(p);
    Py_RETURN_TRUE;
}
