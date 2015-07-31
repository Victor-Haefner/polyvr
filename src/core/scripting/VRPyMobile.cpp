#include "VRPyMobile.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRMobile>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Mobile",             /*tp_name*/
    sizeof(VRPyMobile),             /*tp_basicsize*/
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
    "Mobile binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMobile::methods,             /* tp_methods */
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

PyMethodDef VRPyMobile::methods[] = {
    {"answer", (PyCFunction)VRPyMobile::answer, METH_VARARGS, "Answer web socket - answer(int id, str message)\n use the device key as id, id = dev.getKey()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMobile::answer(VRPyMobile* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMobile::answer - Object is invalid"); return NULL; }
    int id; PyObject* msg;
    if (! PyArg_ParseTuple(args, "iO", &id, &msg)) return NULL;
    self->obj->answerWebSocket(id, PyString_AsString(msg));
    Py_RETURN_TRUE;
}




