#include "VRPyPose.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::pose>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Pose",             /*tp_name*/
    sizeof(VRPyPose),             /*tp_basicsize*/
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
    "Pose binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyPose::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    VRPyPose::New,                 /* tp_new */
};

PyMethodDef VRPyPose::methods[] = {
    {"pos", (PyCFunction)VRPyPose::pos, METH_VARARGS, "Get the position - [x,y,z] pos()" },
    {"dir", (PyCFunction)VRPyPose::dir, METH_VARARGS, "Get the direction - [x,y,z] dir()" },
    {"up", (PyCFunction)VRPyPose::up, METH_VARARGS, "Get the up vector - [x,y,z] up()" },
    {"set", (PyCFunction)VRPyPose::set, METH_VARARGS, "Set the pose - set([pos], [dir], [up])" },
    {"mult", (PyCFunction)VRPyPose::mult, METH_VARARGS, "Transform a vector - mult([vec])" },
    {"multInv", (PyCFunction)VRPyPose::multInv, METH_VARARGS, "Transform back a vector - multInv([vec])" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPose::New(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject *p, *d, *u;
    if (! PyArg_ParseTuple(args, "OOO", &p, &d, &u)) return NULL;
    return allocPtr( type, OSG::pose::create( parseVec3fList(p), parseVec3fList(d), parseVec3fList(u) ) );
}

PyObject* VRPyPose::set(VRPyPose* self, PyObject* args) {
    if (!self->valid()) return NULL;
    PyObject *p, *d, *u;
    if (! PyArg_ParseTuple(args, "OOO", &p, &d, &u)) return NULL;
    self->objPtr->set( parseVec3fList(p), parseVec3fList(d), parseVec3fList(u) );
    Py_RETURN_TRUE;
}

PyObject* VRPyPose::pos(VRPyPose* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->pos() );
}

PyObject* VRPyPose::dir(VRPyPose* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->dir() );
}

PyObject* VRPyPose::up(VRPyPose* self) {
    if (!self->valid()) return NULL;
    return toPyTuple( self->objPtr->up() );
}

PyObject* VRPyPose::mult(VRPyPose* self, PyObject* args) {
    if (!self->valid()) return NULL;
    auto v = parseVec3f(args);
    auto m = self->objPtr->asMatrix();
    m.mult(v,v);
    return toPyTuple( v );
}

PyObject* VRPyPose::multInv(VRPyPose* self, PyObject* args) {
    if (!self->valid()) return NULL;
    auto v = parseVec3f(args);
    auto m = self->objPtr->asMatrix();
    m.invert();
    m.mult(v,v);
    return toPyTuple( v );
}


