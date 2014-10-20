#include "VRPySocket.h"

PyObject* VRPySocket::err = NULL;

PyMODINIT_FUNC initVRPySocket(PyObject* module) {
    if (PyType_Ready(&VRPySocket::VRPySocketType) < 0)
        return;

    Py_INCREF(&VRPySocket::VRPySocketType);
    PyModule_AddObject(module, "Socket", (PyObject *)&VRPySocket::VRPySocketType);

    VRPySocket::err = PyErr_NewException((char *)"VR.Error", NULL, NULL);
}

void VRPySocket::dealloc(VRPySocket* self) {
    self->ob_type->tp_free((PyObject*)self);
}

PyObject* VRPySocket::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyErr_SetString(err, "VRPySocket::New, not allowed");
    return NULL;
    /*PyObject* name = NULL;
    if (! PyArg_ParseTuple(args, "O", &name)) return NULL;
    string _name = PyString_AsString(name);

    VRPySocket* self = (VRPySocket *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->obj = new OSG::VRSocket(_name);
    }

    return (PyObject *)self;*/
}

PyObject* VRPySocket::fromPtr(OSG::VRSocket* obj) {
    VRPySocket *self = (VRPySocket *)VRPySocketType.tp_alloc(&VRPySocketType, 0);

    if (self != NULL) self->obj = obj;

    return (PyObject *)self;
}

int VRPySocket::init(VRPySocket *self, PyObject *args, PyObject *kwds) {
    return 0;
}


PyTypeObject VRPySocket::VRPySocketType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Socket",             /*tp_name*/
    sizeof(VRPySocket),             /*tp_basicsize*/
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
    "VRSocket binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    methods,             /* tp_methods */
    members,             /* tp_members */
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

PyMemberDef VRPySocket::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPySocket::methods[] = {
    {"getName", (PyCFunction)VRPySocket::getName, METH_NOARGS, "Return socket name." },
    {"send", (PyCFunction)VRPySocket::send, METH_VARARGS, "Send message" },
    {"destroy", (PyCFunction)VRPySocket::destroy, METH_NOARGS, "Destroy socket." },
    {NULL}  /* Sentinel */
};


PyObject* VRPySocket::getName(VRPySocket* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPySocket::getName, Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getName().c_str());
}

PyObject* VRPySocket::destroy(VRPySocket* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPySocket::destroy, Object is invalid"); return NULL; }
    delete self->obj;
    self->obj = 0;
    Py_RETURN_TRUE;
}

PyObject* VRPySocket::send(VRPySocket* self, PyObject* args) {
    PyObject* msg = NULL;
    if (! PyArg_ParseTuple(args, "O", &msg)) return NULL;
    if (msg == NULL) {
        PyErr_SetString(err, "Missing message parameter");
        return NULL;
    }

    string _msg = PyString_AsString(msg);

    if (self->obj == 0) { PyErr_SetString(err, "VRPySocket::send, Object is invalid"); return NULL; }
    self->obj->sendMessage(_msg);
    Py_RETURN_TRUE;
}
