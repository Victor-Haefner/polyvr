#include "VRPySetup.h"
#include "VRPyBaseT.h"

#include "core/setup/windows/VRView.h"

template<> PyTypeObject VRPyBaseT<OSG::VRSetup>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Setup",             /*tp_name*/
    sizeof(VRPySetup),             /*tp_basicsize*/
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
    "Setup binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPySetup::methods,             /* tp_methods */
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

PyMethodDef VRPySetup::methods[] = {
    {"toggleStereo", (PyCFunction)VRPySetup::toggleStereo, METH_VARARGS, "Toggle stereo of view i - toggleStereo(int i)" },
    {NULL}  /* Sentinel */
};


PyObject* VRPySetup::toggleStereo(VRPySetup* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPySetup::activate - Object is invalid"); return NULL; }
    int i = parseInt(args);
    auto v = self->obj->getView(i);
    if (v) v->setStereo(!v->isStereo());
    Py_RETURN_TRUE;
}
