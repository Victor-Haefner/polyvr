#include "VRPyCaveKeeper.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyDevice.h"
#include "core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::CaveKeeper>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.CaveKeeper",             /*tp_name*/
    sizeof(VRPyCaveKeeper),             /*tp_basicsize*/
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
    "VRCaveKeeper binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyCaveKeeper::methods,             /* tp_methods */
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

PyMethodDef VRPyCaveKeeper::methods[] = {
    {"init", (PyCFunction)VRPyCaveKeeper::initWorld, METH_VARARGS, "Init real world" },
    {"update", (PyCFunction)VRPyCaveKeeper::update, METH_VARARGS, "Update real world" },
    {"dig", (PyCFunction)VRPyCaveKeeper::dig, METH_VARARGS, "Dig a cube, pass the device as argument" },
    {"place", (PyCFunction)VRPyCaveKeeper::place, METH_VARARGS, "Place an object, pass the device && the type of object as string" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCaveKeeper::initWorld(VRPyCaveKeeper* self, PyObject* args) {
    VRPyObject* child = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;

    if (child->obj == 0) { PyErr_SetString(err, "VRPyCaveKeeper::initWorld, root is invalid"); return NULL; }

    child->obj->addChild(self->obj->getAnchor());
    Py_RETURN_TRUE;
}

PyObject* VRPyCaveKeeper::update(VRPyCaveKeeper* self, PyObject* args) {
    PyObject* child = NULL;
    if (! PyArg_ParseTuple(args, "O", &child)) return NULL;
    if (child == NULL) { PyErr_SetString(err, "Missing child parameter"); return NULL; }
    VRPyTransform* _child = (VRPyTransform*)child;

    if (_child->obj == 0) { PyErr_SetString(err, "VRPyCaveKeeper::update, obj is invalid"); return NULL; }

    //self->obj->update(_child->obj->getWorldPosition());
    Py_RETURN_TRUE;
}

PyObject* VRPyCaveKeeper::dig(VRPyCaveKeeper* self, PyObject* args) {
    PyObject* dev = NULL;
    if (! PyArg_ParseTuple(args, "O", &dev)) return NULL;
    if (dev == NULL) { PyErr_SetString(err, "Missing device parameter"); return NULL; }
    VRPyDevice* _dev = (VRPyDevice*)dev;

    self->obj->dig(_dev->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyCaveKeeper::place(VRPyCaveKeeper* self, PyObject* args) {
    VRPyDevice* dev = NULL;
    const char *obj_t;
    VRPyTransform* geo = NULL;
    if (! PyArg_ParseTuple(args, "OsO", &dev, &obj_t, &geo)) return NULL;
    self->obj->place(dev->obj, obj_t, geo->objPtr);
    Py_RETURN_TRUE;
}
