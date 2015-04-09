#include "VRPyPathtool.h"
#include "VRPyObject.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRPathtool>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Pathtool",             /*tp_name*/
    sizeof(VRPyPathtool),             /*tp_basicsize*/
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
    "Pathtool binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyPathtool::methods,             /* tp_methods */
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

PyMethodDef VRPyPathtool::methods[] = {
    {"newPath", (PyCFunction)VRPyPathtool::newPath, METH_VARARGS, "Add a new path - int newPath(device, anchor)" },
    {"remPath", (PyCFunction)VRPyPathtool::remPath, METH_VARARGS, "Remove a path - remPath(int id)" },
    {"extrude", (PyCFunction)VRPyPathtool::extrude, METH_VARARGS, "Extrude a path - extrude(device, int id)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyPathtool::newPath(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::newPath - Object is invalid"); return NULL; }
    VRPyDevice* dev;
    VRPyObject* obj;
    if (! PyArg_ParseTuple(args, "OO", &dev, &obj)) return NULL;
    int id = self->obj->newPath( dev->obj, obj->obj );
    return PyInt_FromLong(id);
}

PyObject* VRPyPathtool::remPath(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::remPath - Object is invalid"); return NULL; }
    self->obj->remPath( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyPathtool::extrude(VRPyPathtool* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyPathtool::extrude - Object is invalid"); return NULL; }
    VRPyDevice* dev; int i;
    if (! PyArg_ParseTuple(args, "Oi", &dev, &i)) return NULL;
    self->obj->extrude( dev->obj, i );
    Py_RETURN_TRUE;
}

