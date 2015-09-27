#include "VRPySelector.h"
#include "VRPyObject.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

template<> PyTypeObject VRPyBaseT<OSG::VRSelector>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Selector",             /*tp_name*/
    sizeof(VRPySelector),             /*tp_basicsize*/
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
    VRPySelector::methods,             /* tp_methods */
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

PyMethodDef VRPySelector::methods[] = {
    {"select", (PyCFunction)VRPySelector::select, METH_VARARGS, "Select an object - select( object )" },
    {"deselect", (PyCFunction)VRPySelector::deselect, METH_NOARGS, "Deselect - deselect()" },
    {"get", (PyCFunction)VRPySelector::get, METH_NOARGS, "Return the selected object - object get()" },
    {"setColor", (PyCFunction)VRPySelector::setColor, METH_VARARGS, "Set the color of the selection - setColor([f,f,f])" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySelector::setColor(VRPySelector* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPySelector::setColor - Object is invalid"); return NULL; }
    self->obj->setColor(parseVec3f(args));
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::select(VRPySelector* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPySelector::select - Object is invalid"); return NULL; }
    VRPyObject* obj;
    parseObject(args, obj);
    if (obj == 0) return NULL;
    self->obj->select(obj->objPtr);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::deselect(VRPySelector* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPySelector::deselect - Object is invalid"); return NULL; }
    self->obj->select(0);
    Py_RETURN_TRUE;
}

PyObject* VRPySelector::get(VRPySelector* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPySelector::get - Object is invalid"); return NULL; }
    return VRPyTypeCaster::cast( self->obj->get() );
}
