#include "VRPyNavPreset.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRNavPreset>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.NavPreset",             /*tp_name*/
    sizeof(VRPyNavPreset),             /*tp_basicsize*/
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
    "NavPreset binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyNavPreset::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMethodDef VRPyNavPreset::methods[] = {
    {"setSpeed", (PyCFunction)VRPyNavPreset::setSpeed, METH_VARARGS, "Set the translation and rotation speed - setSpeed( t, r )" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyNavPreset::setSpeed(VRPyNavPreset* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyNavPreset::setSpeed - Object is invalid"); return NULL; }
    OSG::Vec2f s = parseVec2f(args);
    self->obj->setSpeed(s[0], s[1]);
    Py_RETURN_TRUE;
}
