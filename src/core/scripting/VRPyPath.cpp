#include "VRPyPath.h"

PyObject* VRPyPath::err = NULL;

PyMODINIT_FUNC initVRPyPath(PyObject* module) {
    if (PyType_Ready(&VRPyPath::type) < 0)
        return;

    Py_INCREF(&VRPyPath::type);
    PyModule_AddObject(module, "Path", (PyObject *)&VRPyPath::type);

    VRPyPath::err = PyErr_NewException((char *)"VR.Error", NULL, NULL);
}

void VRPyPath::dealloc(VRPyPath* self) {
    self->ob_type->tp_free((PyObject*)self);
}

PyObject* VRPyPath::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    VRPyPath* self = (VRPyPath *)type->tp_alloc(type, 0);
    if (self != NULL) self->obj = new OSG::path();
    return (PyObject *)self;
}

PyObject* VRPyPath::fromPtr(OSG::path* obj) {
    VRPyPath *self = (VRPyPath *)type.tp_alloc(&type, 0);

    if (self != NULL) self->obj = obj;

    return (PyObject *)self;
}

int VRPyPath::init(VRPyPath *self, PyObject *args, PyObject *kwds) {
    return 0;
}


PyTypeObject VRPyPath::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Path",             /*tp_name*/
    sizeof(VRPyPath),             /*tp_basicsize*/
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
    "Path binding",           /* tp_doc */
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

PyMemberDef VRPyPath::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyPath::methods[] = {
    {"setStart", (PyCFunction)VRPyPath::setStartPoint, METH_VARARGS, "Set the path start point" },
    {"setEnd", (PyCFunction)VRPyPath::setEndPoint, METH_VARARGS, "Set the path end point" },
    {"getStart", (PyCFunction)VRPyPath::getStartPoint, METH_NOARGS, "Get the path start point" },
    {"getEnd", (PyCFunction)VRPyPath::getEndPoint, METH_NOARGS, "Get the path end point" },
    {"compute", (PyCFunction)VRPyPath::compute, METH_VARARGS, "Compute path" },
    {"update", (PyCFunction)VRPyPath::update, METH_NOARGS, "Update path" },
    {NULL}  /* Sentinel */
};


OSG::Vec3f PyVecToVec(PyObject* _v) {
    OSG::Vec3f v;
    PyObject *pi;

    std::string type = _v->ob_type->tp_name;
    if (type == "list") _v = PyList_AsTuple(_v);

    for (int i=0; i<3; i++) {
        pi = PyTuple_GetItem(_v, i);
        v[i] = PyFloat_AsDouble(pi);
    }
    return v;
}

PyObject* VRPyPath::setStartPoint(VRPyPath* self, PyObject* args) {
    PyObject *_p, *_n, *_c; _p=_n=_c=0;
    if (! PyArg_ParseTuple(args, "OOO", &_p, &_n, &_c)) return NULL;

    OSG::Vec3f p, n, c;
    p = PyVecToVec(_p);
    n = PyVecToVec(_n);
    c = PyVecToVec(_c);

    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::setStartPoint, Object is invalid"); return NULL; }
    self->obj->setStartPoint(p,n,c);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::setEndPoint(VRPyPath* self, PyObject* args) {
    PyObject *_p, *_n, *_c; _p=_n=_c=0;
    if (! PyArg_ParseTuple(args, "OOO", &_p, &_n, &_c)) return NULL;

    OSG::Vec3f p, n, c;
    p = PyVecToVec(_p);
    n = PyVecToVec(_n);
    c = PyVecToVec(_c);

    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::setEndPoint, Object is invalid"); return NULL; }
    self->obj->setEndPoint(p,n,c);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::getStartPoint(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getStartPoint, Object is invalid"); return NULL; }
    OSG::Vec3f v[3];
    self->obj->getStartPoint(v[0], v[1], v[2]);

    PyObject* res = PyTuple_New(3);
    for (int i=0; i<3; i++) {
        PyObject* l = PyList_New(3);
        for (int j=0; j<3; j++) {
            PyList_SetItem(l, j, PyFloat_FromDouble(v[i][j]));
        }
        PyTuple_SetItem(res, i, l);
    }

    return res;
}

PyObject* VRPyPath::getEndPoint(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::getEndPoint, Object is invalid"); return NULL; }
    OSG::Vec3f v[3];
    self->obj->getEndPoint(v[0], v[1], v[2]);

    PyObject* res = PyTuple_New(3);
    for (int i=0; i<3; i++) {
        PyObject* l = PyList_New(3);
        for (int j=0; j<3; j++) {
            PyList_SetItem(l, j, PyFloat_FromDouble(v[i][j]));
        }
        PyTuple_SetItem(res, i, l);
    }

    return res;
}

PyObject* VRPyPath::compute(VRPyPath* self, PyObject* args) {
    int N; N=0;
    if (! PyArg_ParseTuple(args, "i", &N)) return NULL;

    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::compute, Object is invalid"); return NULL; }
    self->obj->compute(N);
    Py_RETURN_TRUE;
}

PyObject* VRPyPath::update(VRPyPath* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPath::compute, Object is invalid"); return NULL; }
    self->obj->update();
    Py_RETURN_TRUE;
}
