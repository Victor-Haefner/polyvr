#include "VRPyRealWorld.h"
#include "../../core/scripting/VRPyTransform.h"
#include "../../core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<realworld::RealWorld>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.RealWorld",             /*tp_name*/
    sizeof(VRPyRealWorld),             /*tp_basicsize*/
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
    "VRRealWorld binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyRealWorld::methods,             /* tp_methods */
    VRPyRealWorld::members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_toZero,                 /* tp_new */
};

PyMemberDef VRPyRealWorld::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRealWorld::methods[] = {
    {"init", (PyCFunction)VRPyRealWorld::initWorld, METH_VARARGS, "Init real obj - init(root)" },
    {"update", (PyCFunction)VRPyRealWorld::update, METH_VARARGS, "Update real obj - update([x,y,z])" },
    {"physicalize", (PyCFunction)VRPyRealWorld::physicalize, METH_VARARGS, "Physicalize real obj - physicalize(bool)" },
    {"enableModule", (PyCFunction)VRPyRealWorld::enableModule, METH_VARARGS, "Enable a module - enableModule(str)" },
    {"disableModule", (PyCFunction)VRPyRealWorld::disableModule, METH_VARARGS, "Disable a module - disableModule(str)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyRealWorld::initWorld(VRPyRealWorld* self, PyObject* args) {
    PyObject* child = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    VRPyObject* _child = (VRPyObject*)child;

    if (_child->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::initWorld, root is invalid"); return NULL; }

    if (self->obj == 0) {
        self->obj = new realworld::RealWorld(_child->obj);
    }
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::update(VRPyRealWorld* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::physicalize, obj is invalid"); return NULL; }
    self->obj->update( parseVec3f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::physicalize(VRPyRealWorld* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::physicalize, obj is invalid"); return NULL; }
    self->obj->physicalize( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::enableModule(VRPyRealWorld* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::enableModule, obj is invalid"); return NULL; }
    self->obj->enableModule( parseString(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::disableModule(VRPyRealWorld* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::disableModule, obj is invalid"); return NULL; }
    self->obj->disableModule( parseString(args) );
    Py_RETURN_TRUE;
}
