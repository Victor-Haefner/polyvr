#include "VRPySnappingEngine.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRSnappingEngine>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Haptic",             /*tp_name*/
    sizeof(VRPySnappingEngine),             /*tp_basicsize*/
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
    "Haptic binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPySnappingEngine::methods,             /* tp_methods */
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

PyMethodDef VRPySnappingEngine::methods[] = {
    {"setPreset", (PyCFunction)VRPySnappingEngine::setPreset, METH_VARARGS, "Initiate the engine with a preset - setPreset('simple alignment')" },
    {NULL}  /* Sentinel */
};


PyObject* VRPySnappingEngine::setPreset(VRPySnappingEngine* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPySnappingEngine::setPreset - Object is invalid"); return NULL; }
    string ps = parseString(args);
    if (ps == "simple alignment") self->obj->setPreset(OSG::VRSnappingEngine::SIMPLE_ALIGNMENT);
    Py_RETURN_TRUE;
}

