#include "VRPyColorChooser.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRColorChooser>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.ColorChooser",             /*tp_name*/
    sizeof(VRPyColorChooser),             /*tp_basicsize*/
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
    "ColorChooser binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyColorChooser::methods,             /* tp_methods */
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

PyMethodDef VRPyColorChooser::methods[] = {
    {"setGeometry", (PyCFunction)VRPyColorChooser::setGeometry, METH_VARARGS, "Set the geometry" },
    {"set", (PyCFunction)VRPyColorChooser::setColor, METH_VARARGS, "Set the color - set(r,g,b)" },
    {"get", (PyCFunction)VRPyColorChooser::getColor, METH_NOARGS, "Return the active color" },
    {"getLast", (PyCFunction)VRPyColorChooser::getLastColor, METH_NOARGS, "Return the previous color" },
    {"resolve", (PyCFunction)VRPyColorChooser::resolve, METH_VARARGS, "Get the color from device interaction - resolve(dev)" },
    {NULL}  /* Sentinel */
};


PyObject* VRPyColorChooser::setGeometry(VRPyColorChooser* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::activate - Object is invalid"); return NULL; }
    VRPyGeometry* geo = 0;
    if (! PyArg_ParseTuple(args, "O", &geo)) return NULL;
    self->obj->setGeometry(geo->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyColorChooser::resolve(VRPyColorChooser* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::activate - Object is invalid"); return NULL; }
    VRPyDevice* dev = 0;
    if (! PyArg_ParseTuple(args, "O", &dev)) return NULL;
    self->obj->resolve(dev->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyColorChooser::getColor(VRPyColorChooser* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::getColor - Object is invalid"); return NULL; }
    return toPyTuple( self->obj->getColor() );
}

PyObject* VRPyColorChooser::getLastColor(VRPyColorChooser* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::getColor - Object is invalid"); return NULL; }
    return toPyTuple( self->obj->getLastColor() );
}

PyObject* VRPyColorChooser::setColor(VRPyColorChooser* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyColorChooser::getColor - Object is invalid"); return NULL; }
    self->obj->setColor( parseVec3f(args) );
    Py_RETURN_TRUE;
}
