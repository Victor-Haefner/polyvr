#include "VRPyClipPlane.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRClipPlane>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.ClipPlane",             /*tp_name*/
    sizeof(VRPyClipPlane),             /*tp_basicsize*/
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
    "ClipPlane binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyClipPlane::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects,                 /* tp_new */
};

PyMethodDef VRPyClipPlane::methods[] = {
    {"setTree", (PyCFunction)VRPyClipPlane::setTree, METH_VARARGS, "Set the tree to apply the clipping plane to - setTree( object )" },
    {"setActive", (PyCFunction)VRPyClipPlane::setActive, METH_VARARGS, "Activate and deactivate the clipping - setActive( bool )" },
    {"isActive", (PyCFunction)VRPyClipPlane::isActive, METH_NOARGS, "Check if active - bool isActive()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyClipPlane::setTree(VRPyClipPlane* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyClipPlane::setTree - Object is invalid"); return NULL; }
    VRPyObject* o = 0;
    parseObject(args, o);
    if (o) self->obj->setTree(o->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPyClipPlane::setActive(VRPyClipPlane* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyClipPlane::setActive - Object is invalid"); return NULL; }
    self->obj->setActive( parseBool(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyClipPlane::isActive(VRPyClipPlane* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyClipPlane::isActive - Object is invalid"); return NULL; }
    return PyBool_FromLong( self->obj->isActive() );
}
