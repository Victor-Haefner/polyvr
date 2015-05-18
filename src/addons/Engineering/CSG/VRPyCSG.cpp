#include "VRPyCSG.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::CSGGeometry>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.CSGGeometry",             /*tp_name*/
    sizeof(VRPyCSG),             /*tp_basicsize*/
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
    "VRCSG binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyCSG::methods,             /* tp_methods */
    VRPyCSG::members,             /* tp_members */
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

PyMemberDef VRPyCSG::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyCSG::methods[] = {
    {"getOperation", (PyCFunction)VRPyCSG::getOperation, METH_VARARGS, "get CSG operation" },
    {"setOperation", (PyCFunction)VRPyCSG::setOperation, METH_VARARGS, "set CSG operation - setOperation(string s)\n 'unite', 'subtract', 'intersect'" },
    {"getEditMode", (PyCFunction)VRPyCSG::getEditMode, METH_VARARGS, "get CSG object edit mode" },
    {"setEditMode", (PyCFunction)VRPyCSG::setEditMode, METH_VARARGS, "set CSG object edit mode - setEditMode(bool b)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyCSG::getOperation(VRPyCSG* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::getOperation, Object is invalid"); return NULL; }
    return PyString_FromString(self->obj->getOperation().c_str());
}

PyObject* VRPyCSG::setOperation(VRPyCSG* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::setOperation, Object is invalid"); return NULL; }
    string op = parseString(args);
    self->obj->setOperation(op);
    Py_RETURN_TRUE;
}

PyObject* VRPyCSG::getEditMode(VRPyCSG* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::getOperation, Object is invalid"); return NULL; }
    return PyBool_FromLong(self->obj->getEditMode());
}

PyObject* VRPyCSG::setEditMode(VRPyCSG* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyCSG::setEditMode, Object is invalid"); return NULL; }
    bool b = parseBool(args);
	return PyBool_FromLong(self->obj->setEditMode(b));
}
